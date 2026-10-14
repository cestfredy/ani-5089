#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKEvent/NkKeyboardEvent.h"
#include "NKLogger/NkLog.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Core/NkCamera.h"
#include "NKRenderer/Mesh/NkMeshSystem.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"

#include <cmath>
#include <cstdio>

using namespace nkentseu;
using namespace nkentseu::renderer;

const float32 DESSUS_TABLE = 0.75f;

NkMat4f PoserAuSol(NkVec3f taille, float x, float z) {
    return NkMat4f::Translate({x, taille.y * 0.5f, z}) * NkMat4f::Scale(taille);
}

NkMat4f PoserSurTable(NkVec3f taille, float x, float z) {
    return NkMat4f::Translate({x, DESSUS_TABLE + taille.y * 0.5f, z}) * NkMat4f::Scale(taille);
}

const float32 L = 4.f;
const float32 P = 4.f;
const float32 H = 2.5f;
const float32 E = 0.10f;
const float32 DECALAGE = 0.001f;

static NkAABB BoitePosee(NkVec3f taille, float x, float z, float appui) {
    return {{x - taille.x * 0.5f, appui, z - taille.z * 0.5f},
            {x + taille.x * 0.5f, appui + taille.y, z + taille.z * 0.5f}};
}

static void Soumettre(NkRender3D* r3d, NkMeshHandle mesh, const NkMat4f& transformation,
                      const NkAABB& boite, NkVec3f teinte, bool projetteOmbre = true) {
    NkDrawCall3D dc;
    dc.mesh = mesh;
    dc.transform = transformation;
    dc.aabb = boite;
    dc.tint = teinte;
    dc.roughness = 0.85f;
    dc.castShadow = projetteOmbre;
    r3d->Submit(dc);
}

static void AuSol(NkRender3D* r3d, NkMeshHandle cube, NkVec3f taille, float x, float z,
                  NkVec3f teinte, bool projetteOmbre = true) {
    NkAABB boite = BoitePosee(taille, x, z, 0.f);
    Soumettre(r3d, cube, PoserAuSol(taille, x, z), boite, teinte, projetteOmbre);
}

static void SurTable(NkRender3D* r3d, NkMeshHandle cube, NkVec3f taille, float x, float z,
                     NkVec3f teinte) {
    NkAABB boite = BoitePosee(taille, x, z, DESSUS_TABLE);
    Soumettre(r3d, cube, PoserSurTable(taille, x, z), boite, teinte);
}

static bool Verifier(const char* nom, const NkMat4f& m, NkVec3f centre, NkVec3f taille) {
    float32 tol = 1e-5f;
    bool centreOk = std::fabs(m.data[12] - centre.x) < tol && std::fabs(m.data[13] - centre.y) < tol
                    && std::fabs(m.data[14] - centre.z) < tol;
    bool tailleOk = std::fabs(m.data[0] - taille.x) < tol && std::fabs(m.data[5] - taille.y) < tol
                    && std::fabs(m.data[10] - taille.z) < tol;
    bool ok = centreOk && tailleOk;
    std::printf("verification %s : centre (%.3f, %.3f, %.3f) taille (%.3f, %.3f, %.3f) -> %s\n",
                nom, m.data[12], m.data[13], m.data[14], m.data[0], m.data[5], m.data[10],
                ok ? "OK" : "FAUX");
    return ok;
}

static void DessinerSalle(NkRender3D* r3d, NkMeshHandle cube, NkMeshHandle plan) {
    NkVec3f mur = {0.72f, 0.70f, 0.66f};
    NkVec3f bois = {0.55f, 0.36f, 0.20f};
    NkVec3f porte = {0.42f, 0.26f, 0.14f};
    NkVec3f verre = {0.55f, 0.75f, 0.95f};
    NkVec3f blanc = {0.92f, 0.92f, 0.90f};

    NkAABB boiteSol = {{-L / 2, 0.f, -P / 2}, {L / 2, 0.f, P / 2}};
    Soumettre(r3d, plan, NkMat4f::Scale({L, 1.f, P}), boiteSol, {0.45f, 0.37f, 0.28f}, false);

    AuSol(r3d, cube, {L + 2 * E, H, E}, 0.f, -(P + E) / 2, mur, false);
    AuSol(r3d, cube, {L + 2 * E, H, E}, 0.f, (P + E) / 2, mur, false);
    AuSol(r3d, cube, {E, H, P}, -(L + E) / 2, 0.f, mur, false);
    AuSol(r3d, cube, {E, H, P}, (L + E) / 2, 0.f, mur, false);
    AuSol(r3d, cube, {0.90f, 2.00f, 0.02f}, -0.70f, -P / 2 - 0.01f + DECALAGE, porte, false);

    float32 hPied = DESSUS_TABLE - 0.05f;
    for (float32 x : {0.60f - 0.57f, 0.60f + 0.57f}) {
        for (float32 z : {0.70f - 0.37f, 0.70f + 0.37f}) {
            AuSol(r3d, cube, {0.06f, hPied, 0.06f}, x, z, bois);
        }
    }
    NkMat4f plateau = NkMat4f::Translate({0.60f, hPied + 0.025f, 0.70f})
                      * NkMat4f::Scale({1.20f, 0.05f, 0.80f});
    Soumettre(r3d, cube, plateau, {{0.f, hPied, 0.30f}, {1.20f, DESSUS_TABLE, 1.10f}}, bois);

    SurTable(r3d, cube, {0.30f, 0.04f, 0.22f}, 0.35f, 0.75f, {0.70f, 0.15f, 0.12f});
    SurTable(r3d, cube, {0.10f, 0.18f, 0.12f}, 0.85f, 0.55f, {0.15f, 0.35f, 0.65f});

    float32 xFenetre = -L / 2 - 0.01f + DECALAGE;
    for (float32 z : {-0.90f, 0.90f}) {
        NkMat4f fenetre = NkMat4f::Translate({xFenetre, 1.40f, z})
                          * NkMat4f::Scale({0.02f, 1.00f, 1.20f});
        NkAABB boite = {{xFenetre - 0.01f, 0.90f, z - 0.60f}, {xFenetre + 0.01f, 1.90f, z + 0.60f}};
        Soumettre(r3d, cube, fenetre, boite, verre, false);
    }
    NkMat4f plafond = NkMat4f::Translate({0.f, H + E / 2, 0.f})
                      * NkMat4f::Scale({L + 2 * E, E, P + 2 * E});
    NkAABB boitePlafond = {{-L / 2 - E, H, -P / 2 - E}, {L / 2 + E, H + E, P / 2 + E}};
    Soumettre(r3d, cube, plafond, boitePlafond, blanc, false);
}

