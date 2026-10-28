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
#include "NKImage/Core/NkImage.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
const float32 ECART_YEUX = 0.063f;
const uint32 LARGE_OEIL = 640;
const uint32 HAUT_OEIL = 720;
const uint64 IMAGE_CAPTURE = 30;
const int32 LIGNE_MAX = 128;

struct Oeil {
    NkOffscreenTarget* cible = nullptr;
    NkRenderer* renderer = nullptr;
    NkVec3f position;
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

static bool CreerOeil(Oeil& oeil, NkIDevice* device, NkRenderer* rEcran, NkGraphicsApi api,
                      const char* nom) {
    NkOffscreenDesc od;
    od.width = LARGE_OEIL;
    od.height = HAUT_OEIL;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = true;
    od.name = nom;
    oeil.cible = rEcran->CreateOffscreen(od);
    if (oeil.cible == nullptr) {
        return false;
    }

    NkRendererConfig cfgSalle = NkRendererConfig::ForGame(api, LARGE_OEIL, HAUT_OEIL);
    cfgSalle.Disable(NK_SS_RENDER2D);
    cfgSalle.Disable(NK_SS_TEXT);
    cfgSalle.Disable(NK_SS_OVERLAY);
    cfgSalle.Enable(NK_SS_OFFSCREEN);
    oeil.renderer = NkRenderer::Create(device, cfgSalle);
    if (oeil.renderer == nullptr || !oeil.renderer->Initialize()) {
        return false;
    }
    NkTexHandle couleur = oeil.cible->GetColorResult();
    oeil.renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(couleur));
    oeil.renderer->SetRenderSizeOverride(LARGE_OEIL, HAUT_OEIL);
    return true;
}

static void DessinerOeil(Oeil& oeil, NkICommandBuffer* cmd, uint64 image) {
    NkRender3D* r3d = oeil.renderer->GetRender3D();
    oeil.renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = oeil.renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = oeil.position;
    cd.target = oeil.position + NkVec3f(0.f, 0.f, -1.f);
    cd.up = {0.f, 1.f, 0.f};
    cd.fovY = 70.f;
    cd.aspect = (float32)LARGE_OEIL / (float32)HAUT_OEIL;
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
    SoumettreLaSalle(r3d, oeil.renderer->GetMeshSystem());
    NkRenderGraph* graphe = oeil.renderer->GetRenderGraph();
    if (graphe != nullptr) {
        graphe->Execute(cmd);
    }
}

static int32 Ecart(const math::NkColor& pa, const math::NkColor& pb) {
    int32 e = std::abs((int32)pa.r - (int32)pb.r);
    e = std::max(e, std::abs((int32)pa.g - (int32)pb.g));
    e = std::max(e, std::abs((int32)pa.b - (int32)pb.b));
    return e;
}

static void EcrireLigne(const char* ligne, bool deuxYeux) {
    char lignes[2][LIGNE_MAX] = {"", ""};
    std::FILE* lu = std::fopen("difference.txt", "r");
    if (lu != nullptr) {
        char tampon[LIGNE_MAX];
        while (std::fgets(tampon, sizeof(tampon), lu) != nullptr) {
            tampon[std::strcspn(tampon, "\r\n")] = '\0';
            if (std::strncmp(tampon, "deux yeux", 9) == 0) {
                std::snprintf(lignes[0], LIGNE_MAX, "%s", tampon);
            }
            if (std::strncmp(tampon, "vue oubliee", 11) == 0) {
                std::snprintf(lignes[1], LIGNE_MAX, "%s", tampon);
            }
        }
        std::fclose(lu);
    }
    int32 rang = 1;
    if (deuxYeux) {
        rang = 0;
    }
    std::snprintf(lignes[rang], LIGNE_MAX, "%s", ligne);
    std::FILE* ecrit = std::fopen("difference.txt", "w");
    if (ecrit == nullptr) {
        return;
    }
    for (int32 i = 0; i < 2; ++i) {
        if (lignes[i][0] != '\0') {
            std::fprintf(ecrit, "%s\n", lignes[i]);
        }
    }
    std::fclose(ecrit);
}

