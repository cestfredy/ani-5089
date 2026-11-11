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
const float32 DEMI_ECART = 0.0315f;
const uint32 IMAGES_PAR_CHAINE = 2;
const uint64 IMAGES_TRACEES = 30;

class Chaine {
public:
    static constexpr uint32 kMaxImages = 4u;

    bool Creer(uint32 nombre) {
        if (nombre == 0u || nombre > kMaxImages) {
            return false;
        }
        mNombre = nombre;
        mSuivante = 0u;
        mEmpruntee = false;
        for (uint32 i = 0; i < kMaxImages; ++i) {
            mImages[i] = 0u;
        }
        return true;
    }

    void Detruire() {
        mNombre = 0u;
        mSuivante = 0u;
        mEmpruntee = false;
        for (uint32 i = 0; i < kMaxImages; ++i) {
            mImages[i] = 0u;
        }
    }

    void Enregistrer(uint32 index, uint64 poignee) {
        if (index < mNombre) {
            mImages[index] = poignee;
        }
    }

    uint64 Poignee(uint32 index) const {
        if (index < mNombre) {
            return mImages[index];
        }
        return 0u;
    }

    bool Emprunter(uint32& index) {
        if (mNombre == 0u || mEmpruntee) {
            return false;
        }
        index = mSuivante;
        mEmpruntee = true;
        return true;
    }

    bool Rendre() {
        if (!mEmpruntee) {
            return false;
        }
        mSuivante = (mSuivante + 1u) % mNombre;
        mEmpruntee = false;
        return true;
    }

    uint32 DerniereRendue() const {
        if (mNombre == 0u) {
            return 0u;
        }
        return (mSuivante + mNombre - 1u) % mNombre;
    }

private:
    uint64 mImages[kMaxImages]{};
    uint32 mNombre = 0u;
    uint32 mSuivante = 0u;
    bool mEmpruntee = false;
};

struct ImageOeil {
    NkOffscreenTarget* cible = nullptr;
    NkRenderer* renderer = nullptr;
};

struct Oeil {
    const char* nom = "";
    Chaine chaine;
    ImageOeil images[IMAGES_PAR_CHAINE];
    NkVec3f position;
    uint32 largeur = 0;
    uint32 hauteur = 0;
};

