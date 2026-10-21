#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKEvent/NkKeyboardEvent.h"
#include "NKLogger/NkLog.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkIDevice.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Core/NkCamera.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Mesh/NkMeshSystem.h"
#include "NKRenderer/Materials/NkMaterial.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"
#include <cstdio>
#include <cstdlib>

using namespace nkentseu;
using namespace nkentseu::renderer;

class CaptureSalle {
public:
    CaptureSalle() {
        const char* n = std::getenv("MASALLE_SHOT");
        if (n && *n) {
            mVisee = (uint64)std::atoll(n);
        }
        const char* p = std::getenv("MASALLE_SHOT_PATH");
        if (p && *p) {
            mChemin = p;
        }
    }

    bool Etape(NkRenderer* r, NkIDevice* d, uint32 w, uint32 h) {
        if (mVisee == 0) {
            return false;
        }
        if (!mArmee && mImages + 3 >= mVisee) {
            NkOffscreenDesc od;
            od.width = w;
            od.height = h;
            od.hasDepth = false;
            od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
            od.readback = true;
            od.name = "masalle_capture";
            if (!mCible.Init(d, r->GetTextures(), od)) {
                return true;
            }
            r->SetFinalColorTarget(r->GetTextures()->GetRHIHandle(mCible.GetColorResult()));
            mArmee = true;
        } else if (mArmee && mImages >= mVisee) {
            d->WaitIdle();
            bool ok = mCible.Capture(mChemin);
            std::printf("Capture de l'image %llu -> %s : %s\n", (unsigned long long)mImages,
                        mChemin, ok ? "OK" : "ECHEC");
            r->SetFinalColorTarget(NkTextureHandle{});
            mCible.Shutdown();
            mArmee = false;
            return true;
        }
        return false;
    }

    void ImageRendue() {
        mImages++;
    }

private:
    uint64 mVisee = 0;
    uint64 mImages = 0;
    const char* mChemin = "salle.png";
    bool mArmee = false;
    NkOffscreenTarget mCible;
};

const float32 LARGEUR = 4.f;
const float32 PROFONDEUR = 4.f;
const float32 HAUTEUR = 2.5f;
const float32 EPAISSEUR = 0.10f;
const float32 DECALAGE = 0.001f;

static NkAABB Boite(NkVec3f c, NkVec3f t) {
    NkVec3f d = {t.x * 0.5f, t.y * 0.5f, t.z * 0.5f};
    return {{c.x - d.x, c.y - d.y, c.z - d.z}, {c.x + d.x, c.y + d.y, c.z + d.z}};
}

static void Pave(NkRender3D* r3d, NkMeshHandle cube, NkVec3f centre, NkVec3f taille,
                 NkVec3f teinte, bool ombre = true) {
    NkDrawCall3D dc;
    dc.mesh = cube;
    dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
    dc.aabb = Boite(centre, taille);
    dc.tint = teinte;
    dc.roughness = 0.85f;
    dc.castShadow = ombre;
    r3d->Submit(dc);
}

static void PaveMateriau(NkRender3D* r3d, NkMeshHandle cube, NkVec3f centre, NkVec3f taille,
                         NkMatInstHandle materiau) {
    NkDrawCall3D dc;
    dc.mesh = cube;
    dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
    dc.aabb = Boite(centre, taille);
    dc.tint = {1.f, 1.f, 1.f};
    dc.material = materiau;
    dc.castShadow = false;
    r3d->Submit(dc);
}

