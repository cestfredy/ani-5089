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

const NkVec3f OEIL = {0.f, 1.70f, 1.5f};
const NkVec3f CIBLE = {0.f, 1.40f, -2.f};
const float32 DISTANCE = 2.f;
const float32 ARETE = 0.5f;

enum Panne { AUCUNE, SANS_LUMIERE, ECHELLE_NULLE, SANS_BOITE, CAMERA_DEDANS, NOMBRE_PANNES };
const char* NOMS_PANNES[NOMBRE_PANNES] = {"aucune", "sans_lumiere", "echelle_nulle",
                                         "sans_boite", "camera_dedans"};

static NkVec3f DevantLaCamera(float32 distance) {
    float32 dx = CIBLE.x - OEIL.x;
    float32 dy = CIBLE.y - OEIL.y;
    float32 dz = CIBLE.z - OEIL.z;
    float32 n = std::sqrt(dx * dx + dy * dy + dz * dz);
    return {OEIL.x + dx / n * distance, OEIL.y + dy / n * distance, OEIL.z + dz / n * distance};
}

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    int panne = AUCUNE;
    for (const NkString& a : etat.args) {
        for (int k = 0; k < NOMBRE_PANNES; ++k) {
            if (a == NOMS_PANNES[k]) {
                panne = k;
            }
        }
    }

    NkWindowConfig config;
    config.title = "Un cube";
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

    bool enMarche = true;
    uint32 largeur = info.width;
    uint32 hauteur = info.height;

    auto afficherPanne = [&]() {
        char t[160];
        std::snprintf(t, sizeof(t), "Un cube - panne : %s", NOMS_PANNES[panne]);
        fenetre.SetTitle(t);
        std::printf("Panne : %s\n", NOMS_PANNES[panne]);
    };
    afficherPanne();

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
        } else if (touche == NkKey::NK_SPACE) {
            panne = (panne + 1) % NOMBRE_PANNES;
            afficherPanne();
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
        if (panne == CAMERA_DEDANS) {
            vue.SetPosition(DevantLaCamera(DISTANCE));
        } else {
            vue.SetPosition(OEIL);
        }
        vue.SetTarget(CIBLE);

        NkSceneContext scene;
        scene.camera = vue;
        NkLightDesc soleil;
        soleil.type = NkLightType::NK_DIRECTIONAL;
        soleil.direction = {-0.4f, -1.f, -0.3f};
        soleil.color = {1.f, 0.95f, 0.85f};
        soleil.intensity = 3.f;
        if (panne == SANS_LUMIERE) {
            soleil.intensity = 0.f;
        }
        scene.lights.PushBack(soleil);
        rendu3d->BeginScene(scene);

        NkVec3f centre = DevantLaCamera(DISTANCE);
        float32 arete = ARETE;
        if (panne == ECHELLE_NULLE) {
            arete = 0.f;
        }
        NkDrawCall3D leCube;
        leCube.mesh = cube;
        leCube.transform = NkMat4f::Translate(centre) * NkMat4f::Scale({arete, arete, arete});
        leCube.tint = {1.f, 1.f, 1.f};
        if (panne != SANS_BOITE) {
            float32 demi = ARETE * 0.5f;
            leCube.aabb = {{centre.x - demi, centre.y - demi, centre.z - demi},
                           {centre.x + demi, centre.y + demi, centre.z + demi}};
        }
        rendu3d->Submit(leCube);

        rendu->Present();
        rendu->EndFrame();
    }

    peripherique->WaitIdle();
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(peripherique);
    fenetre.Close();
    return 0;
}