int nkmain(const NkEntryState& etat) {
    (void)etat;
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    Verifier("PoserAuSol, mur du fond", PoserAuSol({4.20f, 2.50f, 0.10f}, 0.f, -2.05f),
             {0.f, 1.25f, -2.05f}, {4.20f, 2.50f, 0.10f});
    Verifier("PoserAuSol, pave plat", PoserAuSol({0.50f, 0.03f, 0.20f}, 1.f, 1.f),
             {1.f, 0.015f, 1.f}, {0.50f, 0.03f, 0.20f});
    Verifier("PoserSurTable, pave plat", PoserSurTable({0.50f, 0.03f, 0.20f}, 0.6f, 0.7f),
             {0.6f, 0.765f, 0.7f}, {0.50f, 0.03f, 0.20f});

    NkWindowConfig config;
    config.title = "La salle posee";
    config.width = 1280;
    config.height = 720;
    NkWindow fenetre(config);
    if (!fenetre.IsValid()) {
        return 1;
    }

    NkDeviceInitInfo info{};
    info.surface = fenetre.GetSurfaceDesc();
    info.width = fenetre.GetSize().width;
    info.height = fenetre.GetSize().height;
    NkIDevice* peripherique = NkDeviceFactory::CreateAutoDetect(info);
    if (!peripherique || !peripherique->IsValid()) {
        std::printf("Le peripherique ne s'est pas cree.\n");
        return 2;
    }
    const char* interfaceGraphique = NkGraphicsApiName(peripherique->GetApi());
    std::printf("Interface graphique : %s\n", interfaceGraphique);
    logger.Info("Interface graphique : {0}", interfaceGraphique);

    NkRendererConfig reglages =
        NkRendererConfig::ForGame(peripherique->GetApi(), info.width, info.height);
    reglages.subsystems = NK_SS_RENDER3D | NK_SS_SHADOW | NK_SS_POST_PROCESS;
    NkRenderer* rendu = NkRenderer::Create(peripherique, reglages);
    if (!rendu || !rendu->Initialize()) {
        std::printf("Le renderer ne s'initialise pas.\n");
        NkRenderer::Destroy(rendu);
        NkDeviceFactory::Destroy(peripherique);
        return 3;
    }
    NkRender3D* rendu3d = rendu->GetRender3D();
    NkMeshHandle cube = rendu->GetMeshSystem()->GetCube();
    NkMeshHandle plan = rendu->GetMeshSystem()->GetPlane();

    bool enMarche = true;
    uint32 largeur = info.width;
    uint32 hauteur = info.height;

    NkEventSystem& evenements = NkEvents();
    evenements.AddEventCallback<NkWindowCloseEvent>([&](NkWindowCloseEvent*) {
        enMarche = false;
    });
    evenements.AddEventCallback<NkWindowResizeEvent>([&](NkWindowResizeEvent* e) {
        uint32 w = (uint32)e->GetWidth();
        uint32 h = (uint32)e->GetHeight();
        bool change = w != largeur || h != hauteur;
        if (w > 0 && h > 0 && change) {
            largeur = w;
            hauteur = h;
            rendu->OnResize(w, h);
        }
    });
    evenements.AddEventCallback<NkKeyPressEvent>([&](NkKeyPressEvent* e) {
        if (e->GetKey() == NkKey::NK_ESCAPE) {
            enMarche = false;
        }
    });

    while (enMarche && fenetre.IsOpen()) {
        evenements.PollEvents();
        if (!enMarche) {
            break;
        }
        if (!rendu->BeginFrame()) {
            continue;
        }

        NkCamera3DData camera;
        camera.up = {0.f, 1.f, 0.f};
        camera.fovY = 70.f;
        camera.aspect = (float32)largeur / (float32)hauteur;
        camera.nearPlane = 0.05f;
        camera.farPlane = 200.f;
        NkCamera3D vue(camera);
        vue.SetPosition({-1.4f, 1.70f, 1.8f});
        vue.SetTarget({0.4f, 0.6f, -0.2f});

        NkSceneContext scene;
        scene.camera = vue;
        NkLightDesc soleil;
        soleil.type = NkLightType::NK_DIRECTIONAL;
        soleil.direction = {-0.4f, -1.f, -0.3f};
        soleil.color = {1.f, 0.95f, 0.85f};
        soleil.intensity = 3.f;
        soleil.castShadow = true;
        scene.lights.PushBack(soleil);

        rendu3d->BeginScene(scene);
        DessinerSalle(rendu3d, cube, plan);
        rendu->Present();
        rendu->EndFrame();
    }

    peripherique->WaitIdle();
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(peripherique);
    fenetre.Close();
    return 0;
}
