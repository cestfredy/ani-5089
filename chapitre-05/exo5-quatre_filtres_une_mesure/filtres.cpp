#include "NKImage/Core/NkImage.h"
#include "NKTime/NkChrono.h"

#include <cstdio>

using namespace nkentseu;

const int32 REPETITIONS = 20;
const int32 COTE_SOURCE = 2048;
const int32 COTE_CIBLE = 256;
const int32 NB_FILTRES = 4;
const int32 BILINEAIRE = 1;

struct Essai {
    NkResizeFilter filtre;
    const char* nom;
    const char* fichier;
};

static int64 PixelsDifferents(const NkImage& a, const NkImage& b) {
    bool memeTaille = a.Width() == b.Width() && a.Height() == b.Height();
    if (!memeTaille) {
        return -1;
    }
    int32 largeur = (int32)a.Width();
    int32 hauteur = (int32)a.Height();
    int64 differents = 0;
    for (int32 y = 0; y < hauteur; ++y) {
        for (int32 x = 0; x < largeur; ++x) {
            math::NkColor pa = a.GetPixel(x, y);
            math::NkColor pb = b.GetPixel(x, y);
            bool differe = pa.r != pb.r || pa.g != pb.g || pa.b != pb.b || pa.a != pb.a;
            if (differe) {
                ++differents;
            }
        }
    }
    return differents;
}

static double Chronometrer(const NkImage& source, NkResizeFilter filtre) {
    int64 total = 0;
    for (int32 r = 0; r < REPETITIONS; ++r) {
        NkChrono chrono;
        NkImage reduite = source.Resize(COTE_CIBLE, COTE_CIBLE, filtre);
        total += chrono.Elapsed().ToMicroseconds();
        if (!reduite.IsValid()) {
            return -1.0;
        }
    }
    return (double)total / REPETITIONS;
}

int main(int argc, char** argv) {
    const char* chemin = "photo.png";
    if (argc > 1) {
        chemin = argv[1];
    }
    NkImage photo;
    if (!photo.Load(chemin) || !photo.IsValid()) {
        std::printf("Impossible de charger %s\n", chemin);
        return 1;
    }
    NkImage source = photo.Resize(COTE_SOURCE, COTE_SOURCE);
    if (!source.IsValid()) {
        std::printf("La source %d x %d n'a pas pu etre creee\n", COTE_SOURCE, COTE_SOURCE);
        return 2;
    }
    std::printf("photo %d x %d -> source %d x %d\n", photo.Width(), photo.Height(), source.Width(),
                source.Height());

    const Essai essais[NB_FILTRES] = {
        {NkResizeFilter::NK_NEAREST, "NK_NEAREST", "nearest.png"},
        {NkResizeFilter::NK_BILINEAR, "NK_BILINEAR", "bilinear.png"},
        {NkResizeFilter::NK_BICUBIC, "NK_BICUBIC", "bicubic.png"},
        {NkResizeFilter::NK_LANCZOS3, "NK_LANCZOS3", "lanczos3.png"},
    };
    NkImage resultats[NB_FILTRES];
    double moyennes[NB_FILTRES] = {};
    for (int32 i = 0; i < NB_FILTRES; ++i) {
        resultats[i] = source.Resize(COTE_CIBLE, COTE_CIBLE, essais[i].filtre);
        if (!resultats[i].IsValid() || !resultats[i].SavePNG(essais[i].fichier)) {
            std::printf("%s : la reduction ou l'enregistrement a echoue\n", essais[i].nom);
            return 3;
        }
        moyennes[i] = Chronometrer(source, essais[i].filtre);
    }

    std::FILE* f = std::fopen("mesures.txt", "w");
    if (!f) {
        return 4;
    }
    for (int32 i = 0; i < NB_FILTRES; ++i) {
        int64 d = PixelsDifferents(resultats[i], resultats[BILINEAIRE]);
        const char* format = "%s : %.0f us, differences : %lld\n";
        std::fprintf(f, format, essais[i].nom, moyennes[i], (long long)d);
        std::printf(format, essais[i].nom, moyennes[i], (long long)d);
    }
    std::fclose(f);
    std::printf("resultats %d x %d, soit %d pixels compares\n", resultats[0].Width(),
                resultats[0].Height(), COTE_CIBLE * COTE_CIBLE);
    return 0;
}