static bool Soustraire(const char* gauche, const char* droit, const char* sortie, bool deuxYeux) {
    NkImage a;
    NkImage b;
    if (!a.Load(gauche) || !b.Load(droit)) {
        std::printf("[MaSalle] impossible de relire %s et %s\n", gauche, droit);
        return false;
    }
    bool memeTaille = a.Width() == b.Width() && a.Height() == b.Height();
    if (!memeTaille) {
        std::printf("[MaSalle] %s et %s n'ont pas la meme taille\n", gauche, droit);
        return false;
    }
    int32 w = a.Width();
    int32 h = a.Height();
    NkImage diff = NkImage::Create((uint32)w, (uint32)h, 4, 0x000000FFu);
    long differents = 0;
    int32 ecartMax = 0;
    for (int32 y = 0; y < h; ++y) {
        for (int32 x = 0; x < w; ++x) {
            int32 e = Ecart(a.GetPixel(x, y), b.GetPixel(x, y));
            if (e > 1) {
                ++differents;
            }
            ecartMax = std::max(ecartMax, e);
            uint8 v = (uint8)std::min(255, e * 4);
            diff.SetPixel(x, y, math::NkColor(v, v, v, 255));
        }
    }
    diff.SavePNG(sortie);

    const char* etiquette = "vue oubliee";
    if (deuxYeux) {
        etiquette = "deux yeux";
    }
    char ligne[LIGNE_MAX];
    const char* format = "%s : %ld pixels sur %ld, ecart max %d";
    std::snprintf(ligne, sizeof(ligne), format, etiquette, differents, (long)w * h, ecartMax);
    std::printf("[MaSalle] %s -> %s\n", ligne, sortie);
    EcrireLigne(ligne, deuxYeux);
    return true;
}

static bool VueOubliee() {
    const char* t = std::getenv("MASALLE_VUE_OUBLIEE");
    return t != nullptr && t[0] != '\0' && t[0] != '0';
}

static uint64 ImageDeSortie() {
    const char* t = std::getenv("MASALLE_EXIT");
    if (t == nullptr || t[0] == '\0') {
        return 0;
    }
    return (uint64)std::strtoull(t, nullptr, 10);
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

int nkmain(const NkEntryState&) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    bool vueOubliee = VueOubliee();

    NkWindowConfig cfg;
    cfg.title = "Ma salle - deux yeux";
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

    Oeil gauche;
    Oeil droit;
    bool gaucheOk = CreerOeil(gauche, device, rEcran, devInit.api, "MaSalle_OeilGauche");
    bool droitOk = CreerOeil(droit, device, rEcran, devInit.api, "MaSalle_OeilDroit");
    if (!gaucheOk || !droitOk) {
        std::printf("[MaSalle] un oeil n'a pas pu etre cree\n");
        return 1;
    }

    const NkVec3f droiteTete = {1.f, 0.f, 0.f};
    float32 demiEcart = ECART_YEUX * 0.5f;
    if (vueOubliee) {
        demiEcart = 0.f;
    }
    gauche.position = TETE - droiteTete * demiEcart;
    droit.position = TETE + droiteTete * demiEcart;
    std::printf("[MaSalle] oeil gauche (%.4f, %.4f, %.4f), oeil droit (%.4f, %.4f, %.4f)\n",
                gauche.position.x, gauche.position.y, gauche.position.z, droit.position.x,
                droit.position.y, droit.position.z);

    const char* fichierG = "oeil-gauche.png";
    const char* fichierD = "oeil-droit.png";
    const char* fichierDiff = "difference.png";
    if (vueOubliee) {
        fichierG = "oubliee-gauche.png";
        fichierD = "oubliee-droit.png";
        fichierDiff = "difference-vue-oubliee.png";
    }
    uint64 imageDeSortie = ImageDeSortie();

    uint64 image = 0;
    bool capture = false;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        if (Fermee()) {
            break;
        }
        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            DessinerOeil(gauche, cmd, image);
            DessinerOeil(droit, cmd, image);
            NkRender2D* r2d = rEcran->GetRender2D();
            if (r2d != nullptr) {
                r2d->DrawImage(gauche.cible->GetColorResult(), {0.f, 0.f, 640.f, 720.f});
                r2d->DrawImage(droit.cible->GetColorResult(), {640.f, 0.f, 640.f, 720.f});
            }
            rEcran->Present();
            rEcran->EndFrame();
        }
        ++image;

        if (image == IMAGE_CAPTURE) {
            bool okG = gauche.cible->Capture(fichierG);
            bool okD = droit.cible->Capture(fichierD);
            capture = okG && okD;
            std::printf("[MaSalle] image %llu : %s (%s), %s (%s)\n", (unsigned long long)image,
                        fichierG, okG ? "ok" : "echec", fichierD, okD ? "ok" : "echec");
        }
        if (imageDeSortie != 0 && image >= imageDeSortie) {
            tourne = false;
        }
    }

    device->WaitIdle();
    NkRenderer::Destroy(gauche.renderer);
    NkRenderer::Destroy(droit.renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);

    if (!capture) {
        std::printf("[MaSalle] pas de capture : fermez la fenetre apres l'image %llu\n",
                    (unsigned long long)IMAGE_CAPTURE);
        return 0;
    }
    Soustraire(fichierG, fichierD, fichierDiff, !vueOubliee);
    return 0;
}
