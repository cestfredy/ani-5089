#include "NKAudio/NkAudio.h"
#include "NKImage/Core/NkImage.h"
#include "NKFont/NkFont.h"
#include "NKFont/Embedded/NkFontEmbedded.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::audio;

const int32 TRAMES = 48000;
const int32 BLOC = 256;
const int32 NB_DISTANCES = 5;
const int32 NB_MODELES = 4;
const float32 DISTANCES[NB_DISTANCES] = {0.5f, 1.f, 2.f, 4.f, 8.f};
const AttenuationModel MODELES[NB_MODELES] = {
    AttenuationModel::NONE,
    AttenuationModel::INVERSE,
    AttenuationModel::LINEAR,
    AttenuationModel::EXPONENTIAL,
};
const char* const NOMS[NB_MODELES] = {"NONE", "INVERSE", "LINEAR", "EXPONENTIAL"};

const int32 IMAGE_L = 900;
const int32 IMAGE_H = 600;
const int32 MARGE_G = 90;
const int32 MARGE_D = 40;
const int32 MARGE_HAUT = 40;
const int32 MARGE_BAS = 80;
const float32 DISTANCE_MAX = 8.f;
const float32 GAIN_MAX = 1.1f;

struct Texte {
    NkFontAtlas atlas;
    NkFont* police = nullptr;
    nkft_uint8* pixels = nullptr;
    nkft_int32 w = 0;
    nkft_int32 h = 0;
};

static VoiceParams Reglages(AttenuationModel modele, float32 d, float32 rolloff) {
    VoiceParams p;
    p.looping = true;
    p.source3d.positional = true;
    p.source3d.minDistance = 1.f;
    p.source3d.maxDistance = 100.f;
    p.source3d.rolloffFactor = rolloff;
    p.source3d.attenuationModel = modele;
    p.source3d.position[0] = 0.f;
    p.source3d.position[1] = 0.f;
    p.source3d.position[2] = -d;
    return p;
}

static float32 Mesurer(AudioEngine& moteur) {
    std::vector<float32> bloc(BLOC * 2);
    double somme = 0.0;
    int32 rendues = 0;
    while (rendues < TRAMES) {
        int32 n = BLOC;
        if (TRAMES - rendues < BLOC) {
            n = TRAMES - rendues;
        }
        moteur.RenderToBuffer(bloc.data(), n, 2);
        for (int32 i = 0; i < n * 2; ++i) {
            somme += (double)bloc[i] * bloc[i];
        }
        rendues += n;
    }
    return (float32)std::sqrt(somme / (TRAMES * 2.0));
}

static float32 MesurerModele(AudioEngine& moteur, const AudioSample& son, AttenuationModel modele,
                             float32 d, float32 rolloff) {
    AudioHandle voix = moteur.Play(son, Reglages(modele, d, rolloff));
    float32 rms = Mesurer(moteur);
    moteur.Stop(voix);
    return rms;
}

static void Point(NkImage& img, int32 x, int32 y, math::NkColor c) {
    bool dedans = x >= 0 && y >= 0 && x < (int32)img.Width() && y < (int32)img.Height();
    if (dedans) {
        img.SetPixel(x, y, c);
    }
}

static void Ligne(NkImage& img, float32 x0, float32 y0, float32 x1, float32 y1, math::NkColor c,
                  int32 epaisseur) {
    int32 pas = (int32)std::fmax(std::fabs(x1 - x0), std::fabs(y1 - y0)) + 1;
    int32 demi = epaisseur / 2;
    for (int32 i = 0; i <= pas; ++i) {
        float32 t = (float32)i / pas;
        int32 x = (int32)(x0 + (x1 - x0) * t);
        int32 y = (int32)(y0 + (y1 - y0) * t);
        for (int32 a = -demi; a <= demi; ++a) {
            for (int32 b = -demi; b <= demi; ++b) {
                Point(img, x + a, y + b, c);
            }
        }
    }
}

static void Melanger(NkImage& img, int32 x, int32 y, math::NkColor c, float32 a) {
    bool dehors = x < 0 || y < 0 || x >= (int32)img.Width() || y >= (int32)img.Height();
    if (a <= 0.f || dehors) {
        return;
    }
    math::NkColor f = img.GetPixel(x, y);
    uint8 r = (uint8)(f.r + (c.r - f.r) * a);
    uint8 g = (uint8)(f.g + (c.g - f.g) * a);
    uint8 b = (uint8)(f.b + (c.b - f.b) * a);
    img.SetPixel(x, y, math::NkColor(r, g, b, 255));
}

