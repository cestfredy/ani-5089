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
#include "NKMath/NkMat.h"
#include "NKMath/NkFunctions.h"

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
const float32 RADIANS_PAR_DEGRE = 3.14159265f / 180.f;
const float32 PROCHE = 0.05f;
const float32 LOIN = 100.f;
const int32 TAILLE_LIGNE = 160;

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

static NkMat4f Projection(float32 gauche, float32 droite, float32 haut, float32 bas,
                          float32 nearPlane, float32 farPlane) {
    const float32 tanLeft = math::NkTan(gauche);
    const float32 tanRight = math::NkTan(droite);
    const float32 tanUp = math::NkTan(haut);
    const float32 tanDown = math::NkTan(bas);
    const float32 tanWidth = tanRight - tanLeft;
    const float32 tanHeight = tanUp - tanDown;

    NkMat4f result;
    result.mat[0][0] = 2.f / tanWidth;
    result.mat[1][1] = 2.f / tanHeight;
    result.mat[2][0] = (tanRight + tanLeft) / tanWidth;
    result.mat[2][1] = (tanUp + tanDown) / tanHeight;
    result.mat[2][3] = -1.f;
    result.mat[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result.mat[3][2] = 2.f * farPlane * nearPlane / (nearPlane - farPlane);
    return result;
}

static void Imprimer(const char* nom, const NkMat4f& m) {
    std::printf("%s, ligne par ligne :\n", nom);
    for (int32 ligne = 0; ligne < 4; ++ligne) {
        std::printf("  %10.6f %10.6f %10.6f %10.6f\n", m.mat[0][ligne], m.mat[1][ligne],
                    m.mat[2][ligne], m.mat[3][ligne]);
    }
}

static float32 Comparer(std::FILE* f, const char* nom, const NkMat4f& a, const NkMat4f& b) {
    float32 plusGrand = 0.f;
    for (int32 ligne = 0; ligne < 4; ++ligne) {
        float32 e[4];
        for (int32 col = 0; col < 4; ++col) {
            e[col] = std::fabs(a.mat[col][ligne] - b.mat[col][ligne]);
            plusGrand = std::fmax(plusGrand, e[col]);
        }
        char texte[TAILLE_LIGNE];
        std::snprintf(texte, sizeof(texte), "%s, ligne %d : %g %g %g %g", nom, ligne, e[0], e[1],
                      e[2], e[3]);
        std::printf("%s\n", texte);
        if (f != nullptr) {
            std::fprintf(f, "%s\n", texte);
        }
    }
    return plusGrand;
}

static void EcrireMax(std::FILE* f, const char* nom, float32 plusGrand) {
    std::printf("%s : plus grand ecart %g\n", nom, plusGrand);
    if (f != nullptr) {
        std::fprintf(f, "%s : plus grand ecart %g\n", nom, plusGrand);
    }
}

static float32 EpreuveSymetrique(std::FILE* f) {
    const float32 rapport = 1280.f / 720.f;
    const float32 haut = 35.f * RADIANS_PAR_DEGRE;
    const float32 bas = -haut;
    const float32 droite = std::atan(rapport * std::tan(haut));
    const float32 gauche = -droite;
    std::printf("symetrique : gauche %.4f, droite %.4f, haut %.4f, bas %.4f degres\n",
                gauche / RADIANS_PAR_DEGRE, droite / RADIANS_PAR_DEGRE, haut / RADIANS_PAR_DEGRE,
                bas / RADIANS_PAR_DEGRE);
    NkMat4f main = Projection(gauche, droite, haut, bas, PROCHE, LOIN);
    NkMat4f moteur = NkMat4f::Perspective(NkAngle(70.f), rapport, PROCHE, LOIN);
    Imprimer("symetrique, a la main", main);
    Imprimer("symetrique, NkMat4f::Perspective", moteur);
    return Comparer(f, "symetrique", main, moteur);
}

static void ProjeterPoint(const char* nom, const NkMat4f& m, float32 x, float32 y) {
    const float32 z = -1.f;
    float32 cx = m.mat[0][0] * x + m.mat[1][0] * y + m.mat[2][0] * z + m.mat[3][0];
    float32 cy = m.mat[0][1] * x + m.mat[1][1] * y + m.mat[2][1] * z + m.mat[3][1];
    float32 cw = m.mat[0][3] * x + m.mat[1][3] * y + m.mat[2][3] * z + m.mat[3][3];
    std::printf("  %s : (%.4f ; %.4f ; -1) -> ndc (%.6f ; %.6f)\n", nom, x, y, cx / cw, cy / cw);
}

static void VerifierLesBords(const NkMat4f& m, float32 gauche, float32 droite, float32 haut,
                             float32 bas) {
    std::printf("quest, points projetes a un metre devant l'oeil :\n");
    ProjeterPoint("bord gauche", m, std::tan(gauche), 0.f);
    ProjeterPoint("bord droit", m, std::tan(droite), 0.f);
    ProjeterPoint("bord haut", m, 0.f, std::tan(haut));
    ProjeterPoint("bord bas", m, 0.f, std::tan(bas));
    ProjeterPoint("axe du regard", m, 0.f, 0.f);
}

static float32 EpreuveQuest(std::FILE* f) {
    const float32 gauche = -52.f * RADIANS_PAR_DEGRE;
    const float32 droite = 45.f * RADIANS_PAR_DEGRE;
    const float32 haut = 48.f * RADIANS_PAR_DEGRE;
    const float32 bas = -50.f * RADIANS_PAR_DEGRE;
    NkMat4f main = Projection(gauche, droite, haut, bas, PROCHE, LOIN);
    NkCamera3D cam;
    cam.SetNearFar(PROCHE, LOIN);
    cam.SetFovAsym(gauche, droite, haut, bas);
    const NkMat4f& moteur = cam.GetProj();
    Imprimer("quest, a la main", main);
    Imprimer("quest, NkCamera3D", moteur);
    VerifierLesBords(main, gauche, droite, haut, bas);
    return Comparer(f, "quest", main, moteur);
}

static void VerifierLesProjections() {
    std::FILE* f = std::fopen("ecarts.txt", "w");
    float32 eSym = EpreuveSymetrique(f);
    float32 eQuest = EpreuveQuest(f);
    EcrireMax(f, "symetrique", eSym);
    EcrireMax(f, "quest", eQuest);
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
    VerifierLesProjections();

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
    NkQuatf q = NkQuatf::RotateY(NkAngle(0.f));

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
