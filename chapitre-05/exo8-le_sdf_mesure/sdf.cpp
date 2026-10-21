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
#include "NKImage/Core/NkImage.h"
#include "NKFont/NkFont.h"
#include "NKFont/Embedded/NkFontEmbedded.h"
#include <filesystem>
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

const float32 PANNEAU_HAUT = 0.20f;
const float32 PANNEAU_EPAIS = 0.01f;
const float32 PANNEAU_Y = 2.00f + 0.02f + PANNEAU_HAUT * 0.5f;
const float32 PANNEAU_Z = -PROFONDEUR * 0.5f + 0.003f + PANNEAU_EPAIS * 0.5f;

const uint32 COULEUR_FOND = 0x1E7B3CFF;
const uint8 TEXTE_R = 255;
const uint8 TEXTE_G = 255;
const uint8 TEXTE_B = 255;
const uint8 FOND_R = 0x1E;
const uint8 FOND_G = 0x7B;
const uint8 FOND_B = 0x3C;

const uint32 PIXELS_BRUTS = 0;

const char* const MOT = "Sortie";
const int32 ECHELLE = 8;
float32 DOUCEUR_SDF = 0.05f;

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

    Pave(r3d, cube, {0.f, H / 2, -(P + E) / 2}, {L + 2 * E, H, E}, mur, false);
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

static void LireDouceur(const NkEntryState& etat) {
    for (const NkString& a : etat.args) {
        const char* t = a.CStr();
        bool reglage = a.Length() > 2 && t[0] == 's' && t[1] == '=';
        if (reglage) {
            DOUCEUR_SDF = (float32)std::atof(t + 2);
        }
    }
}

static void Melanger(NkImage& image, int32 x, int32 y, float32 couverture) {
    if (couverture <= 0.f) {
        return;
    }
    if (couverture > 1.f) {
        couverture = 1.f;
    }
    uint8 r = (uint8)(FOND_R + (TEXTE_R - FOND_R) * couverture + 0.5f);
    uint8 g = (uint8)(FOND_G + (TEXTE_G - FOND_G) * couverture + 0.5f);
    uint8 b = (uint8)(FOND_B + (TEXTE_B - FOND_B) * couverture + 0.5f);
    image.SetPixel(x, y, math::NkColor(r, g, b, 255));
}

static float32 smoothstep(float32 a, float32 b, float32 x) {
    float32 t = (x - a) / (b - a);
    if (t < 0.f) {
        t = 0.f;
    }
    if (t > 1.f) {
        t = 1.f;
    }
    return t * t * (3.f - 2.f * t);
}

static float32 Pixel(const nkft_uint8* p, int32 w, int32 h, int32 i, int32 j) {
    if (i < 0) {
        i = 0;
    }
    if (i >= w) {
        i = w - 1;
    }
    if (j < 0) {
        j = 0;
    }
    if (j >= h) {
        j = h - 1;
    }
    return (float32)p[j * w + i];
}

static float32 LireBilineaire(const nkft_uint8* p, int32 w, int32 h, float32 u, float32 v) {
    float32 x = u * w - 0.5f;
    float32 y = v * h - 0.5f;
    int32 x0 = (int32)x;
    int32 y0 = (int32)y;
    if (x < 0.f) {
        x0 = -1;
    }
    if (y < 0.f) {
        y0 = -1;
    }
    float32 fx = x - (float32)x0;
    float32 fy = y - (float32)y0;
    float32 haut = Pixel(p, w, h, x0, y0) * (1.f - fx) + Pixel(p, w, h, x0 + 1, y0) * fx;
    float32 bas = Pixel(p, w, h, x0, y0 + 1) * (1.f - fx) + Pixel(p, w, h, x0 + 1, y0 + 1) * fx;
    return haut * (1.f - fy) + bas * fy;
}

static NkImage FabriquerPanneau(NkFont* police, const nkft_uint8* atlasPixels, int32 texW,
                                int32 texH, bool sdf) {
    float32 largeurMot = 0.f;
    for (const char* c = MOT; *c; ++c) {
        largeurMot += police->FindGlyph((NkFontCodepoint)*c)->advanceX;
    }
    int32 L = (int32)(largeurMot * ECHELLE + 0.999f);
    int32 H = (int32)((police->ascent - police->descent) * ECHELLE + 0.999f);
    float32 base = police->ascent * ECHELLE;

    NkImage image = NkImage::Create((uint32)L, (uint32)H, 4, COULEUR_FOND);
    float32 curseur = 0.f;
    for (const char* c = MOT; *c; ++c) {
        const NkFontGlyph* g = police->FindGlyph((NkFontCodepoint)*c);
        float32 rx0 = (curseur + g->x0) * ECHELLE;
        float32 rx1 = (curseur + g->x1) * ECHELLE;
        float32 ry0 = base + g->y0 * ECHELLE;
        float32 ry1 = base + g->y1 * ECHELLE;
        int32 yFin = (int32)(ry1 + 1.f);
        int32 xFin = (int32)(rx1 + 1.f);
        for (int32 y = (int32)ry0; y < yFin; ++y) {
            for (int32 x = (int32)rx0; x < xFin; ++x) {
                bool dehors = x < 0 || y < 0 || x >= L || y >= H;
                if (dehors) {
                    continue;
                }
                float32 fx = ((float32)x + 0.5f - rx0) / (rx1 - rx0);
                float32 fy = ((float32)y + 0.5f - ry0) / (ry1 - ry0);
                bool horsGlyphe = fx < 0.f || fx > 1.f || fy < 0.f || fy > 1.f;
                if (horsGlyphe) {
                    continue;
                }
                float32 u = g->u0 + fx * (g->u1 - g->u0);
                float32 v = g->v0 + fy * (g->v1 - g->v0);
                float32 valeur = LireBilineaire(atlasPixels, texW, texH, u, v);
                float32 alpha = valeur / 255.f;
                if (sdf) {
                    float32 dist = valeur / 255.f;
                    alpha = smoothstep(0.5f - DOUCEUR_SDF, 0.5f + DOUCEUR_SDF, dist);
                }
                Melanger(image, x, y, alpha);
            }
        }
        curseur += g->advanceX;
    }
    return image;
}