static void Glyphe(NkImage& img, const Texte& t, const NkFontGlyph* g, float32 x, float32 y,
                   math::NkColor c) {
    float32 rx0 = x + g->x0;
    float32 ry0 = y + g->y0;
    int32 xFin = (int32)(x + g->x1 + 1.f);
    int32 yFin = (int32)(y + g->y1 + 1.f);
    for (int32 py = (int32)ry0; py < yFin; ++py) {
        for (int32 px = (int32)rx0; px < xFin; ++px) {
            float32 fx = ((float32)px + 0.5f - rx0) / (g->x1 - g->x0);
            float32 fy = ((float32)py + 0.5f - ry0) / (g->y1 - g->y0);
            bool horsGlyphe = fx < 0.f || fx > 1.f || fy < 0.f || fy > 1.f;
            if (horsGlyphe) {
                continue;
            }
            int32 ax = (int32)((g->u0 + fx * (g->u1 - g->u0)) * t.w);
            int32 ay = (int32)((g->v0 + fy * (g->v1 - g->v0)) * t.h);
            if (ax >= t.w) {
                ax = t.w - 1;
            }
            if (ay >= t.h) {
                ay = t.h - 1;
            }
            Melanger(img, px, py, c, t.pixels[ay * t.w + ax] / 255.f);
        }
    }
}

static void Ecrire(NkImage& img, const Texte& t, float32 x, float32 y, const char* s,
                   math::NkColor c) {
    size_t longueur = std::strlen(s);
    for (size_t i = 0; i < longueur; ++i) {
        const NkFontGlyph* g = t.police->FindGlyph((NkFontCodepoint)(unsigned char)s[i]);
        if (!g) {
            continue;
        }
        Glyphe(img, t, g, x, y, c);
        x += g->advanceX;
    }
}

static float32 X(float32 d) {
    return MARGE_G + (IMAGE_L - MARGE_G - MARGE_D) * (d / DISTANCE_MAX);
}

static float32 Y(float32 g) {
    return IMAGE_H - MARGE_BAS - (IMAGE_H - MARGE_BAS - MARGE_HAUT) * (g / GAIN_MAX);
}

static void Axes(NkImage& img, const Texte& t, math::NkColor noir) {
    math::NkColor gris(215, 215, 215);
    char s[32];
    for (int32 i = 0; i <= 8; ++i) {
        float32 d = (float32)i;
        Ligne(img, X(d), Y(0.f), X(d), Y(GAIN_MAX), gris, 1);
        std::snprintf(s, sizeof(s), "%d", i);
        Ecrire(img, t, X(d) - 5.f, Y(0.f) + 24.f, s, noir);
    }
    for (int32 i = 0; i <= 5; ++i) {
        float32 g = 0.2f * i;
        Ligne(img, X(0.f), Y(g), X(DISTANCE_MAX), Y(g), gris, 1);
        std::snprintf(s, sizeof(s), "%.1f", g);
        Ecrire(img, t, X(0.f) - 40.f, Y(g) + 6.f, s, noir);
    }
    Ligne(img, X(0.f), Y(0.f), X(DISTANCE_MAX), Y(0.f), noir, 2);
    Ligne(img, X(0.f), Y(0.f), X(0.f), Y(GAIN_MAX), noir, 2);
    Ecrire(img, t, X(4.f) - 60.f, (float32)IMAGE_H - 20.f, "distance (m)", noir);
    Ecrire(img, t, 10.f, (float32)MARGE_HAUT - 12.f, "gain (rapporte a NONE)", noir);
}

static void Courbe(NkImage& img, const float32 gains[NB_DISTANCES], math::NkColor c,
                   int32 epaisseur) {
    for (int32 i = 0; i + 1 < NB_DISTANCES; ++i) {
        float32 xa = X(DISTANCES[i]);
        float32 ya = Y(gains[i]);
        float32 xb = X(DISTANCES[i + 1]);
        float32 yb = Y(gains[i + 1]);
        Ligne(img, xa, ya, xb, yb, c, epaisseur);
    }
    for (int32 i = 0; i < NB_DISTANCES; ++i) {
        float32 x = X(DISTANCES[i]);
        float32 y = Y(gains[i]);
        Ligne(img, x - 4.f, y, x + 4.f, y, c, 9);
    }
}

