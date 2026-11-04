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
#include "NKTime/NkChrono.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

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
const uint64 IMAGES_DE_DEMARRAGE = 30;
const uint32 INTERVALLES = 1000;
const int32 NB_CADENCES = 3;
const float64 CADENCES[NB_CADENCES] = {72.0, 90.0, 120.0};
const int32 TAILLE_LIGNE = 160;

struct Mesure {
    float64 moyenne = 0.0;
    float64 mediane = 0.0;
    float64 max = 0.0;
};

static void Pave(NkRender3D* r3d, NkMeshSystem* meshes, const NkVec3f& centre,
                 const NkVec3f& taille, const NkVec3f& teinte, float32 rugosite) {
    NkDrawCall3D dc;
    dc.mesh = meshes->GetCube();
    NkVec3f place = centre;
    dc.transform = NkMat4f::Translate(place) * NkMat4f::Scale(taille);
    NkVec3f moitie = taille * 0.5f;
    dc.aabb = {place - moitie, place + moitie};
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

static float64 MaintenantNs() {
    return NkChrono::Now().nanoseconds;
}

static Mesure Calculer(std::vector<float64> v) {
    Mesure m;
    if (v.empty()) {
        return m;
    }
    for (float64 x : v) {
        m.moyenne += x;
        m.max = std::max(m.max, x);
    }
    m.moyenne /= (float64)v.size();
    std::sort(v.begin(), v.end());
    size_t milieu = v.size() / 2;
    m.mediane = v[milieu];
    if (v.size() % 2 == 0) {
        m.mediane = 0.5 * (v[milieu - 1] + v[milieu]);
    }
    return m;
}

static void Budgets(char lignes[NB_CADENCES][TAILLE_LIGNE]) {
    for (int32 i = 0; i < NB_CADENCES; ++i) {
        float64 budget = 1000.0 / CADENCES[i];
        std::snprintf(lignes[i], TAILLE_LIGNE, "budget %.0f Hz : %.3f ms", CADENCES[i], budget);
        std::printf("%s\n", lignes[i]);
    }
}

static void LireLigne(const char* debut, char* ligne, int32 taille) {
    ligne[0] = '\0';
    std::FILE* f = std::fopen("budget.txt", "r");
    if (f == nullptr) {
        return;
    }
    char tampon[TAILLE_LIGNE];
    while (std::fgets(tampon, sizeof(tampon), f) != nullptr) {
        tampon[std::strcspn(tampon, "\r\n")] = '\0';
        if (std::strncmp(tampon, debut, std::strlen(debut)) == 0) {
            std::snprintf(ligne, (size_t)taille, "%s", tampon);
        }
    }
    std::fclose(f);
}

static void Ecrire(char budgets[NB_CADENCES][TAILLE_LIGNE], bool sansAttente, const Mesure& m) {
    char ecran[TAILLE_LIGNE];
    char libre[TAILLE_LIGNE];
    LireLigne("ecran", ecran, TAILLE_LIGNE);
    LireLigne("sans attente", libre, TAILLE_LIGNE);
    char* mienne = ecran;
    if (sansAttente) {
        mienne = libre;
    }
    std::snprintf(mienne, TAILLE_LIGNE, "%s : moyenne %.3f ms, mediane %.3f ms, max %.3f ms",
                  sansAttente ? "sans attente" : "ecran", m.moyenne, m.mediane, m.max);
    std::printf("%s\n", mienne);
    std::FILE* f = std::fopen("budget.txt", "w");
    if (f == nullptr) {
        return;
    }
    for (int32 i = 0; i < NB_CADENCES; ++i) {
        std::fprintf(f, "%s\n", budgets[i]);
    }
    if (ecran[0] != '\0') {
        std::fprintf(f, "%s\n", ecran);
    }
    if (libre[0] != '\0') {
        std::fprintf(f, "%s\n", libre);
    }
    std::fclose(f);
}

static bool EnvActif(const char* nom) {
    const char* t = std::getenv(nom);
    return t != nullptr && t[0] == '1';
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
    char budgets[NB_CADENCES][TAILLE_LIGNE];
    Budgets(budgets);
    bool sansAttente = EnvActif("NK_VK_MAILBOX");

    NkWindowConfig cfg;
    cfg.title = "Ma salle";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.centered = true;
    cfg.vsync = !sansAttente;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        std::printf("[MaSalle] la fenetre n'a pas pu s'ouvrir\n");
        return 1;
    }

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    if (sansAttente) {
        devInit.context.dx11.vsync = false;
        devInit.context.dx12.vsync = false;
        devInit.context.opengl.swapInterval = NkGLSwapInterval::NK_GL_SWAP_IMMEDIATE;
    }
    NkIDevice* device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (device == nullptr) {
        std::printf("[MaSalle] aucune API graphique disponible\n");
        return 1;
    }

    NkRendererConfig cfgEcran = NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
    cfgEcran.vsync = !sansAttente;
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
    std::vector<float64> intervalles;
    intervalles.reserve(INTERVALLES);
    float64 debutPrecedent = 0.0;

    uint64 image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        if (Fermee()) {
            break;
        }
        float64 debutImage = MaintenantNs();
        if (image > IMAGES_DE_DEMARRAGE && intervalles.size() < INTERVALLES) {
            intervalles.push_back((debutImage - debutPrecedent) / 1e6);
        }
        debutPrecedent = debutImage;
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

    Mesure m = Calculer(intervalles);
    std::printf("[MaSalle] %zu intervalles mesures\n", intervalles.size());
    Ecrire(budgets, sansAttente, m);
    device->WaitIdle();
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
