#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKEvent/NkKeyboardEvent.h"

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

const NkVec3f TETE = {0.30f, 1.70f, 1.20f};

struct Pointeur {
    NkWindow* fenetre = nullptr;
    bool pris = false;
    bool confiner = true;
    bool naif = false;

    void Prendre() {
        if (pris && !naif) {
            return;
        }
        fenetre->ShowMouse(false);
        fenetre->CaptureMouse(true);
        if (confiner) {
            fenetre->ClipMouseToClient(true);
        }
        pris = true;
        std::printf("[MaSalle] pointeur pris (cache, capture%s)\n", confiner ? ", confine" : "");
    }

    void Rendre() {
        if (!pris && !naif) {
            return;
        }
        fenetre->ClipMouseToClient(false);
        fenetre->CaptureMouse(false);
        fenetre->ShowMouse(true);
        pris = false;
        std::printf("[MaSalle] pointeur rendu (visible, libre)\n");
    }
};

struct Commandes {
    bool fermee = false;
};

struct Tete {
    float32 lacet = 0.f;
    float32 tangage = 0.f;
    NkVec3f position = TETE;

    NkVec3f Regard() const {
        float32 x = -std::sin(lacet) * std::cos(tangage);
        float32 y = std::sin(tangage);
        float32 z = -std::cos(lacet) * std::cos(tangage);
        return {x, y, z};
    }
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

static bool EnvActif(const char* nom) {
    const char* t = std::getenv(nom);
    return t != nullptr && t[0] != '\0' && t[0] != '0';
}

static uint64 EnvU64(const char* nom) {
    const char* t = std::getenv(nom);
    if (t == nullptr || t[0] == '\0') {
        return 0;
    }
    return (uint64)std::strtoull(t, nullptr, 10);
}

static Commandes LireCommandes(Pointeur& pointeur) {
    Commandes c;
    NkEvent* ev = nullptr;
    while (NkEvents().PollEvent(ev)) {
        if (ev->As<NkWindowCloseEvent>() != nullptr) {
            c.fermee = true;
        }
        NkKeyPressEvent* touche = ev->As<NkKeyPressEvent>();
        if (touche != nullptr) {
            NkKey k = touche->GetKey();
            if (k == NkKey::NK_ESCAPE) {
                pointeur.Rendre();
            }
            if (k == NkKey::NK_SPACE) {
                pointeur.Prendre();
            }
        }
    }
    return c;
}

static void DessinerSalle(NkRenderer* renderer, NkICommandBuffer* cmd, const Tete& tete,
                          float32 aspect, uint64 image) {
    NkRender3D* r3d = renderer->GetRender3D();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = tete.position;
    cd.target = cd.position + tete.Regard();
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
    SoumettreLaSalle(r3d, renderer->GetMeshSystem());
    NkRenderGraph* graphe = renderer->GetRenderGraph();
    if (graphe != nullptr) {
        graphe->Execute(cmd);
    }
}

static NkRenderer* CreerRendererSalle(NkIDevice* device, NkRenderer* rEcran,
                                      NkOffscreenTarget* cible, NkGraphicsApi api, uint32 w,
                                      uint32 h) {
    NkRendererConfig cfgSalle = NkRendererConfig::ForGame(api, w, h);
    cfgSalle.Disable(NK_SS_RENDER2D);
    cfgSalle.Disable(NK_SS_TEXT);
    cfgSalle.Disable(NK_SS_OVERLAY);
    cfgSalle.Enable(NK_SS_OFFSCREEN);
    NkRenderer* renderer = NkRenderer::Create(device, cfgSalle);
    if (renderer == nullptr || !renderer->Initialize()) {
        return nullptr;
    }
    NkTexHandle couleur = cible->GetColorResult();
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(couleur));
    renderer->SetRenderSizeOverride(w, h);
    return renderer;
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

    NkWindowConfig cfg;
    cfg.title = "Ma salle";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        std::printf("[MaSalle] la fenetre n'a pas pu s'ouvrir\n");
        return 1;
    }
    Pointeur pointeur;
    pointeur.fenetre = &fenetre;
    pointeur.confiner = !EnvActif("MASALLE_CAPTURE_SEULE");
    pointeur.naif = EnvActif("MASALLE_NAIF");
    pointeur.Prendre();

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    NkIDevice* device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (device == nullptr) {
        std::printf("[MaSalle] aucune API graphique disponible\n");
        pointeur.Rendre();
        return 1;
    }

    NkRendererConfig cfgEcran = NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
    NkRenderer* rEcran = NkRenderer::Create(device, cfgEcran);
    if (rEcran == nullptr || !rEcran->Initialize()) {
        std::printf("[MaSalle] le renderer d'ecran n'a pas pu s'initialiser\n");
        NkDeviceFactory::Destroy(device);
        pointeur.Rendre();
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
    NkRenderer* renderer = nullptr;
    if (cible != nullptr) {
        renderer = CreerRendererSalle(device, rEcran, cible, devInit.api, devInit.width,
                                      devInit.height);
    }
    if (renderer == nullptr) {
        std::printf("[MaSalle] la salle n'a pas pu etre preparee\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        pointeur.Rendre();
        return 1;
    }

    uint64 imageDeSortie = EnvU64("MASALLE_EXIT");
    uint64 imageDeCapture = EnvU64("MASALLE_SHOT");
    float32 aspect = (float32)devInit.width / (float32)devInit.height;
    Tete tete;

    uint64 image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        Commandes c = LireCommandes(pointeur);
        if (c.fermee) {
            break;
        }
        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            DessinerSalle(renderer, cmd, tete, aspect, image);
            NkRender2D* r2d = rEcran->GetRender2D();
            if (r2d != nullptr) {
                float32 largeur = (float32)devInit.width;
                float32 hauteur = (float32)devInit.height;
                r2d->DrawImage(cible->GetColorResult(), {0.f, 0.f, largeur, hauteur});
            }
            rEcran->Present();
            rEcran->EndFrame();
        }
        ++image;

        if (imageDeCapture != 0 && image == imageDeCapture) {
            Capturer(cible, image);
        }
        if (imageDeSortie != 0 && image >= imageDeSortie) {
            tourne = false;
        }
    }

    if (EnvActif("MASALLE_SANS_RELACHE")) {
        std::printf("[MaSalle] MASALLE_SANS_RELACHE : pointeur non rendu\n");
    } else {
        pointeur.naif = false;
        pointeur.Rendre();
    }
    device->WaitIdle();
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