static NkFont* ConstruireAtlas(NkFontAtlas& atlas, float32 taille, bool sdf) {
    if (sdf) {
        atlas.sdfMode = true;
        atlas.sdfSpread = 6;
    }
    NkFont* police = NkFontEmbedded::AddToAtlas(atlas, NkEmbeddedFontId::Inter, taille);
    if (!police || !atlas.Build()) {
        return nullptr;
    }
    return police;
}

static void EcrireTailles(NkFontAtlas& a32, NkFontAtlas& s32, NkFontAtlas& a128) {
    std::FILE* f = std::fopen("atlas.txt", "w");
    if (!f) {
        return;
    }
    std::fprintf(f, "ordinaire 32 : %d x %d\n", a32.texWidth, a32.texHeight);
    std::fprintf(f, "sdf 32 : %d x %d\n", s32.texWidth, s32.texHeight);
    std::fprintf(f, "ordinaire 128 : %d x %d\n", a128.texWidth, a128.texHeight);
    std::fclose(f);
}

int nkmain(const NkEntryState& etat) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    NkWindowConfig config;
    config.title = "Le SDF, mesure";
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

    NkFontAtlas ordinaire32;
    NkFontAtlas sdf32;
    NkFontAtlas ordinaire128;
    NkFont* policeOrd = ConstruireAtlas(ordinaire32, 32.f, false);
    NkFont* policeSdf = ConstruireAtlas(sdf32, 32.f, true);
    NkFont* police128 = ConstruireAtlas(ordinaire128, 128.f, false);
    if (!policeOrd || !policeSdf || !police128) {
        std::printf("Un des trois atlas ne s'est pas construit\n");
        return 4;
    }
    EcrireTailles(ordinaire32, sdf32, ordinaire128);
    std::printf("ordinaire 32 : %d x %d\n", ordinaire32.texWidth, ordinaire32.texHeight);
    std::printf("sdf 32 : %d x %d\n", sdf32.texWidth, sdf32.texHeight);
    std::printf("ordinaire 128 : %d x %d\n", ordinaire128.texWidth, ordinaire128.texHeight);

    bool modeSdf = !ArgumentPresent(etat, "ordinaire");
    LireDouceur(etat);
    if (modeSdf) {
        std::printf("s = %.3f\n", DOUCEUR_SDF);
    }
    NkFontAtlas& atlas = modeSdf ? sdf32 : ordinaire32;
    NkFont* police = modeSdf ? policeSdf : policeOrd;
    nkft_uint8* atlasPixels = nullptr;
    nkft_int32 texW = 0;
    nkft_int32 texH = 0;
    atlas.GetTexDataAsAlpha8(&atlasPixels, &texW, &texH);
    if (!atlasPixels) {
        return 4;
    }
    NkImage panneau = FabriquerPanneau(police, atlasPixels, texW, texH, modeSdf);
    panneau.FlipHorizontal();
    std::filesystem::create_directories("textures");
    const char* fichier = modeSdf ? "textures/panneau-sdf.png" : "textures/panneau-ordinaire.png";
    if (!panneau.IsValid() || !panneau.SavePNG(fichier)) {
        std::printf("%s n'a pas pu etre ecrit\n", fichier);
        return 4;
    }
    std::printf("panneau %s : %d x %d -> %s\n", modeSdf ? "sdf" : "ordinaire", panneau.Width(),
                panneau.Height(), fichier);

    float32 panneauLarge = PANNEAU_HAUT * (float32)panneau.Width() / (float32)panneau.Height();
    NkLoadOptions options;
    options.compression = PIXELS_BRUTS;
    NkTexHandle texture = rendu->GetTextures()->Load(fichier, options);
    NkMaterialType type = NkMaterialType::NK_PBR_METALLIC;
    NkMaterial* materiau = NkMaterial::Create(rendu->GetMaterials(), type);
    materiau->SetAlbedoMap(texture);
    materiau->SetRoughness(0.8f);

    NkVec3f oeil = {-0.70f, 1.70f, 1.00f};
    if (ArgumentPresent(etat, "50cm")) {
        oeil = {-0.70f, 2.20f, -1.50f};
    }
    NkVec3f cible = {-0.70f, PANNEAU_Y, -PROFONDEUR * 0.5f};

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
        NkVec3f taillePanneau = {panneauLarge, PANNEAU_HAUT, PANNEAU_EPAIS};
        PaveMateriau(rendu3d, cube, {-0.70f, PANNEAU_Y, PANNEAU_Z}, taillePanneau,
                     materiau->GetInstHandle());
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
