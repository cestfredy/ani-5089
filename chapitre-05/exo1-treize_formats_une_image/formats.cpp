#include "NKImage/Core/NkImage.h"

#include <cstdio>

using namespace nkentseu;

const int32 NB_FORMATS = 5;

struct Format {
    const char* nom;
    const char* fichier;
};

static long TailleSurDisque(const char* chemin) {
    std::FILE* f = std::fopen(chemin, "rb");
    if (!f) {
        return 0;
    }
    std::fseek(f, 0, SEEK_END);
    long taille = std::ftell(f);
    std::fclose(f);
    return taille;
}

static bool Ecrire(const NkImage& photo, int32 i, const char* fichier) {
    switch (i) {
        case 0:
            return photo.SavePNG(fichier);
        case 1:
            return photo.SaveJPEG(fichier, 90);
        case 2:
            return photo.SaveBMP(fichier);
        case 3:
            return photo.SaveTGA(fichier);
        default:
            return photo.SaveQOI(fichier);
    }
}

int main(int argc, char** argv) {
    const char* source = "photo.png";
    if (argc > 1) {
        source = argv[1];
    }
    NkImage photo;
    if (!photo.Load(source) || !photo.IsValid()) {
        std::printf("Impossible de charger %s\n", source);
        return 1;
    }
    std::printf("photo : %s, %ld octets, %d x %d, %d canaux\n", source, TailleSurDisque(source),
                photo.Width(), photo.Height(), photo.Channels());

    const Format formats[NB_FORMATS] = {
        {"png", "sortie.png"},
        {"jpeg", "sortie.jpg"},
        {"bmp", "sortie.bmp"},
        {"tga", "sortie.tga"},
        {"qoi", "sortie.qoi"},
    };
    std::FILE* sortie = std::fopen("formats.txt", "w");
    if (!sortie) {
        std::printf("Impossible d'ecrire formats.txt\n");
        return 2;
    }
    for (int32 i = 0; i < NB_FORMATS; ++i) {
        bool ecrit = Ecrire(photo, i, formats[i].fichier);
        long octets = TailleSurDisque(formats[i].fichier);
        NkImage relue;
        bool lisible = relue.Load(formats[i].fichier) && relue.IsValid();
        char ligne[160];
        if (lisible) {
            const char* modele = "%s : %ld octets, %d x %d";
            std::snprintf(ligne, sizeof(ligne), modele, formats[i].nom, octets, relue.Width(),
                          relue.Height());
        } else {
            const char* modele = "%s : %ld octets, illisible";
            std::snprintf(ligne, sizeof(ligne), modele, formats[i].nom, octets);
        }
        std::fprintf(sortie, "%s\n", ligne);

        const char* dimensions = "-";
        if (lisible) {
            bool memes = relue.Width() == photo.Width() && relue.Height() == photo.Height();
            dimensions = memes ? "identiques a la photo" : "differentes de la photo";
        }
        std::printf("%s   (ecrit : %s, relu : %s, dimensions : %s)\n", ligne, ecrit ? "oui" : "non",
                    lisible ? "oui" : "non", dimensions);
    }
    std::fclose(sortie);
    std::printf("Resultats ecrits dans formats.txt\n");
    return 0;
}
