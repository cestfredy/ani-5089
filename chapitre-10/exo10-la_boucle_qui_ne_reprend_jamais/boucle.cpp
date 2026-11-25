#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"

#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Core/NkCamera.h"
#include "NKRenderer/Core/NkRenderGraph.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/NkMaterialCollection.h"
#include "NKRenderer/Mesh/NkMeshSystem.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"
#include "NKRenderer/Tools/Render2D/NkRender2D.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace nkentseu;
using namespace nkentseu::renderer;

static void ConfigureAppData(NkAppData& d) {
    d.appName = "MaSalle";
}
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

const float32 PIECE = 4.00f;
const float32 PLAFOND = 2.50f;
const float32 MUR = 0.10f;
const float32 PORTE_L = 0.90f;
const float32 PORTE_H = 2.00f;
const float32 FEN_L = 1.20f;
const float32 FEN_H = 1.00f;
const float32 FEN_Y = 1.40f;
const float32 TABLE_L = 1.20f;
const float32 TABLE_P = 0.80f;
const float32 TABLE_H = 0.75f;
const float32 DEMI = PIECE * 0.5f;

const NkVec3f OEIL = {0.30f, 1.70f, 1.20f};
const float64 PERIODE_TRAME = 1.0 / 72.0;
const float64 AVANCE_PREDICTION = 0.028;
const uint64 DEBUT_PAUSE = 6;
const uint64 FIN_PAUSE = 10;
const uint64 TRAMES_RELEVEES = 20;

enum class Espace {
    LOCAL,
    STAGE
};

struct Vue {
    float32 lacet = 0.f;
    float32 fovY = 70.f;
};

struct Chaine {
    uint64 poignee = 0;
    uint32 images = 3;
};

struct VueDeCouche {
    Vue vue;
    const Chaine* chaine = nullptr;
    uint32 indexImage = 0;
};

struct CoucheProjection {
    Espace espace = Espace::STAGE;
    VueDeCouche vues[2];
};

struct FinDeTrame {
    float64 instantAffichage = 0.0;
    const CoucheProjection* const* couches = nullptr;
    uint32 nombreCouches = 0;
};

struct EtatDeTrame {
    float64 instantPredit = 0.0;
    bool shouldRender = true;
};

class SessionSimulee {
public:
    void PomperEvenements() {}

    bool AttendreTrame(uint64 trame, EtatDeTrame& et) {
        if (mOuverte) {
            return false;
        }
        et.instantPredit = (float64)trame * PERIODE_TRAME + AVANCE_PREDICTION;
        et.shouldRender = true;
        mAttendue = true;
        return true;
    }

    bool CommencerTrame() {
        if (!mAttendue) {
            return false;
        }
        mAttendue = false;
        mOuverte = true;
        return true;
    }

    void SynchroniserActions() {}

    void LocaliserVues(uint64 trame, Vue vues[2]) const {
        for (int32 oeil = 0; oeil < 2; ++oeil) {
            vues[oeil].lacet = 10.f - (float32)trame;
        }
    }

    uint32 Acquerir() const {
        return mIndex;
    }

    void Liberer(const Chaine& chaine) {
        mIndex = (mIndex + 1) % chaine.images;
    }

    bool FinTrame(const FinDeTrame& fin) {
        if (!mOuverte) {
            return false;
        }
        mOuverte = false;
        mCouches = fin.nombreCouches;
        return true;
    }

private:
    uint32 mIndex = 0;
    uint32 mCouches = 0;
    bool mAttendue = false;
    bool mOuverte = false;
};

struct Releve {
    bool rend = false;
    bool debut = false;
    bool fin = false;
};

static float32 Radians(float32 degres) {
    return degres * 3.14159265f / 180.f;
}

static void Pave(NkRender3D* r3d, NkMeshSystem* meshes, const NkVec3f& centre,
                 const NkVec3f& taille, const NkVec3f& teinte, float32 rugosite) {
    NkDrawCall3D dc;
    dc.mesh = meshes->GetCube();
    dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
    NkVec3f moitie = taille * 0.5f;
    dc.aabb = {centre - moitie, centre + moitie};
    dc.tint = teinte;
    dc.metallic = 0.f;
    dc.roughness = rugosite;
    r3d->Submit(dc);
}

