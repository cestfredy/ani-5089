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
#include <cstdlib>

using namespace nkentseu;
using namespace nkentseu::renderer;

const float32 LARGEUR = 4.f;
const float32 PROFONDEUR = 4.f;
const float32 HAUTEUR = 2.5f;
const float32 EPAISSEUR = 0.10f;

const float32 PORTE_X = -0.70f;
const float32 PORTE_LARGEUR = 0.90f;
const float32 PORTE_HAUTEUR = 2.00f;
const float32 FENETRE_LARGEUR = 1.20f;
const float32 FENETRE_HAUTEUR = 1.00f;
const float32 FENETRE_Y = 1.40f;
const float32 FENETRE_Z[2] = {-0.90f, 0.90f};
const float32 PANNEAU = 0.02f;

const float32 Z_MUR_FOND = -(PROFONDEUR * 0.5f + EPAISSEUR * 0.5f);
const float32 X_MUR_GAUCHE = -(LARGEUR * 0.5f + EPAISSEUR * 0.5f);
const float32 Z_FACE_FOND = Z_MUR_FOND + EPAISSEUR * 0.5f;
const float32 X_FACE_GAUCHE = X_MUR_GAUCHE + EPAISSEUR * 0.5f;

const float32 DECALAGE_RETENU = 0.001f;
const float32 PAS_DECALAGE = 0.0005f;

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
    SoumettreCube(r3d, cube, {0.f, yMur, Z_MUR_FOND}, murLong, mur, false);
    SoumettreCube(r3d, cube, {0.f, yMur, demiP + demiE}, murLong, mur, false);
    SoumettreCube(r3d, cube, {X_MUR_GAUCHE, yMur, 0.f}, murCote, mur, false);
    SoumettreCube(r3d, cube, {demiL + demiE, yMur, 0.f}, murCote, mur, false);
    NkVec3f plafond = {largeurTotale, EPAISSEUR, profondeurTotale};
    SoumettreCube(r3d, cube, {0.f, HAUTEUR + demiE, 0.f}, plafond, blanc, false);
}

static void DessinerOuvertures(NkRender3D* r3d, NkMeshHandle cube, float32 decalage) {
    NkVec3f bois = {0.42f, 0.26f, 0.14f};
    NkVec3f verre = {0.55f, 0.75f, 0.95f};
    NkVec3f porte = {PORTE_LARGEUR, PORTE_HAUTEUR, PANNEAU};
    float32 zPorte = Z_FACE_FOND - PANNEAU * 0.5f + decalage;
    SoumettreCube(r3d, cube, {PORTE_X, PORTE_HAUTEUR * 0.5f, zPorte}, porte, bois, false);

    NkVec3f fenetre = {PANNEAU, FENETRE_HAUTEUR, FENETRE_LARGEUR};
    float32 xFenetre = X_FACE_GAUCHE - PANNEAU * 0.5f + decalage;
    for (float32 z : FENETRE_Z) {
        SoumettreCube(r3d, cube, {xFenetre, FENETRE_Y, z}, fenetre, verre, false);
    }
}

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    float32 decalage = DECALAGE_RETENU;
    for (const NkString& a : etat.args) {
        const char* texte = a.CStr();
        bool nombre = texte[0] >= '0' && texte[0] <= '9';
        if (nombre) {
            decalage = (float32)std::atof(texte) / 1000.f;
        }
    }

    NkWindowConfig config;
    config.title = "Porte et fenetres";
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

    auto afficherDecalage = [&]() {
        char t[160];
        float32 mm = decalage * 1000.f;
        std::snprintf(t, sizeof(t), "Porte et fenetres - decalage = %.1f mm", mm);
        fenetre.SetTitle(t);
        std::printf("Decalage : %.1f mm\n", mm);
    };
    afficherDecalage();

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
            decalage += PAS_DECALAGE;
            afficherDecalage();
        } else if (touche == NkKey::NK_DOWN && decalage > 0.f) {
            decalage -= PAS_DECALAGE;
            afficherDecalage();
        } else if (touche == NkKey::NK_SPACE) {
            decalage = DECALAGE_RETENU;
            afficherDecalage();
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
        soleil.castShadow = true;
        scene.lights.PushBack(soleil);

        rendu3d->BeginScene(scene);
        DessinerSalle(rendu3d, cube, plan);
        DessinerOuvertures(rendu3d, cube, decalage);
        rendu->Present();
        rendu->EndFrame();
    }

    peripherique->WaitIdle();
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(peripherique);
    fenetre.Close();
    return 0;
}
