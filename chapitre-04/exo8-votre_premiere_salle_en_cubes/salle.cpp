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

#include <cstdio>

using namespace nkentseu;
using namespace nkentseu::renderer;

const float32 LARGEUR = 4.f;
const float32 PROFONDEUR = 4.f;
const float32 HAUTEUR = 2.5f;
const float32 EPAISSEUR = 0.10f;


static NkAABB Boite(NkVec3f centre, NkVec3f taille) {
    NkVec3f demi = {taille.x * 0.5f, taille.y * 0.5f, taille.z * 0.5f};
    return {{centre.x - demi.x, centre.y - demi.y, centre.z - demi.z},
            {centre.x + demi.x, centre.y + demi.y, centre.z + demi.z}};
}

static void SoumettreCube(NkRender3D* r3d, NkMeshHandle cube, NkVec3f centre, NkVec3f taille,
                          NkVec3f teinte, bool projetteOmbre = true) {
    NkDrawCall3D dc;
    dc.mesh = cube;
    dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
    dc.aabb = Boite(centre, taille);
    dc.tint = teinte;
    dc.roughness = 0.85f;
    dc.castShadow = projetteOmbre;
    r3d->Submit(dc);
}

static void DessinerSalle(NkRender3D* r3d, NkMeshHandle cube, NkMeshHandle plan) {
    NkVec3f mur = {0.72f, 0.70f, 0.66f};
    NkVec3f blanc = {0.92f, 0.92f, 0.90f};
    float32 demiL = LARGEUR * 0.5f;
    float32 demiP = PROFONDEUR * 0.5f;
    float32 demiE = EPAISSEUR * 0.5f;
    float32 yMur = HAUTEUR * 0.5f;
    float32 largeurTotale = LARGEUR + 2 * EPAISSEUR;
    float32 profondeurTotale = PROFONDEUR + 2 * EPAISSEUR;

    NkDrawCall3D sol;
    sol.mesh = plan;
    sol.transform = NkMat4f::Scale({LARGEUR, 1.f, PROFONDEUR});
    sol.aabb = {{-demiL, 0.f, -demiP}, {demiL, 0.f, demiP}};
    sol.tint = {0.45f, 0.37f, 0.28f};
    sol.roughness = 0.8f;
    sol.castShadow = false;
    r3d->Submit(sol);

    NkVec3f murLong = {largeurTotale, HAUTEUR, EPAISSEUR};
    NkVec3f murCote = {EPAISSEUR, HAUTEUR, PROFONDEUR};
    SoumettreCube(r3d, cube, {0.f, yMur, -(demiP + demiE)}, murLong, mur, false);
    SoumettreCube(r3d, cube, {0.f, yMur, demiP + demiE}, murLong, mur, false);
    SoumettreCube(r3d, cube, {-(demiL + demiE), yMur, 0.f}, murCote, mur, false);
    SoumettreCube(r3d, cube, {demiL + demiE, yMur, 0.f}, murCote, mur, false);
    NkVec3f plafond = {largeurTotale, EPAISSEUR, profondeurTotale};
    SoumettreCube(r3d, cube, {0.f, HAUTEUR + demiE, 0.f}, plafond, blanc, false);
}

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    (void)etat;

    NkWindowConfig config;
    config.title = "La salle";
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
    reglages.subsystems = NK_SS_RENDER3D | NK_SS_POST_PROCESS;
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
        NkKey touche = e->GetKey();
        if (touche == NkKey::NK_ESCAPE) {
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
        vue.SetPosition({1.5f, 1.70f, 1.6f});
        vue.SetTarget({-1.2f, 1.25f, -0.9f});

        NkSceneContext scene;
        scene.camera = vue;
        NkLightDesc soleil;
        soleil.type = NkLightType::NK_DIRECTIONAL;
        soleil.direction = {-0.4f, -1.f, -0.3f};
        soleil.color = {1.f, 0.95f, 0.85f};
        soleil.intensity = 3.f;
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
