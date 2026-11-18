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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

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

const float32 DT = 1.f / 60.f;
const float32 COTE = 0.10f;
const float32 PORTEE = 0.28f;
const NkVec3f CUBE_DEPART = {0.55f, 0.80f, 0.70f};
const NkVec3f DANS_LA_PAUME = {0.f, -0.06f, 0.f};
const uint64 DEBUT_SAISIE = 20;
const uint64 DEBUT_EXTINCTION = 40;
const uint64 FIN_EXTINCTION = 50;
const uint64 DEBUT_JOURNAL = 35;
const uint64 FIN_JOURNAL = 55;

struct EtatAction {
    bool courant = false;
    bool change = false;
    bool actif = false;
};

class Action {
public:
    Action(std::string nom, std::string usage) : mNom(std::move(nom)), mUsage(std::move(usage)) {}

    void Lier(const bool* source) {
        mSource = source;
    }

    void Synchroniser() {
        bool precedent = mEtat.courant;
        mEtat.actif = mSource != nullptr;
        mEtat.courant = mEtat.actif && *mSource;
        mEtat.change = mEtat.courant != precedent;
    }

    const EtatAction& Etat() const {
        return mEtat;
    }

private:
    std::string mNom;
    std::string mUsage;
    const bool* mSource = nullptr;
    EtatAction mEtat;
};

struct Prise {
    NkMat4f poseCube;
    NkMat4f attache;
    bool tenu = false;
    float32 vitesseY = 0.f;
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
}

static NkVec3f Position(const NkMat4f& m) {
    return {m.data[12], m.data[13], m.data[14]};
}

static NkMat4f PoseDePaume(uint64 image) {
    NkVec3f prise = {CUBE_DEPART.x, CUBE_DEPART.y + 0.25f, CUBE_DEPART.z};
    NkVec3f p;
    float32 lacet = 0.f;
    if (image <= DEBUT_SAISIE) {
        float32 a = 1.f - (float32)image / (float32)DEBUT_SAISIE;
        p = {prise.x + 0.25f * a, prise.y + 0.35f * a, prise.z + 0.20f * a};
    } else {
        float32 a = (float32)std::min<uint64>(image - DEBUT_SAISIE, 40) / 40.f;
        p = {prise.x - 0.30f * a, prise.y + 0.45f * a, prise.z + 0.35f * a};
        lacet = 40.f * a;
    }
    return NkMat4f::Translate(p) * NkMat4f::RotationY(NkAngle(lacet));
}

static void Objet(NkRender3D* r3d, NkMeshHandle mesh, const NkMat4f& pose, float32 cote,
                  const NkVec3f& teinte) {
    NkDrawCall3D dc;
    dc.mesh = mesh;
    dc.transform = pose * NkMat4f::Scale({cote, cote, cote});
    NkVec3f c = Position(pose);
    NkVec3f r = {cote, cote, cote};
    dc.aabb = {c - r, c + r};
    dc.tint = teinte;
    dc.metallic = 0.f;
    dc.roughness = 0.6f;
    r3d->Submit(dc);
}

static void Tomber(NkMat4f& pose, float32& vitesseY) {
    vitesseY -= 9.81f * DT;
    NkVec3f c = Position(pose);
    c.y += vitesseY * DT;
    bool surLaTable = std::fabs(c.x - 0.60f) < 0.60f && std::fabs(c.z - 0.70f) < 0.40f;
    float32 appui = COTE * 0.5f;
    if (surLaTable) {
        appui += TABLE_H;
    }
    if (c.y < appui) {
        c.y = appui;
        vitesseY = 0.f;
    }
    pose.data[12] = c.x;
    pose.data[13] = c.y;
    pose.data[14] = c.z;
}

static bool APortee(const Prise& p, const NkMat4f& paume) {
    return (Position(paume) - Position(p.poseCube)).Len() < PORTEE;
}

static bool Lacher(const EtatAction& e, bool sansActif) {
    if (sansActif) {
        return !e.courant;
    }
    return e.actif && !e.courant && e.change;
}

static void Tenir(Prise& p, const EtatAction& e, const NkMat4f& paume, bool sansActif) {
    if (!p.tenu && e.courant && e.change && APortee(p, paume)) {
        p.attache = NkMat4f::Translate(DANS_LA_PAUME);
        p.tenu = true;
        p.vitesseY = 0.f;
    }
    if (p.tenu && Lacher(e, sansActif)) {
        p.tenu = false;
    }
    if (p.tenu) {
        p.poseCube = paume * p.attache;
    } else {
        Tomber(p.poseCube, p.vitesseY);
    }
}