static void DessinerSalle(NkRender3D* r3d, NkMeshHandle cube, NkMeshHandle plan) {
    NkVec3f mur = {0.72f, 0.70f, 0.66f};
    NkVec3f bois = {0.55f, 0.36f, 0.20f};
    NkVec3f porte = {0.42f, 0.26f, 0.14f};
    NkVec3f verre = {0.55f, 0.75f, 0.95f};
    NkVec3f blanc = {0.92f, 0.92f, 0.90f};
    float32 L = LARGEUR;
    float32 P = PROFONDEUR;
    float32 H = HAUTEUR;
    float32 E = EPAISSEUR;

    NkDrawCall3D sol;
    sol.mesh = plan;
    sol.transform = NkMat4f::Scale({L, 1.f, P});
    sol.aabb = {{-L / 2, 0.f, -P / 2}, {L / 2, 0.f, P / 2}};
    sol.tint = {0.45f, 0.37f, 0.28f};
    sol.roughness = 0.8f;
    sol.castShadow = false;
    r3d->Submit(sol);

    Pave(r3d, cube, {0.f, H / 2, (P + E) / 2}, {L + 2 * E, H, E}, mur, false);
    Pave(r3d, cube, {-(L + E) / 2, H / 2, 0.f}, {E, H, P}, mur, false);
    Pave(r3d, cube, {(L + E) / 2, H / 2, 0.f}, {E, H, P}, mur, false);
    Pave(r3d, cube, {0.f, H + E / 2, 0.f}, {L + 2 * E, E, P + 2 * E}, blanc, false);

    float32 zPorte = -P / 2 + 0.01f + DECALAGE;
    Pave(r3d, cube, {-0.70f, 1.00f, zPorte}, {0.90f, 2.00f, 0.02f}, porte, false);
    for (float32 z : {-0.90f, 0.90f}) {
        Pave(r3d, cube, {-L / 2 + 0.01f + DECALAGE, 1.40f, z}, {0.02f, 1.00f, 1.20f}, verre, false);
    }

    Pave(r3d, cube, {0.60f, 0.725f, 0.70f}, {1.20f, 0.05f, 0.80f}, bois);
    for (float32 x : {0.60f - 0.57f, 0.60f + 0.57f}) {
        for (float32 z : {0.70f - 0.37f, 0.70f + 0.37f}) {
            Pave(r3d, cube, {x, 0.35f, z}, {0.06f, 0.70f, 0.06f}, bois);
        }
    }
}

static bool ArgumentPresent(const NkEntryState& etat, const char* mot) {
    for (const NkString& a : etat.args) {
        if (a == mot) {
            return true;
        }
    }
    return false;
}

const char* const CHEMIN_TEXTURE = "murs/mur.png";
const NkVec3f MUR_FOND_CENTRE = {0.f, HAUTEUR * 0.5f, -(PROFONDEUR + EPAISSEUR) * 0.5f};
const NkVec3f MUR_FOND_TAILLE = {LARGEUR + 2 * EPAISSEUR, HAUTEUR, EPAISSEUR};

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    NkWindowConfig config;
    config.title = "Le premier mur texture";
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
    std::printf("Interface graphique : %s\n", NkGraphicsApiName(peripherique->GetApi()));

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

    NkVec3f oeil = {0.f, 1.70f, 1.5f};
    NkVec3f cible = {0.f, 1.40f, -2.f};
    bool avant = ArgumentPresent(etat, "avant");
    NkMaterial* materiauMur = nullptr;
    if (!avant) {
        NkTexHandle texture = rendu->GetTextures()->Load(CHEMIN_TEXTURE);
        if (!texture.IsValid()) {
            std::printf("Texture %s introuvable : voir logs/app.log\n", CHEMIN_TEXTURE);
        }
        NkMaterialType type = NkMaterialType::NK_PBR_METALLIC;
        materiauMur = NkMaterial::Create(rendu->GetMaterials(), type);
        materiauMur->SetAlbedoMap(texture);
        materiauMur->SetRoughness(0.9f);
        materiauMur->SetTriplanarTileSize(1.0f);
    }

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

    CaptureSalle capture;
    while (enMarche && fenetre.IsOpen()) {
        evenements.PollEvents();
        if (!enMarche) {
            break;
        }
        if (capture.Etape(rendu, peripherique, largeur, hauteur)) {
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
        vue.SetPosition(oeil);
        vue.SetTarget(cible);

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
        if (materiauMur) {
            NkMatInstHandle instance = materiauMur->GetInstHandle();
            PaveMateriau(rendu3d, cube, MUR_FOND_CENTRE, MUR_FOND_TAILLE, instance);
        } else {
            NkVec3f gris = {0.72f, 0.70f, 0.66f};
            Pave(rendu3d, cube, MUR_FOND_CENTRE, MUR_FOND_TAILLE, gris, false);
        }
        rendu->Present();
        rendu->EndFrame();
        capture.ImageRendue();
    }

    peripherique->WaitIdle();
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(peripherique);
    fenetre.Close();
    return 0;
}