struct Contexte {
    NkIDevice* device = nullptr;
    NkRenderer* rEcran = nullptr;
    NkGraphicsApi api;
    std::FILE* journal = nullptr;
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

static void Operation(const Contexte& c, const char* texte, const Oeil& oeil) {
    std::printf("[MaSalle] %s, %s\n", texte, oeil.nom);
    if (c.journal != nullptr) {
        std::fprintf(c.journal, "%s, %s\n", texte, oeil.nom);
    }
}

static bool CreerImage(ImageOeil& img, const Contexte& c, uint32 l, uint32 h, const char* nom) {
    NkOffscreenDesc od;
    od.width = l;
    od.height = h;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = true;
    od.name = nom;
    img.cible = c.rEcran->CreateOffscreen(od);
    if (img.cible == nullptr) {
        return false;
    }
    NkRendererConfig cfg = NkRendererConfig::ForGame(c.api, l, h);
    cfg.Disable(NK_SS_RENDER2D);
    cfg.Disable(NK_SS_TEXT);
    cfg.Disable(NK_SS_OVERLAY);
    cfg.Enable(NK_SS_OFFSCREEN);
    img.renderer = NkRenderer::Create(c.device, cfg);
    if (img.renderer == nullptr || !img.renderer->Initialize()) {
        return false;
    }
    NkTexHandle couleur = img.cible->GetColorResult();
    img.renderer->SetFinalColorTarget(c.rEcran->GetTextures()->GetRHIHandle(couleur));
    img.renderer->SetRenderSizeOverride(l, h);
    return true;
}

static bool CreerImages(Oeil& oeil, const Contexte& c, uint32 l, uint32 h) {
    oeil.largeur = l;
    oeil.hauteur = h;
    for (uint32 i = 0; i < IMAGES_PAR_CHAINE; ++i) {
        if (!CreerImage(oeil.images[i], c, l, h, oeil.nom)) {
            return false;
        }
    }
    char texte[96];
    std::snprintf(texte, sizeof(texte), "cree %u images de %ux%u", IMAGES_PAR_CHAINE, l, h);
    Operation(c, texte, oeil);
    return true;
}

static void DetruireImages(Oeil& oeil, const Contexte& c) {
    for (uint32 i = 0; i < IMAGES_PAR_CHAINE; ++i) {
        NkRenderer::Destroy(oeil.images[i].renderer);
        oeil.images[i].renderer = nullptr;
        c.rEcran->DestroyOffscreen(oeil.images[i].cible);
        oeil.images[i].cible = nullptr;
    }
    Operation(c, "detruit les images", oeil);
}

static void CreerChaine(Oeil& oeil, const Contexte& c) {
    oeil.chaine.Creer(IMAGES_PAR_CHAINE);
    Operation(c, "cree la chaine", oeil);
}

static void Enregistrer(Oeil& oeil, const Contexte& c) {
    for (uint32 i = 0; i < IMAGES_PAR_CHAINE; ++i) {
        oeil.chaine.Enregistrer(i, oeil.images[i].cible->GetColorResult().id);
    }
    char texte[96];
    std::snprintf(texte, sizeof(texte), "enregistre les poignees %llu et %llu",
                  (unsigned long long)oeil.chaine.Poignee(0),
                  (unsigned long long)oeil.chaine.Poignee(1));
    Operation(c, texte, oeil);
}

static void DessinerOeil(Oeil& oeil, uint32 index, NkICommandBuffer* cmd, uint64 image) {
    NkRenderer* renderer = oeil.images[index].renderer;
    NkRender3D* r3d = renderer->GetRender3D();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    NkMaterialCollection* materiaux = renderer->GetMaterialCollection();
    if (materiaux != nullptr) {
        materiaux->Upload();
    }

    NkCamera3DData cd;
    cd.position = oeil.position;
    cd.target = oeil.position + NkVec3f(0.f, 0.f, -1.f);
    cd.up = {0.f, 1.f, 0.f};
    cd.fovY = 70.f;
    cd.aspect = (float32)oeil.largeur / (float32)oeil.hauteur;
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

static bool UnOeil(Oeil& oeil, NkICommandBuffer* cmd, uint64 image, uint32& index) {
    if (!oeil.chaine.Emprunter(index)) {
        std::printf("[MaSalle] %s : emprunt refuse\n", oeil.nom);
        return false;
    }
    DessinerOeil(oeil, index, cmd, image);
    return oeil.chaine.Rendre();
}

static void Soumettre(Oeil& oeil, NkRender2D* r2d, float32 x, float32 l, float32 h) {
    NkTexHandle poignee;
    poignee.id = oeil.chaine.Poignee(oeil.chaine.DerniereRendue());
    if (poignee.id == 0u) {
        return;
    }
    r2d->DrawImage(poignee, {x, 0.f, l, h});
}

static void Capturer(Oeil* yeux[2], uint64 image) {
    for (int32 i = 0; i < 2; ++i) {
        Oeil& o = *yeux[i];
        uint32 index = o.chaine.DerniereRendue();
        char chemin[96];
        std::snprintf(chemin, sizeof(chemin), "capture-%llu-%s.png", (unsigned long long)image,
                      i == 0 ? "gauche" : "droit");
        bool ok = o.images[index].cible->Capture(chemin);
        std::printf("[MaSalle] capture image %llu, %s, index %u -> %s (%s)\n",
                    (unsigned long long)image, o.nom, index, chemin, ok ? "ok" : "echec");
    }
}

static uint64 EnvU64(const char* nom) {
    const char* t = std::getenv(nom);
    if (t == nullptr || t[0] == '\0') {
        return 0;
    }
    return (uint64)std::strtoull(t, nullptr, 10);
}

struct Evenements {
    bool fermee = false;
    bool redimensionnee = false;
    uint32 largeur = 0;
    uint32 hauteur = 0;
};

static Evenements Lire(uint32 largeur, uint32 hauteur) {
    Evenements e;
    e.largeur = largeur;
    e.hauteur = hauteur;
    NkEvent* ev = nullptr;
    while (NkEvents().PollEvent(ev)) {
        if (ev->As<NkWindowCloseEvent>() != nullptr) {
            e.fermee = true;
        }
        NkWindowResizeEvent* r = ev->As<NkWindowResizeEvent>();
        if (r == nullptr) {
            continue;
        }
        uint32 l = (uint32)r->GetWidth();
        uint32 h = (uint32)r->GetHeight();
        bool change = l != e.largeur || h != e.hauteur;
        if (l > 0 && h > 0 && change) {
            e.largeur = l;
            e.hauteur = h;
            e.redimensionnee = true;
        }
    }
    return e;
}

static bool Redimensionner(Oeil* yeux[2], const Contexte& c, uint32 l, uint32 h, bool recreer) {
    c.device->WaitIdle();
    std::printf("[MaSalle] fenetre redimensionnee : %ux%u\n", l, h);
    if (c.journal != nullptr) {
        std::fprintf(c.journal, "fenetre redimensionnee en %ux%u, attente du GPU\n", l, h);
    }
    c.rEcran->OnResize(l, h);
    for (int32 i = 0; i < 2; ++i) {
        Oeil& o = *yeux[i];
        if (recreer) {
            o.chaine.Detruire();
            Operation(c, "detruit la chaine", o);
        }
        DetruireImages(o, c);
        if (!CreerImages(o, c, l / 2, h)) {
            return false;
        }
        if (recreer) {
            CreerChaine(o, c);
            Enregistrer(o, c);
        }
    }
    return true;
}

int nkmain(const NkEntryState&) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    bool recreer = EnvU64("MASALLE_SANS_RECREER") != 1;
    uint64 imageResize = EnvU64("MASALLE_RESIZE");
    uint64 imageDeSortie = EnvU64("MASALLE_EXIT");
    uint64 imageCapture1 = EnvU64("MASALLE_SHOT");
    uint64 imageCapture2 = EnvU64("MASALLE_SHOT2");

    NkWindowConfig cfg;
    cfg.title = "Ma salle - deux chaines";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        return 1;
    }

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    Contexte c;
    c.device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (c.device == nullptr) {
        return 1;
    }
    c.api = devInit.api;
    NkRendererConfig cfgEcran = NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
    cfgEcran.Enable(NK_SS_OFFSCREEN);
    c.rEcran = NkRenderer::Create(c.device, cfgEcran);
    if (c.rEcran == nullptr || !c.rEcran->Initialize()) {
        NkDeviceFactory::Destroy(c.device);
        return 1;
    }

