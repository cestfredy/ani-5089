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
#include "NKMath/NkQuat.h"
#include "NKMath/NkAngle.h"

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
const float32 ECART_PUPILLES = 0.063f;
const float32 MILLIMETRES_PAR_METRE = 1000.f;
const int32 NB_POSES = 3;

struct Pose {
    const char* nom;
    NkQuatf orientation;
};

struct Yeux {
    NkVec3f gauche;
    NkVec3f droit;
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

static Yeux YeuxLeLongDe(const NkVec3f& tete, const NkVec3f& droite) {
    float32 demiEcart = ECART_PUPILLES * 0.5f;
    Yeux y;
    y.gauche = tete - droite * demiEcart;
    y.droit = tete + droite * demiEcart;
    return y;
}

static void EcrireYeux(std::FILE* f, const char* pose, const char* methode, const Yeux& y) {
    char ligne[160];
    std::snprintf(ligne, sizeof(ligne), "%s, %s : G (%.4f, %.4f, %.4f) D (%.4f, %.4f, %.4f)", pose,
                  methode, (double)y.gauche.x, (double)y.gauche.y, (double)y.gauche.z,
                  (double)y.droit.x, (double)y.droit.y, (double)y.droit.z);
    std::printf("%s\n", ligne);
    if (f != nullptr) {
        std::fprintf(f, "%s\n", ligne);
    }
}

static float32 Distance(const NkVec3f& a, const NkVec3f& b) {
    NkVec3f d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

static void CalculerLesPoses() {
    const Pose poses[NB_POSES] = {
        {"neutre", NkQuatf::Identity()},
        {"roulis 30", NkQuatf::RotateZ(NkAngle(30.f))},
        {"lacet 90", NkQuatf::RotateY(NkAngle(90.f))},
    };
    const NkVec3f axeXMonde = {1.f, 0.f, 0.f};
    std::FILE* f = std::fopen("poses.txt", "w");
    float32 ecarts[NB_POSES] = {};
    for (int32 i = 0; i < NB_POSES; ++i) {
        NkVec3f droiteTete = poses[i].orientation * NkVec3f(1.f, 0.f, 0.f);
        Yeux parTete = YeuxLeLongDe(TETE, droiteTete);
        Yeux parMonde = YeuxLeLongDe(TETE, axeXMonde);
        EcrireYeux(f, poses[i].nom, "tete", parTete);
        EcrireYeux(f, poses[i].nom, "monde", parMonde);
        ecarts[i] = Distance(parTete.gauche, parMonde.gauche) * MILLIMETRES_PAR_METRE;
    }
    for (int32 i = 0; i < NB_POSES; ++i) {
        std::printf("%s : ecart %.1f mm\n", poses[i].nom, (double)ecarts[i]);
        if (f != nullptr) {
            std::fprintf(f, "%s : ecart %.1f mm\n", poses[i].nom, (double)ecarts[i]);
        }
    }
    if (f != nullptr) {
        std::fclose(f);
    }
}

static bool Fermee() {
    NkEvent* ev = nullptr;
    bool fermee = false;
    while (NkEvents().PollEvent(ev)) {
        if (ev->As<NkWindowCloseEvent>() != nullptr) {
            fermee = true;
        }
    }
    return fermee;
}

static uint64 EnvU64(const char* nom) {
    const char* t = std::getenv(nom);
    if (t == nullptr || t[0] == '\0') {
        return 0;
    }
    return (uint64)std::strtoull(t, nullptr, 10);
}

static void DessinerSalle(NkRenderer* renderer, NkICommandBuffer* cmd, const NkQuatf& q,
                          float32 aspect, uint64 image) {
    NkRender3D* r3d = renderer->GetRender3D();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = TETE;
    cd.target = cd.position + q * NkVec3f(0.f, 0.f, -1.f);
    cd.up = q * NkVec3f(0.f, 1.f, 0.f);
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
    CalculerLesPoses();

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

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    NkIDevice* device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (device == nullptr) {
        std::printf("[MaSalle] aucune API graphique disponible\n");
        return 1;
    }

    NkRendererConfig cfgEcran = NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
    NkRenderer* rEcran = NkRenderer::Create(device, cfgEcran);
    if (rEcran == nullptr || !rEcran->Initialize()) {
        std::printf("[MaSalle] le renderer d'ecran n'a pas pu s'initialiser\n");
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
    NkRenderer* renderer = nullptr;
    if (cible != nullptr) {
        renderer = CreerRendererSalle(device, rEcran, cible, devInit.api, devInit.width,
                                      devInit.height);
    }
    if (renderer == nullptr) {
        std::printf("[MaSalle] la salle n'a pas pu etre preparee\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }

    uint64 imageDeSortie = EnvU64("MASALLE_EXIT");
    uint64 imageDeCapture = EnvU64("MASALLE_SHOT");
    float32 aspect = (float32)devInit.width / (float32)devInit.height;
    NkQuatf q = NkQuatf::Identity();

    uint64 image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        if (Fermee()) {
            break;
        }
        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            DessinerSalle(renderer, cmd, q, aspect, image);
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

    device->WaitIdle();
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