static void Scene(NkRenderer* renderer, const NkMat4f& paume, const Prise& p, bool suivie,
                  float32 aspect, uint64 image) {
    NkRender3D* r3d = renderer->GetRender3D();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = {-0.55f, 1.05f, 1.95f};
    cd.target = {0.45f, 1.00f, 0.60f};
    cd.up = {0.f, 1.f, 0.f};
    cd.fovY = 70.f;
    cd.aspect = aspect;
    cd.nearPlane = 0.05f;
    cd.farPlane = 100.f;

    NkSceneContext sctx;
    sctx.camera = NkCamera3D(cd);
    sctx.time = (float32)image / 60.f;
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
    Objet(r3d, meshes->GetCube(), p.poseCube, COTE, {0.85f, 0.25f, 0.20f});
    NkVec3f teinteMain = {0.95f, 0.80f, 0.65f};
    if (!suivie) {
        teinteMain = {0.35f, 0.35f, 0.35f};
    }
    Objet(r3d, meshes->GetSphere(), paume, 0.05f, teinteMain);
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

static void Capturer(NkOffscreenTarget* cible, uint64 image) {
    const char* chemin = std::getenv("MASALLE_SHOT_PATH");
    if (chemin == nullptr || chemin[0] == '\0') {
        chemin = "masalle.png";
    }
    bool ok = cible->Capture(chemin);
    std::printf("[MaSalle] capture image %llu -> %s (%s)\n", (unsigned long long)image, chemin,
                ok ? "ok" : "echec");
}

int nkmain(const NkEntryState&) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    uint64 imageDeSortie = EnvU64("MASALLE_EXIT");
    uint64 imageDeCapture = EnvU64("MASALLE_SHOT");
    bool sansActif = EnvU64("MASALLE_SANS_ACTIF") == 1;

    NkWindowConfig cfg;
    cfg.title = "Ma salle - la manette qui s'eteint";
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

    bool gripDroit = false;
    Action saisir("saisir", "GRAB");
    saisir.Lier(&gripDroit);
    Prise prise;
    prise.poseCube = NkMat4f::Translate(CUBE_DEPART);
    float32 aspect = (float32)devInit.width / (float32)devInit.height;
    NkMat4f paume = PoseDePaume(0);
    std::FILE* journal = std::fopen("journal.txt", "w");
    std::printf("[MaSalle] version %s\n", sansActif ? "sans tester actif" : "qui teste actif");

    uint64 image = 0;
    while (fenetre.IsOpen() && !Fermee()) {
        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            gripDroit = image >= DEBUT_SAISIE;
            bool eteinte = image >= DEBUT_EXTINCTION && image < FIN_EXTINCTION;
            if (eteinte) {
                saisir.Lier(nullptr);
            } else {
                saisir.Lier(&gripDroit);
            }
            saisir.Synchroniser();
            const EtatAction& e = saisir.Etat();
            if (e.actif) {
                paume = PoseDePaume(image);
            }
            Tenir(prise, e, paume, sansActif);
            Scene(renderer, paume, prise, e.actif, aspect, image);
            bool journalise = image >= DEBUT_JOURNAL && image < FIN_JOURNAL;
            if (journal != nullptr && journalise) {
                std::fprintf(journal, "image : %llu, courant : %d, change : %d, actif : %d\n",
                             (unsigned long long)image, e.courant, e.change, e.actif);
            }
            if (journalise) {
                std::printf("[MaSalle] image %llu : courant %d change %d actif %d, cube %s, "
                            "hauteur %.3f m\n",
                            (unsigned long long)image, e.courant, e.change, e.actif,
                            prise.tenu ? "tenu" : "libre", (double)Position(prise.poseCube).y);
            }
            NkRenderGraph* graphe = renderer->GetRenderGraph();
            if (graphe != nullptr) {
                graphe->Execute(cmd);
            }
            NkRender2D* r2d = rEcran->GetRender2D();
            if (r2d != nullptr) {
                r2d->Begin(cmd, devInit.width, devInit.height);
                r2d->DrawImage(couleur, {0.f, 0.f, (float32)devInit.width,
                                         (float32)devInit.height});
                r2d->End();
            }
            rEcran->Present();
            rEcran->EndFrame();
            ++image;
            if (imageDeCapture != 0 && image == imageDeCapture) {
                Capturer(cible, image);
            }
        }
        if (imageDeSortie != 0 && image >= imageDeSortie) {
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
