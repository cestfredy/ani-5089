#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKEvent/NkKeyboardEvent.h"
#include "NKEvent/NkMouseEvent.h"

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
#include "NKTime/NkChrono.h"
#include "NKMath/NkMat.h"
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
const float32 SENSIBILITE = 0.0022f;
const float32 RADIANS_PAR_DEGRE = 3.14159265f / 180.f;
const float32 TANGAGE_MAX = 89.f * RADIANS_PAR_DEGRE;
const uint32 IMAGES_MESUREES = 1000;

struct Pointeur {
    NkWindow* fenetre = nullptr;
    bool pris = false;

    void Prendre() {
        if (pris) {
            return;
        }
        fenetre->ShowMouse(false);
        fenetre->CaptureMouse(true);
        fenetre->ClipMouseToClient(true);
        pris = true;
    }

    void Rendre() {
        if (!pris) {
            return;
        }
        fenetre->ClipMouseToClient(false);
        fenetre->CaptureMouse(false);
        fenetre->ShowMouse(true);
        pris = false;
    }
};

struct Commandes {
    bool fermee = false;
};

struct Accumulateur {
    int32 dx = 0;
    int32 dy = 0;

    void Ajouter(int32 x, int32 y) {
        dx += x;
        dy += y;
    }

    void LireEtVider(int32& x, int32& y) {
        x = dx;
        y = dy;
        dx = 0;
        dy = 0;
    }
};

struct Tete {
    float32 lacet = 0.f;
    float32 tangage = 0.f;
    NkVec3f position = TETE;

    NkQuatf Orientation() const {
        const NkMat4f rotation = NkMat4f::RotationY(NkAngle::FromRad(lacet)) *
                                 NkMat4f::RotationX(NkAngle::FromRad(tangage));
        const NkQuatf orientation = NkQuatf(rotation).Normalized();
        return orientation;
    }
};

struct Erreurs {
    float64 somme = 0.0;
    float64 max = 0.0;
    uint32 images = 0;

    void Ajouter(float64 deg) {
        somme += deg;
        max = std::fmax(max, deg);
        ++images;
    }

    float64 Moyenne() const {
        if (images == 0) {
            return 0.0;
        }
        return somme / (float64)images;
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

static NkVec3f VitesseAngulaire(const NkQuatf& from, const NkQuatf& to, float32 dtSeconds) {
    if (dtSeconds <= 0.f) {
        return NkVec3f(0.f, 0.f, 0.f);
    }
    NkQuatf delta = (to * from.Conjugate()).Normalized();
    if (delta.w < 0.f) {
        delta = NkQuatf(-delta.x, -delta.y, -delta.z, -delta.w);
    }
    const float32 angleRad = delta.Angle().Rad();
    if (angleRad <= 1e-6f) {
        return NkVec3f(0.f, 0.f, 0.f);
    }
    return delta.Axis() * (angleRad / dtSeconds);
}

static NkQuatf Extrapoler(const NkQuatf& orientation, const NkVec3f& vitesse, float32 dtSeconds) {
    const float32 speedRad = vitesse.Len();
    if (speedRad > 1e-6f) {
        const NkVec3f axis = vitesse * (1.f / speedRad);
        const NkQuatf delta(NkAngle::FromRad(speedRad * dtSeconds), axis);
        return (delta * orientation).Normalized();
    }
    return orientation;
}

static float64 AngleDeg(const NkQuatf& a, const NkQuatf& b) {
    return (b * a.Conjugate()).Normalized().Angle().Deg();
}

static float64 FrequenceEcran() {
    const char* t = std::getenv("MASALLE_HZ");
    if (t == nullptr || t[0] == '\0') {
        return 60.0;
    }
    return std::atof(t);
}

static void Ecrire(const Erreurs& avec, const Erreurs& sans) {
    const char* format = "%s prediction : moyenne %.4f deg, max %.4f deg, sur %u images\n";
    std::printf(format, "avec", avec.Moyenne(), avec.max, avec.images);
    std::printf(format, "sans", sans.Moyenne(), sans.max, sans.images);
    std::FILE* f = std::fopen("erreurs.txt", "w");
    if (f == nullptr) {
        return;
    }
    std::fprintf(f, format, "avec", avec.Moyenne(), avec.max, avec.images);
    std::fprintf(f, format, "sans", sans.Moyenne(), sans.max, sans.images);
    std::fclose(f);
}

static uint64 EnvU64(const char* nom) {
    const char* t = std::getenv(nom);
    if (t == nullptr || t[0] == '\0') {
        return 0;
    }
    return (uint64)std::strtoull(t, nullptr, 10);
}

static Commandes LireCommandes(Pointeur& pointeur, Accumulateur& souris) {
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
        NkMouseRawEvent* brut = ev->As<NkMouseRawEvent>();
        if (brut != nullptr) {
            souris.Ajouter(brut->GetDeltaX(), brut->GetDeltaY());
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
    cd.target = cd.position + tete.Orientation() * NkVec3f(0.f, 0.f, -1.f);
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

    Pointeur pointeur;
    pointeur.fenetre = &fenetre;
    pointeur.Prendre();
    uint64 imageDeSortie = EnvU64("MASALLE_EXIT");
    uint64 imageDeCapture = EnvU64("MASALLE_SHOT");
    float32 aspect = (float32)devInit.width / (float32)devInit.height;
    Tete tete;
    Accumulateur souris;
    float64 hz = FrequenceEcran();
    float32 periode = (float32)(1.0 / hz);
    std::printf("[MaSalle] ecran %.3f Hz, prediction a %.3f ms\n", hz, periode * 1000.0);
    Erreurs avec;
    Erreurs sans;
    uint32 immobiles = 0;
    NkQuatf precedente = tete.Orientation();
    NkQuatf predite = precedente;
    float64 instantPrecedent = NkChrono::Now().ToMilliseconds();

    uint64 image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        Commandes c = LireCommandes(pointeur, souris);
        if (c.fermee) {
            break;
        }
        int32 dx = 0;
        int32 dy = 0;
        souris.LireEtVider(dx, dy);
        tete.lacet -= (float32)dx * SENSIBILITE;
        tete.tangage -= (float32)dy * SENSIBILITE;
        tete.tangage = std::fmax(-TANGAGE_MAX, std::fmin(TANGAGE_MAX, tete.tangage));

        float64 instant = NkChrono::Now().ToMilliseconds();
        NkQuatf mesuree = tete.Orientation();
        if (image > 0) {
            avec.Ajouter(AngleDeg(predite, mesuree));
            sans.Ajouter(AngleDeg(precedente, mesuree));
            if (AngleDeg(predite, mesuree) > 5.0) {
                std::printf("PIC image %llu dt %.3f ms dx %d dy %d avec %.2f sans %.2f\n",
                            (unsigned long long)image, instant - instantPrecedent, dx, dy,
                            AngleDeg(predite, mesuree), AngleDeg(precedente, mesuree));
            }
            if (dx == 0 && dy == 0) {
                ++immobiles;
            }
        }
        float32 dt = (float32)((instant - instantPrecedent) / 1000.0);
        NkVec3f vitesse = VitesseAngulaire(precedente, mesuree, dt);
        predite = Extrapoler(mesuree, vitesse, periode);
        precedente = mesuree;
        instantPrecedent = instant;
        if (avec.images >= IMAGES_MESUREES) {
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

    pointeur.Rendre();
    Ecrire(avec, sans);
    std::printf("images ou la souris n'a pas bouge : %u sur %u\n", immobiles, avec.images);
    device->WaitIdle();
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