static void SoumettreLeSol(NkRender3D* r3d, NkMeshSystem* meshes) {
    NkDrawCall3D dc;
    dc.mesh = meshes->GetPlane();
    dc.transform = NkMat4f::Scale({PIECE, 1.f, PIECE});
    dc.aabb = {{-DEMI, 0.f, -DEMI}, {DEMI, 0.f, DEMI}};
    dc.castShadow = false;
    dc.tint = {0.38f, 0.36f, 0.34f};
    dc.metallic = 0.f;
    dc.roughness = 1.f;
    r3d->Submit(dc);
}

static void SoumettreLaTable(NkRender3D* r3d, NkMeshSystem* meshes, const NkVec3f& bois) {
    const float32 tx = 0.60f;
    const float32 tz = 0.70f;
    const float32 ep = 0.05f;
    const float32 pied = 0.06f;
    const float32 hp = TABLE_H - ep;
    Pave(r3d, meshes, {tx, TABLE_H - ep * 0.5f, tz}, {TABLE_L, ep, TABLE_P}, bois, 0.5f);
    for (int32 i = 0; i < 4; ++i) {
        float32 sx = 1.f;
        float32 sz = 1.f;
        if ((i & 1) == 0) {
            sx = -1.f;
        }
        if ((i & 2) == 0) {
            sz = -1.f;
        }
        float32 dx = sx * (TABLE_L * 0.5f - pied);
        float32 dz = sz * (TABLE_P * 0.5f - pied);
        Pave(r3d, meshes, {tx + dx, hp * 0.5f, tz + dz}, {pied, hp, pied}, bois, 0.6f);
    }
}

static void SoumettreLaSalle(NkRender3D* r3d, NkMeshSystem* meshes) {
    const NkVec3f grisMur = {0.72f, 0.70f, 0.66f};
    const NkVec3f bois = {0.55f, 0.38f, 0.22f};
    const NkVec3f vitre = {0.75f, 0.85f, 0.92f};
    const float32 yMur = PLAFOND * 0.5f;
    const float32 bord = DEMI + MUR * 0.5f;

    SoumettreLeSol(r3d, meshes);
    Pave(r3d, meshes, {0.f, yMur, -bord}, {PIECE, PLAFOND, MUR}, grisMur, 1.f);
    Pave(r3d, meshes, {0.f, yMur, bord}, {PIECE, PLAFOND, MUR}, grisMur, 1.f);
    Pave(r3d, meshes, {-bord, yMur, 0.f}, {MUR, PLAFOND, PIECE}, grisMur, 1.f);
    Pave(r3d, meshes, {bord, yMur, 0.f}, {MUR, PLAFOND, PIECE}, grisMur, 1.f);
    Pave(r3d, meshes, {0.f, PLAFOND + MUR * 0.5f, 0.f}, {PIECE, MUR, PIECE}, grisMur, 1.f);
    Pave(r3d, meshes, {-0.70f, PORTE_H * 0.5f, -DEMI + 0.02f}, {PORTE_L, PORTE_H, 0.04f}, bois,
         0.6f);
    Pave(r3d, meshes, {-DEMI + 0.02f, FEN_Y, -0.90f}, {0.04f, FEN_H, FEN_L}, vitre, 0.15f);
    Pave(r3d, meshes, {-DEMI + 0.02f, FEN_Y, 0.90f}, {0.04f, FEN_H, FEN_L}, vitre, 0.15f);
    SoumettreLaTable(r3d, meshes, bois);
    Pave(r3d, meshes, {-1.10f, 0.50f, 1.20f}, {1.f, 1.f, 1.f}, {0.85f, 0.80f, 0.30f}, 0.8f);
}

