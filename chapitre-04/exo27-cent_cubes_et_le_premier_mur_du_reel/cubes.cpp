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

#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace nkentseu;
using namespace nkentseu::renderer;

const float32 LARGEUR = 4.f;
const float32 PROFONDEUR = 4.f;
const float32 HAUTEUR = 2.5f;
const float32 EPAISSEUR = 0.10f;

const float32 COTE = 0.08f;
const float32 PAS = 0.12f;
const int PAR_COTE = 30;
const int IMAGES_PAR_MESURE = 400;

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

static NkVec3f PlaceDuCube(int i) {
    int parEtage = PAR_COTE * PAR_COTE;
    int etage = i / parEtage;
    int reste = i % parEtage;
    float32 x = -1.74f + (reste % PAR_COTE) * PAS;
    float32 z = -1.74f + (reste / PAR_COTE) * PAS;
    return {x, COTE * 0.5f + etage * PAS, z};
}

static void CubesUnParUn(NkRender3D* r3d, NkMeshHandle cube, int nombre) {
    for (int i = 0; i < nombre; ++i) {
        SoumettreCube(r3d, cube, PlaceDuCube(i), {COTE, COTE, COTE}, {0.85f, 0.45f, 0.20f});
    }
}

static void CubesGroupes(NkRender3D* r3d, NkMeshHandle cube, int nombre) {
    NkDrawCallInstanced groupe;
    groupe.mesh = cube;
    for (int i = 0; i < nombre; ++i) {
        NkVec3f c = PlaceDuCube(i);
        groupe.transforms.PushBack(NkMat4f::Translate(c) * NkMat4f::Scale({COTE, COTE, COTE}));
        groupe.tints.PushBack({0.85f, 0.45f, 0.20f});
        NkAABB b = Boite(c, {COTE, COTE, COTE});
        if (i == 0) {
            groupe.aabb = b;
        } else {
            groupe.aabb.Merge(b);
        }
    }
    r3d->SubmitInstanced(groupe);
}

static void NoterMesure(int nombre, bool groupes, uint32 ordres, double cadence) {
    const char* mode = "un par un";
    if (groupes) {
        mode = "groupes";
    }
    std::printf("%d cubes, %s : drawCalls = %u, cadence = %.1f images/s (sur %d images)\n",
                nombre, mode, ordres, cadence, IMAGES_PAR_MESURE);
    std::FILE* f = std::fopen("cadence_mesures.txt", "a");
    if (!f) {
        return;
    }
    std::fprintf(f, "%d cubes, %s : drawCalls = %u, cadence = %.1f images/s\n",
                 nombre, mode, ordres, cadence);
    std::fclose(f);
}

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    int nombre = 100;
    bool groupes = false;
    bool automatique = false;
    for (const NkString& a : etat.args) {
        int n = std::atoi(a.CStr());
        if (n > 0) {
            nombre = n;
        }
        if (a == "groupes") {
            groupes = true;
        }
        if (a == "auto") {
            automatique = true;
        }
    }

    NkWindowConfig config;
    config.title = "Cent cubes";
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
    info.context.dx11.vsync = false;
    info.context.dx12.vsync = false;
    info.context.vulkan.vsync = false;
    info.context.opengl.swapInterval = NkGLSwapInterval::NK_GL_SWAP_IMMEDIATE;
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
    reglages.vsync = false;
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
    using Horloge = std::chrono::steady_clock;
    int images = 0;
    int mesures = 0;
    Horloge::time_point debutMesure = Horloge::now();

    auto changer = [&](int n) {
        nombre = n;
        if (nombre < 1) {
            nombre = 1;
        }
        images = 0;
        debutMesure = Horloge::now();
        std::printf("%d cubes, %s\n", nombre, groupes ? "groupes" : "un par un");
    };

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
        } else if (touche == NkKey::NK_UP) {
            changer(nombre * 2);
        } else if (touche == NkKey::NK_DOWN) {
            changer(nombre / 2);
        } else if (touche == NkKey::NK_RIGHT) {
            changer(nombre + nombre / 4 + 1);
        } else if (touche == NkKey::NK_LEFT) {
            changer(nombre - nombre / 4);
        } else if (touche == NkKey::NK_SPACE) {
            groupes = !groupes;
            changer(nombre);
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
        vue.SetPosition({0.f, 1.70f, 1.5f});
        vue.SetTarget({0.f, 1.40f, -2.f});

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
        if (groupes) {
            CubesGroupes(rendu3d, cube, nombre);
        } else {
            CubesUnParUn(rendu3d, cube, nombre);
        }
        rendu->Present();
        rendu->EndFrame();

        images++;
        if (images < IMAGES_PAR_MESURE) {
            continue;
        }
        double secondes = std::chrono::duration<double>(Horloge::now() - debutMesure).count();
        double cadence = IMAGES_PAR_MESURE / secondes;
        uint32 ordres = rendu->GetStats().drawCalls;
        mesures++;
        if (!automatique || mesures > 1) {
            NoterMesure(nombre, groupes, ordres, cadence);
        }
        char titre[200];
        std::snprintf(titre, sizeof(titre), "Cent cubes - %d cubes - drawCalls %u - %.1f images/s",
                      nombre, ordres, cadence);
        fenetre.SetTitle(titre);
        images = 0;
        debutMesure = Horloge::now();
        if (automatique && mesures > 1) {
            enMarche = false;
        }
    }

    peripherique->WaitIdle();
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(peripherique);
    fenetre.Close();
    return 0;
}
