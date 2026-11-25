#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"

#include "NKImage/Core/NkImage.h"
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
const float64 DEMI_TOUR = 3.14159265358979;

const NkVec3f OEIL = {0.30f, 1.70f, 1.20f};
const NkVec3f POTEAU = {-0.40f, 1.25f, -1.00f};
const float64 PERIODE_TRAME = 1.0 / 72.0;
const float64 AVANCE_PREDICTION = 0.028;
const float64 RETARD = 0.028;
const float64 DUREE_DU_RENDU = 0.045;

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
    bool doitDessiner = true;
};

struct Composition {
    float32 lacetSoumis = 0.f;
    float32 lacetReel = 0.f;
    float32 correction = 0.f;
    bool couche = false;
};

static float32 Radians(float32 degres) {
    return degres * (float32)DEMI_TOUR / 180.f;
}

static float32 LacetDuPoteau() {
    return (float32)(std::atan2(OEIL.x - POTEAU.x, OEIL.z - POTEAU.z) * 180.0 / DEMI_TOUR);
}

class SessionSimulee {
public:
    static float32 Tete(float64 t) {
        return LacetDuPoteau() + 60.f * (float32)std::sin(DEMI_TOUR * t);
    }

    bool AttendreTrame(uint64 trame, EtatDeTrame& et) {
        if (mOuverte) {
            return false;
        }
        mMaintenant = (float64)trame * PERIODE_TRAME;
        et.instantPredit = mMaintenant + AVANCE_PREDICTION;
        et.doitDessiner = true;
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

    void LocaliserVues(float64 instant, Vue vues[2]) const {
        for (int32 oeil = 0; oeil < 2; ++oeil) {
            vues[oeil].lacet = Tete(instant);
        }
    }

    uint32 Acquerir() const {
        return mIndex;
    }

    void Liberer(const Chaine& chaine) {
        mIndex = (mIndex + 1) % chaine.images;
    }

    float64 Maintenant() const {
        return mMaintenant;
    }

    void AvancerHorloge(float64 dt) {
        mMaintenant += dt;
    }

    bool FinTrame(const FinDeTrame& fin) {
        if (!mOuverte) {
            return false;
        }
        mOuverte = false;
        mComposition = Composition{};
        if (fin.nombreCouches > 0) {
            const VueDeCouche& v = fin.couches[0]->vues[0];
            mComposition.couche = true;
            mComposition.lacetSoumis = v.vue.lacet;
            mComposition.lacetReel = Tete(fin.instantAffichage + RETARD);
            mComposition.correction = mComposition.lacetReel - v.vue.lacet;
        }
        return true;
    }

    const Composition& DerniereComposition() const {
        return mComposition;
    }

private:
    float64 mMaintenant = 0.0;
    uint32 mIndex = 0;
    bool mAttendue = false;
    bool mOuverte = false;
    Composition mComposition;
};

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
    Pave(r3d, meshes, POTEAU, {0.06f, 2.50f, 0.06f}, {0.95f, 0.80f, 0.10f}, 0.5f);
}