static void SoumettreLesPerles(NkRender3D* r3d, NkMeshSystem* meshes, const Releve* releves,
                               uint64 trame) {
    const NkVec3f vert = {0.15f, 0.85f, 0.25f};
    const NkVec3f gris = {0.30f, 0.30f, 0.30f};
    for (uint64 k = 1; k <= trame && k <= TRAMES_RELEVEES; ++k) {
        NkVec3f teinte = gris;
        if (releves[k].rend) {
            teinte = vert;
        }
        float32 x = -1.52f + 0.16f * (float32)(k - 1);
        Pave(r3d, meshes, {x, 1.95f, -DEMI + 0.06f}, {0.10f, 0.10f, 0.06f}, teinte, 0.5f);
    }
}

static void Dessiner(NkRenderer* renderer, NkICommandBuffer* cmd, float32 aspect, float32 lacet,
                     const Releve* releves, uint64 trame) {
    NkRender3D* r3d = renderer->GetRender3D();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = OEIL;
    cd.target = {OEIL.x - std::sin(Radians(lacet)), OEIL.y - 0.05f,
                 OEIL.z - std::cos(Radians(lacet))};
    cd.up = {0.f, 1.f, 0.f};
    cd.fovY = 70.f;
    cd.aspect = aspect;
    cd.nearPlane = 0.05f;
    cd.farPlane = 100.f;

    NkSceneContext sctx;
    sctx.camera = NkCamera3D(cd);
    sctx.time = (float32)trame / 72.f;
    r3d->SetIBLStrength(0.35f);
    NkLightDesc soleil;
    soleil.type = NkLightType::NK_DIRECTIONAL;
    soleil.direction = {-0.5f, -1.f, -0.35f};
    soleil.color = {1.f, 0.96f, 0.88f};
    soleil.intensity = 3.f;
    soleil.castShadow = false;
    sctx.lights.PushBack(soleil);

    r3d->BeginScene(sctx);
    NkMeshSystem* meshes = renderer->GetMeshSystem();
    SoumettreLaSalle(r3d, meshes);
    SoumettreLesPerles(r3d, meshes, releves, trame);
    NkRenderGraph* graphe = renderer->GetRenderGraph();
    if (graphe != nullptr) {
        graphe->Execute(cmd);
    }
}

static uint64 EnvU64(const char* nom) {
    const char* t = std::getenv(nom);
    if (t == nullptr || t[0] == '\0') {
        return 0;
    }
    return (uint64)std::strtoull(t, nullptr, 10);
}

static bool Fermee() {
    bool fermee = false;
    NkEvent* ev = nullptr;
    while (NkEvents().PollEvent(ev)) {
        if (ev->As<NkWindowCloseEvent>() != nullptr) {
            fermee = true;
        }
    }
    return fermee;
}

static void Afficher(NkRenderer* rEcran, NkTexHandle couleur, uint32 largeur, uint32 hauteur) {
    NkICommandBuffer* cmd = rEcran->GetCmd();
    NkRender2D* r2d = rEcran->GetRender2D();
    if (r2d != nullptr) {
        r2d->Begin(cmd, largeur, hauteur);
        r2d->DrawImage(couleur, {0.f, 0.f, (float32)largeur, (float32)hauteur});
        r2d->End();
    }
    rEcran->Present();
    rEcran->EndFrame();
}

static void Capturer(NkOffscreenTarget* cible, uint64 trame) {
    const char* chemin = std::getenv("MASALLE_SHOT_PATH");
    if (chemin == nullptr || chemin[0] == '\0') {
        chemin = "masalle.png";
    }
    bool ok = cible->Capture(chemin);
    std::printf("[MaSalle] capture trame %llu -> %s (%s)\n", (unsigned long long)trame, chemin,
                ok ? "ok" : "echec");
}