    Oeil gauche;
    Oeil droit;
    gauche.nom = "oeil gauche";
    droit.nom = "oeil droit";
    gauche.position = TETE - NkVec3f(DEMI_ECART, 0.f, 0.f);
    droit.position = TETE + NkVec3f(DEMI_ECART, 0.f, 0.f);
    Oeil* yeux[2] = {&gauche, &droit};
    uint32 largeur = devInit.width;
    uint32 hauteur = devInit.height;
    for (int32 i = 0; i < 2; ++i) {
        if (!CreerImages(*yeux[i], c, largeur / 2, hauteur)) {
            return 1;
        }
        CreerChaine(*yeux[i], c);
        Enregistrer(*yeux[i], c);
    }
    c.journal = std::fopen("resize.txt", "w");
    std::FILE* trace = std::fopen("trace.txt", "w");

    uint64 image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        Evenements e = Lire(largeur, hauteur);
        if (e.fermee) {
            break;
        }
        if (imageResize != 0 && image == imageResize) {
            fenetre.SetSize(960, 540);
        }
        if (e.redimensionnee) {
            largeur = e.largeur;
            hauteur = e.hauteur;
            if (!Redimensionner(yeux, c, largeur, hauteur, recreer)) {
                break;
            }
        }
        if (c.rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = c.rEcran->GetCmd();
            uint32 indexG = 0;
            uint32 indexD = 0;
            UnOeil(gauche, cmd, image, indexG);
            UnOeil(droit, cmd, image, indexD);
            NkRender2D* r2d = c.rEcran->GetRender2D();
            if (r2d != nullptr) {
                float32 demi = (float32)largeur * 0.5f;
                r2d->Begin(cmd, largeur, hauteur);
                Soumettre(gauche, r2d, 0.f, demi, (float32)hauteur);
                Soumettre(droit, r2d, demi, demi, (float32)hauteur);
                r2d->End();
            }
            c.rEcran->Present();
            c.rEcran->EndFrame();
            ++image;
            bool capture = image == imageCapture1 || image == imageCapture2;
            if (capture) {
                Capturer(yeux, image);
            }
            if (trace != nullptr && image <= IMAGES_TRACEES) {
                std::fprintf(trace, "image : %llu, gauche : %u, droit : %u\n",
                             (unsigned long long)image, indexG, indexD);
            }
        }
        if (imageDeSortie != 0 && image >= imageDeSortie) {
            tourne = false;
        }
    }

    c.device->WaitIdle();
    if (trace != nullptr) {
        std::fclose(trace);
    }
    if (c.journal != nullptr) {
        std::fclose(c.journal);
    }
    for (int32 i = 0; i < 2; ++i) {
        for (uint32 k = 0; k < IMAGES_PAR_CHAINE; ++k) {
            NkRenderer::Destroy(yeux[i]->images[k].renderer);
            c.rEcran->DestroyOffscreen(yeux[i]->images[k].cible);
        }
    }
    NkRenderer::Destroy(c.rEcran);
    NkDeviceFactory::Destroy(c.device);
    std::printf("[MaSalle] %llu images\n", (unsigned long long)image);
    return 0;
}
