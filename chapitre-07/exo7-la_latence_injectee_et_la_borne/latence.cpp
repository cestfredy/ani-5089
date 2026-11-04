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
const double kPi = 3.14159265358979323846;
const double CADENCE_HZ = 72.0;
const double PERIODE = 1.0 / CADENCE_HZ;
const double DEBUT_GESTE = 1.0;
const double DUREE_SIMULEE = 3.0;
const double DEBUT_MESURE = 1.0;
const double FIN_MESURE = 1.9;
const double MILIEU_GESTE = 1.2;
const double LATENCE_LONGUE = 1.0;
const float32 BORNE = 0.1f;
const uint32 kMaxSamples = 64;
const int32 NB_LATENCES = 5;
const double LATENCES_MS[NB_LATENCES] = {0.0, 10.0, 20.0, 40.0, 80.0};
const double DEGRES_PAR_RADIAN = 180.0 / kPi;

struct PoseDatee {
    double time = 0.0;
    NkQuatf pose;
    NkVec3f velocity;
};

struct Historique {
    PoseDatee mSamples[kMaxSamples];
    uint32 mSampleHead = 0;
    uint32 mSampleCount = 0;

    void Ajouter(const PoseDatee& p) {
        if (mSampleCount > 0u) {
            mSampleHead = (mSampleHead + 1u) % kMaxSamples;
        }
        mSamples[mSampleHead] = p;
        if (mSampleCount < kMaxSamples) {
            ++mSampleCount;
        }
    }

    const PoseDatee& SampleAt(double now, double latence) const {
        if (latence <= 0.0 || mSampleCount <= 1u) {
            return mSamples[mSampleHead];
        }
        const double target = now - latence;
        uint32 index = mSampleHead;
        for (uint32 i = 0u; i < mSampleCount; ++i) {
            if (mSamples[index].time <= target) {
                return mSamples[index];
            }
            index = (index + kMaxSamples - 1u) % kMaxSamples;
        }
        return mSamples[(mSampleHead + kMaxSamples - (mSampleCount - 1u)) % kMaxSamples];
    }
};

struct Resultat {
    double moyenneAvec = 0.0;
    double maxAvec = 0.0;
    double moyenneSans = 0.0;
    double maxSans = 0.0;
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

static double Lacet(double t) {
    const double amplitude = 90.0;
    const double duree = 0.400;
    if (t <= 0.0) {
        return 0.0;
    }
    if (t >= duree) {
        return amplitude;
    }
    return amplitude * 0.5 * (1.0 - std::cos(kPi * t / duree));
}

static NkQuatf Pose(double t) {
    return NkQuatf::RotateY(NkAngle((float32)Lacet(t - DEBUT_GESTE)));
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

static double AngleDeg(const NkQuatf& a, const NkQuatf& b) {
    return (b * a.Conjugate()).Normalized().Angle().Deg();
}

static NkQuatf PredictAt(const PoseDatee& p, double when, bool borne) {
    float32 dt = (float32)(when - p.time);
    if (borne) {
        dt = std::fmax(-BORNE, std::fmin(BORNE, dt));
    }
    return Extrapoler(p.pose, p.velocity, dt);
}

static double LacetDe(const NkQuatf& q) {
    return 2.0 * std::atan2((double)q.y, (double)q.w) * DEGRES_PAR_RADIAN;
}

static Resultat Simuler(double latence) {
    Historique h;
    Resultat r;
    int32 mesures = 0;
    NkQuatf precedente = Pose(0.0);
    int32 nbPoses = (int32)(DUREE_SIMULEE * CADENCE_HZ);
    for (int32 k = 0; k <= nbPoses; ++k) {
        double now = k * PERIODE;
        PoseDatee p;
        p.time = now;
        p.pose = Pose(now);
        p.velocity = VitesseAngulaire(precedente, p.pose, (float32)PERIODE);
        if (k == 0) {
            p.velocity = NkVec3f(0.f, 0.f, 0.f);
        }
        h.Ajouter(p);
        precedente = p.pose;
        bool pendant = now >= DEBUT_MESURE - 1e-9 && now <= FIN_MESURE + 1e-9;
        if (!pendant) {
            continue;
        }
        const PoseDatee& e = h.SampleAt(now, latence);
        NkQuatf vraie = Pose(now + PERIODE);
        double avec = AngleDeg(PredictAt(e, now + PERIODE, true), vraie);
        double sans = AngleDeg(e.pose, vraie);
        r.moyenneAvec += avec;
        r.moyenneSans += sans;
        r.maxAvec = std::fmax(r.maxAvec, avec);
        r.maxSans = std::fmax(r.maxSans, sans);
        ++mesures;
    }
    r.moyenneAvec /= mesures;
    r.moyenneSans /= mesures;
    return r;
}

static void Borne(std::FILE* f, bool borne) {
    Historique h;
    NkQuatf precedente = Pose(0.0);
    int32 nbPoses = (int32)(DUREE_SIMULEE * CADENCE_HZ);
    double meilleurEcart = 1e9;
    PoseDatee choisi;
    double nowChoisi = 0.0;
    for (int32 k = 0; k <= nbPoses; ++k) {
        double now = k * PERIODE;
        PoseDatee p;
        p.time = now;
        p.pose = Pose(now);
        p.velocity = VitesseAngulaire(precedente, p.pose, (float32)PERIODE);
        if (k == 0) {
            p.velocity = NkVec3f(0.f, 0.f, 0.f);
        }
        h.Ajouter(p);
        precedente = p.pose;
        const PoseDatee& e = h.SampleAt(now, LATENCE_LONGUE);
        double ecart = std::fabs(e.time - MILIEU_GESTE);
        if (ecart < meilleurEcart) {
            meilleurEcart = ecart;
            choisi = e;
            nowChoisi = now;
        }
    }
    double when = nowChoisi + PERIODE;
    double horizon = when - choisi.time;
    if (borne) {
        horizon = std::fmax(-(double)BORNE, std::fmin((double)BORNE, horizon));
    }
    double predit = LacetDe(PredictAt(choisi, when, borne));
    double vrai = LacetDe(Pose(when));
    char ligne[160];
    std::snprintf(ligne, sizeof(ligne),
                  "%s : echantillon a %.3f s, horizon %.1f ms, predit %.2f deg, vrai %.2f deg",
                  borne ? "avec borne" : "sans borne", choisi.time, horizon * 1000.0, predit, vrai);
    std::printf("%s  (image a %.3f s, vitesse %.1f deg/s)\n", ligne, nowChoisi,
                choisi.velocity.Len() * DEGRES_PAR_RADIAN);
    if (f != nullptr) {
        std::fprintf(f, "%s\n", ligne);
    }
}

static void MesurerLesLatences() {
    std::FILE* f = std::fopen("latences.txt", "w");
    for (int32 i = 0; i < NB_LATENCES; ++i) {
        Resultat r = Simuler(LATENCES_MS[i] / 1000.0);
        char ligne[160];
        std::snprintf(ligne, sizeof(ligne),
                      "latence %.0f ms : avec %.3f deg (max %.3f), sans %.3f deg (max %.3f)",
                      LATENCES_MS[i], r.moyenneAvec, r.maxAvec, r.moyenneSans, r.maxSans);
        std::printf("%s\n", ligne);
        if (f != nullptr) {
            std::fprintf(f, "%s\n", ligne);
        }
    }
    Borne(f, false);
    Borne(f, true);
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
    MesurerLesLatences();

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