int nkmain(const NkEntryState&) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    uint64 trameDeSortie = EnvU64("MASALLE_EXIT");
    uint64 trameDeCapture = EnvU64("MASALLE_SHOT");

    NkWindowConfig cfg;
    cfg.title = "Ma salle - la boucle";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        return 1;
    }

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    NkIDevice* device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (device == nullptr) {
        return 1;
    }
    NkRendererConfig cfgEcran = NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
    cfgEcran.Enable(NK_SS_OFFSCREEN);
    NkRenderer* rEcran = NkRenderer::Create(device, cfgEcran);
    if (rEcran == nullptr || !rEcran->Initialize()) {
        NkDeviceFactory::Destroy(device);
        return 1;
    }

    NkOffscreenDesc od;
    od.width = devInit.width;
    od.height = devInit.height;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = true;
    od.name = "MaSalle_Cible";
    NkOffscreenTarget* cible = rEcran->CreateOffscreen(od);
    if (cible == nullptr) {
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }

    NkRendererConfig cfgSalle = NkRendererConfig::ForGame(devInit.api, devInit.width,
                                                          devInit.height);
    cfgSalle.Disable(NK_SS_RENDER2D);
    cfgSalle.Disable(NK_SS_TEXT);
    cfgSalle.Disable(NK_SS_OVERLAY);
    cfgSalle.Enable(NK_SS_OFFSCREEN);
    NkRenderer* renderer = NkRenderer::Create(device, cfgSalle);
    if (renderer == nullptr || !renderer->Initialize()) {
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    NkTexHandle couleur = cible->GetColorResult();
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(couleur));

    float32 aspect = (float32)devInit.width / (float32)devInit.height;
    SessionSimulee session;
    const Chaine chaine{0x51A1u, 3};
    Releve releves[TRAMES_RELEVEES + 1];
    std::FILE* journal = std::fopen("journal.txt", "w");

    uint64 trame = 0;
    while (fenetre.IsOpen() && !Fermee()) {
        ++trame;
        Releve r;
        bool ecranOuvert = rEcran->BeginFrame();
        session.PomperEvenements();
        EtatDeTrame et;
        if (session.AttendreTrame(trame, et)) {
            if (trame >= DEBUT_PAUSE && trame <= FIN_PAUSE) {
                et.shouldRender = false;
            }
            r.debut = session.CommencerTrame();
            session.SynchroniserActions();

            CoucheProjection couche;
            const CoucheProjection* liste[1] = {&couche};
            FinDeTrame fin;
            fin.instantAffichage = et.instantPredit;
            fin.couches = liste;
            fin.nombreCouches = 0;
            r.rend = et.shouldRender;
            if (trame <= TRAMES_RELEVEES) {
                releves[trame].rend = r.rend;
            }

            if (et.shouldRender) {
                Vue vues[2];
                session.LocaliserVues(trame, vues);
                uint32 index = session.Acquerir();
                if (ecranOuvert) {
                    Dessiner(renderer, rEcran->GetCmd(), aspect, vues[0].lacet, releves, trame);
                }
                session.Liberer(chaine);
                for (int32 oeil = 0; oeil < 2; ++oeil) {
                    couche.vues[oeil].vue = vues[oeil];
                    couche.vues[oeil].chaine = &chaine;
                    couche.vues[oeil].indexImage = index;
                }
                fin.nombreCouches = 1;
                // Version fautive : la fin de trame dans le test de rendu (journal.txt,
                // avant.png). Aux trames de pause, la trame reste ouverte.
                //
                // r.fin = session.FinTrame(fin);
            }

            r.fin = session.FinTrame(fin);
        }
        if (ecranOuvert) {
            Afficher(rEcran, couleur, devInit.width, devInit.height);
        }

        if (journal != nullptr && trame <= TRAMES_RELEVEES) {
            std::fprintf(journal, "trame : %llu, rend : %d, debut : %d, fin : %d\n",
                         (unsigned long long)trame, r.rend ? 1 : 0, r.debut ? 1 : 0,
                         r.fin ? 1 : 0);
        }
        std::printf("[MaSalle] trame %llu : rend %d, debut %d, fin %d\n", (unsigned long long)trame,
                    r.rend ? 1 : 0, r.debut ? 1 : 0, r.fin ? 1 : 0);
        if (trameDeCapture != 0 && trame == trameDeCapture) {
            Capturer(cible, trame);
        }
        if (trameDeSortie != 0 && trame >= trameDeSortie) {
            break;
        }
    }

    if (journal != nullptr) {
        std::fclose(journal);
    }
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