static void Dessiner(NkRenderer* renderer, NkICommandBuffer* cmd, float32 aspect, float32 lacet,
                     float32 temps) {
    NkRender3D* r3d = renderer->GetRender3D();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = OEIL;
    cd.target = {OEIL.x - std::sin(Radians(lacet)), OEIL.y - 0.12f,
                 OEIL.z - std::cos(Radians(lacet))};
    cd.up = {0.f, 1.f, 0.f};
    cd.fovY = 70.f;
    cd.aspect = aspect;
    cd.nearPlane = 0.05f;
    cd.farPlane = 100.f;

    NkSceneContext sctx;
    sctx.camera = NkCamera3D(cd);
    sctx.time = temps;
    r3d->SetIBLStrength(0.35f);
    NkLightDesc soleil;
    soleil.type = NkLightType::NK_DIRECTIONAL;
    soleil.direction = {-0.5f, -1.f, -0.35f};
    soleil.color = {1.f, 0.96f, 0.88f};
    soleil.intensity = 3.f;
    soleil.castShadow = false;
    sctx.lights.PushBack(soleil);

    r3d->BeginScene(sctx);
    SoumettreLaSalle(r3d, renderer->GetMeshSystem());
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

static int32 ColonneDuPoteau(float32 lacetReel, int32 largeur, float32 focalePx) {
    float32 a = Radians(lacetReel);
    float32 px = POTEAU.x - OEIL.x;
    float32 pz = POTEAU.z - OEIL.z;
    float32 devant = -std::sin(a) * px - std::cos(a) * pz;
    float32 droite = std::cos(a) * px - std::sin(a) * pz;
    return (int32)std::lround((float32)largeur * 0.5f + focalePx * droite / devant);
}

static void Composer(const char* chemin, const Composition& c, float32 erreur, float32 focalePx) {
    NkImage brut;
    if (!brut.Load(chemin, 4)) {
        return;
    }
    int32 w = brut.Width();
    int32 h = brut.Height();
    int32 dx = (int32)std::lround(focalePx * std::tan(Radians(c.correction)));
    NkImage compo = NkImage::Create((uint32)w, (uint32)h, (int32)4, (uint32)0x000000FFu);
    math::NkColor noir(0, 0, 0, 255);
    for (int32 y = 0; y < h; ++y) {
        for (int32 x = 0; x < w; ++x) {
            int32 sx = x - dx;
            bool dedans = sx >= 0 && sx < w;
            compo.SetPixel(x, y, dedans ? brut.GetPixel(sx, y) : noir);
        }
    }
    int32 xRef = ColonneDuPoteau(c.lacetReel, w, focalePx);
    math::NkColor vert(40, 230, 60, 255);
    for (int32 y = 0; y < h; ++y) {
        bool marge = y <= h * 12 / 100 || y >= h * 88 / 100;
        for (int32 x = xRef - 2; marge && x <= xRef + 2; ++x) {
            if (x >= 0 && x < w) {
                compo.SetPixel(x, y, vert);
            }
        }
    }
    compo.SavePNG(chemin);
    std::printf("[MaSalle] image composee : correction %.2f deg (%d px), erreur restante %.2f deg "
                "(%.0f px)\n",
                (double)c.correction, dx, (double)erreur,
                (double)(focalePx * std::tan(Radians(erreur))));
}

static void Capturer(NkOffscreenTarget* cible, uint64 image, const Composition& c, float32 erreur,
                     float32 focalePx) {
    const char* chemin = std::getenv("MASALLE_SHOT_PATH");
    if (chemin == nullptr || chemin[0] == '\0') {
        chemin = "masalle.png";
    }
    bool ok = cible->Capture(chemin);
    std::printf("[MaSalle] capture image %llu -> %s (%s)\n", (unsigned long long)image, chemin,
                ok ? "ok" : "echec");
    if (ok) {
        Composer(chemin, c, erreur, focalePx);
    }
}

int nkmain(const NkEntryState&) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    uint64 imageDeSortie = EnvU64("MASALLE_EXIT");
    uint64 imageDeCapture = EnvU64("MASALLE_SHOT");

    NkWindowConfig cfg;
    cfg.title = "Ma salle - la pose du rendu";
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
    float32 demiChampH = std::atan(std::tan(Radians(35.f)) * aspect);
    float32 focalePx = (float32)devInit.width * 0.5f / std::tan(demiChampH);
    SessionSimulee session;
    const Chaine chaine{0x51A1u, 3};

    uint64 image = 0;
    while (fenetre.IsOpen() && !Fermee()) {
        EtatDeTrame et;
        session.AttendreTrame(image, et);
        session.CommencerTrame();
        session.SynchroniserActions();
        Vue vuesDuRendu[2];
        session.LocaliserVues(et.instantPredit, vuesDuRendu);
        float32 erreur = 0.f;
        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            uint32 index = session.Acquerir();
            Dessiner(renderer, cmd, aspect, vuesDuRendu[0].lacet, (float32)image / 72.f);
            session.Liberer(chaine);
            session.AvancerHorloge(DUREE_DU_RENDU);

            CoucheProjection couche;
            couche.espace = Espace::STAGE;
            for (int32 oeil = 0; oeil < 2; ++oeil) {
                couche.vues[oeil].vue = vuesDuRendu[oeil];
                couche.vues[oeil].chaine = &chaine;
                couche.vues[oeil].indexImage = index;
            }

            // Version fautive : redemander les vues juste avant de soumettre, pour
            // l'instant courant (fraiche.png).
            //
            // Vue vuesFraiches[2];
            // session.LocaliserVues(session.Maintenant(), vuesFraiches);
            // for (int32 oeil = 0; oeil < 2; ++oeil) {
            //     couche.vues[oeil].vue = vuesFraiches[oeil];
            // }

            const CoucheProjection* liste[1] = {&couche};
            FinDeTrame fin;
            fin.instantAffichage = et.instantPredit;
            fin.couches = liste;
            fin.nombreCouches = 1;
            session.FinTrame(fin);

            const Composition& c = session.DerniereComposition();
            erreur = vuesDuRendu[0].lacet - c.lacetSoumis;
            float32 dx = focalePx * std::tan(Radians(c.correction));
            NkRender2D* r2d = rEcran->GetRender2D();
            if (r2d != nullptr) {
                r2d->Begin(cmd, devInit.width, devInit.height);
                r2d->DrawImage(couleur, {dx, 0.f, (float32)devInit.width,
                                         (float32)devInit.height});
                r2d->End();
            }
            rEcran->Present();
            rEcran->EndFrame();
            if (image % 12 == 0) {
                std::printf("[MaSalle] image %llu : lacet rendu %.2f, soumis %.2f, reel %.2f, "
                            "correction %.2f, erreur restante %.2f deg\n",
                            (unsigned long long)image, (double)vuesDuRendu[0].lacet,
                            (double)c.lacetSoumis, (double)c.lacetReel, (double)c.correction,
                            (double)erreur);
            }
            ++image;
            if (imageDeCapture != 0 && image == imageDeCapture) {
                Capturer(cible, image, c, erreur, focalePx);
            }
        }
        if (imageDeSortie != 0 && image >= imageDeSortie) {
            break;
        }
    }

    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