static bool TracerCourbes(const float32 gains[NB_MODELES][NB_DISTANCES], const char* fichier) {
    Texte t;
    t.police = NkFontEmbedded::AddToAtlas(t.atlas, NkEmbeddedFontId::Inter, 18.f);
    if (!t.police || !t.atlas.Build()) {
        return false;
    }
    t.atlas.GetTexDataAsAlpha8(&t.pixels, &t.w, &t.h);
    if (!t.pixels) {
        return false;
    }

    NkImage img = NkImage::Create(IMAGE_L, IMAGE_H, 4, 0xFFFFFFFF);
    math::NkColor noir(30, 30, 30);
    const math::NkColor couleurs[NB_MODELES] = {
        math::NkColor(120, 120, 120),
        math::NkColor(31, 119, 180),
        math::NkColor(214, 39, 40),
        math::NkColor(44, 160, 44),
    };
    Axes(img, t, noir);
    for (int32 m = 0; m < NB_MODELES; ++m) {
        int32 epaisseur = 3;
        if (MODELES[m] == AttenuationModel::INVERSE) {
            epaisseur = 9;
        }
        Courbe(img, gains[m], couleurs[m], epaisseur);
        float32 lx = X(5.2f);
        float32 ly = Y(0.80f) + 26.f * m;
        Ligne(img, lx, ly - 6.f, lx + 30.f, ly - 6.f, couleurs[m], 3);
        Ecrire(img, t, lx + 40.f, ly, NOMS[m], noir);
    }
    Ecrire(img, t, X(5.2f), Y(0.80f) + 26.f * NB_MODELES, "rolloff 1, min 1 m, max 100 m", noir);
    return img.SavePNG(fichier);
}

static int Mesures() {
    AudioEngineConfig cfg;
    cfg.sampleRate = 48000;
    cfg.backend = AudioBackendType::NULL_OUTPUT;
    AudioEngine& moteur = AudioEngine::Instance();
    if (!moteur.Initialize(cfg)) {
        std::printf("Le moteur de son ne s'est pas initialise\n");
        return 1;
    }
    moteur.SetListenerPosition(0.f, 0.f, 0.f);
    moteur.SetListenerOrientation(0.f, 0.f, -1.f, 0.f, 1.f, 0.f);
    AudioSample son = AudioGenerator::GenerateTone(440.f, 2.f, WaveformType::SINE, 48000, 0.1f);

    std::FILE* f = std::fopen("mesures.txt", "w");
    if (!f) {
        moteur.Shutdown();
        AudioLoader::Free(son);
        return 2;
    }
    float32 courbes[NB_MODELES][NB_DISTANCES] = {};
    const float32 rolloffs[2] = {1.f, 2.f};
    for (float32 rolloff : rolloffs) {
        for (int32 i = 0; i < NB_DISTANCES; ++i) {
            float32 d = DISTANCES[i];
            float32 rms[NB_MODELES] = {};
            for (int32 m = 0; m < NB_MODELES; ++m) {
                rms[m] = MesurerModele(moteur, son, MODELES[m], d, rolloff);
            }
            float32 reference = rms[0];
            char distance[16];
            std::snprintf(distance, sizeof(distance), d < 1.f ? "%.1f" : "%.0f", d);
            for (int32 m = 0; m < NB_MODELES; ++m) {
                float32 gain = 0.f;
                if (reference > 0.f) {
                    gain = rms[m] / reference;
                }
                if (rolloff == 1.f) {
                    courbes[m][i] = gain;
                }
                const char* format = "%s, rolloff %.0f, %s m : %.3f\n";
                std::fprintf(f, format, NOMS[m], rolloff, distance, gain);
                std::printf(format, NOMS[m], rolloff, distance, gain);
            }
        }
    }
    std::fclose(f);
    moteur.Shutdown();
    AudioLoader::Free(son);

    if (!TracerCourbes(courbes, "courbe.png")) {
        std::printf("courbe.png n'a pas pu etre ecrit\n");
        return 3;
    }
    std::printf("Mesures ecrites dans mesures.txt, courbes dans courbe.png\n");
    return 0;
}

static int Ecoute() {
    AudioEngineConfig cfg;
    cfg.sampleRate = 48000;
    AudioEngine& moteur = AudioEngine::Instance();
    if (!moteur.Initialize(cfg)) {
        std::printf("Le moteur de son ne s'est pas initialise\n");
        return 1;
    }
    std::printf("Sortie sonore : %s\n", moteur.GetBackendName());
    moteur.SetListenerPosition(0.f, 0.f, 0.f);
    moteur.SetListenerOrientation(0.f, 0.f, -1.f, 0.f, 1.f, 0.f);
    AudioSample son = AudioGenerator::GenerateTone(440.f, 2.f, WaveformType::SINE, 48000, 0.1f);

    const AttenuationModel modeles[2] = {AttenuationModel::INVERSE, AttenuationModel::LINEAR};
    const char* const noms[2] = {"INVERSE", "LINEAR"};
    const float32 distances[2] = {1.f, 8.f};
    for (int32 m = 0; m < 2; ++m) {
        for (int32 i = 0; i < 2; ++i) {
            std::printf("%s a %.0f m...\n", noms[m], distances[i]);
            AudioHandle voix = moteur.Play(son, Reglages(modeles[m], distances[i], 1.f));
            std::this_thread::sleep_for(std::chrono::seconds(4));
            moteur.Stop(voix);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    moteur.Shutdown();
    AudioLoader::Free(son);
    return 0;
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc > 1 && std::strcmp(argv[1], "ecoute") == 0) {
        return Ecoute();
    }
    return Mesures();
}
