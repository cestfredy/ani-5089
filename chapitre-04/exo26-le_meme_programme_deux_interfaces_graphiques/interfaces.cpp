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

using namespace nkentseu;
using namespace nkentseu::renderer;

const float32 LARGEUR = 4.f;
const float32 PROFONDEUR = 4.f;
const float32 HAUTEUR = 2.5f;
const float32 EPAISSEUR = 0.10f;

const float32 TABLE_X = 0.60f;
const float32 TABLE_Z = 0.70f;
const float32 TABLE_LONGUEUR = 1.20f;
const float32 TABLE_PROFONDEUR = 0.80f;
const float32 DESSUS = 0.75f;
const float32 PLATEAU_EPAISSEUR = 0.05f;
const float32 PIED = 0.06f;
const float32 Y_PLATEAU = DESSUS - PLATEAU_EPAISSEUR * 0.5f;
const float32 HAUTEUR_PIED = DESSUS - PLATEAU_EPAISSEUR;
const float32 Y_PIED = HAUTEUR_PIED * 0.5f;

const int IMAGES_IGNOREES = 30;
const int IMAGES_MESUREES = 400;
const int IMAGES_TOTAL = IMAGES_IGNOREES + IMAGES_MESUREES;

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

static void DessinerTable(NkRender3D* r3d, NkMeshHandle cube) {
    NkVec3f bois = {0.55f, 0.36f, 0.20f};
    NkVec3f plateau = {TABLE_LONGUEUR, PLATEAU_EPAISSEUR, TABLE_PROFONDEUR};
    SoumettreCube(r3d, cube, {TABLE_X, Y_PLATEAU, TABLE_Z}, plateau, bois);

    float32 dx = TABLE_LONGUEUR * 0.5f - PIED * 0.5f;
    float32 dz = TABLE_PROFONDEUR * 0.5f - PIED * 0.5f;
    NkVec3f pied = {PIED, HAUTEUR_PIED, PIED};
    for (float32 sx : {-1.f, 1.f}) {
        for (float32 sz : {-1.f, 1.f}) {
            SoumettreCube(r3d, cube, {TABLE_X + sx * dx, Y_PIED, TABLE_Z + sz * dz}, pied, bois);
        }
    }
}

static NkGraphicsApi InterfaceDemandee(const NkEntryState& etat) {
    for (const NkString& a : etat.args) {
        if (a == "logiciel" || a == "software") {
            return NkGraphicsApi::NK_GFX_API_SOFTWARE;
        }
        if (a == "opengl") {
            return NkGraphicsApi::NK_GFX_API_OPENGL;
        }
        if (a == "dx11") {
            return NkGraphicsApi::NK_GFX_API_DX11;
        }
        if (a == "dx12") {
            return NkGraphicsApi::NK_GFX_API_DX12;
        }
        if (a == "vulkan") {
            return NkGraphicsApi::NK_GFX_API_VULKAN;
        }
    }
    return NkGraphicsApi::NK_GFX_API_SOFTWARE;
}

static void EcrireMesure(const char* interfaceGraphique, double moyenne) {
    char nomFichier[64];
    std::snprintf(nomFichier, sizeof(nomFichier), "mesure_%s.txt", interfaceGraphique);
    std::FILE* f = std::fopen(nomFichier, "w");
    if (!f) {
        return;
    }
    std::fprintf(f, "%s %.2f ms\n", interfaceGraphique, moyenne);
    std::fclose(f);
}

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    NkGraphicsApi api = InterfaceDemandee(etat);

    NkWindowConfig config;
    config.title = "Deux interfaces";
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
    NkIDevice* peripherique = NkDeviceFactory::CreateForApi(api, info);
    if (!peripherique || !peripherique->IsValid()) {
        std::printf("Refus : l'interface %s ne demarre pas (voir logs/app.log)\n",
                    NkGraphicsApiName(api));
        logger.Error("Refus : l'interface {0} ne demarre pas", NkGraphicsApiName(api));
        if (peripherique) {
            NkDeviceFactory::Destroy(peripherique);
        }
        return 2;
    }

    const char* interfaceGraphique = NkGraphicsApiName(peripherique->GetApi());
    std::printf("Interface graphique : %s\n", interfaceGraphique);
    logger.Info("Interface graphique : {0}", interfaceGraphique);
    char titre[160];
    std::snprintf(titre, sizeof(titre), "Deux interfaces - %s", interfaceGraphique);
    fenetre.SetTitle(titre);

    NkRendererConfig reglages = NkRendererConfig::ForGame(api, info.width, info.height);
    reglages.api = api;
    reglages.subsystems = NK_SS_RENDER3D | NK_SS_SHADOW | NK_SS_POST_PROCESS;
    reglages.vsync = false;
    NkRenderer* rendu = NkRenderer::Create(peripherique, reglages);
    if (!rendu || !rendu->Initialize()) {
        std::printf("Refus : le renderer ne s'initialise pas sur %s (voir logs/app.log)\n",
                    interfaceGraphique);
        logger.Error("Refus : le renderer ne s'initialise pas sur {0}", interfaceGraphique);
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
    int image = 0;
    double totalMs = 0.0;

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
        Horloge::time_point debut = Horloge::now();
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
        vue.SetTarget({0.6f, 0.45f, 0.5f});

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
        DessinerTable(rendu3d, cube);
        rendu->Present();
        rendu->EndFrame();
        double ms = std::chrono::duration<double, std::milli>(Horloge::now() - debut).count();
        image++;
        if (image > IMAGES_TOTAL) {
            continue;
        }
        if (image > IMAGES_IGNOREES) {
            totalMs += ms;
        }
        if (image % 25 == 0 && image < IMAGES_TOTAL) {
            char t[160];
            std::snprintf(t, sizeof(t), "Deux interfaces - %s - image %d / %d",
                          interfaceGraphique, image, IMAGES_TOTAL);
            fenetre.SetTitle(t);
            std::printf("image %d / %d (derniere : %.1f ms)\n", image, IMAGES_TOTAL, ms);
        }
        if (image == IMAGES_TOTAL) {
            double moyenne = totalMs / IMAGES_MESUREES;
            std::printf("%s : %.2f ms par image (moyenne sur %d images)\n",
                        interfaceGraphique, moyenne, IMAGES_MESUREES);
            logger.Info("{0} : {1} ms par image (moyenne sur 400 images)",
                        interfaceGraphique, moyenne);
            char t[160];
            std::snprintf(t, sizeof(t), "Deux interfaces - %s - %.2f ms par image",
                          interfaceGraphique, moyenne);
            fenetre.SetTitle(t);
            EcrireMesure(interfaceGraphique, moyenne);
        }
    }

    peripherique->WaitIdle();
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(peripherique);
    fenetre.Close();
    return 0;
}
