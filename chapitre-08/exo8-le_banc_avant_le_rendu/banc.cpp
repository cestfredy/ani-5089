#include "chaine.h"

#include <cstdio>

static int echecs = 0;

static void Verifier(bool ok, const char* quoi) {
    std::printf("%-46s %s\n", quoi, ok ? "OK" : "ECHEC");
    if (!ok) {
        ++echecs;
    }
}

static void SixTours(Chaine& chaine) {
    for (int tour = 1; tour <= 6; ++tour) {
        std::uint32_t index = 99;
        bool emprunt = chaine.Emprunter(index);
        bool rendu = chaine.Rendre();
        std::uint32_t soumise = chaine.DerniereRendue();
        unsigned long long poignee = (unsigned long long)chaine.Poignee(soumise);
        std::printf("tour %d : empruntee %u, rendue %s, soumise %u (poignee %llu)\n", tour, index,
                    rendu ? "oui" : "non", soumise, poignee);
        if (!emprunt || !rendu || soumise != index) {
            ++echecs;
        }
    }
    std::printf("bouclage : le tour 4 reprend l'image 0, comme le tour 1\n");
}

int main() {
    Chaine chaine;
    Verifier(chaine.Creer(3), "creer une chaine de 3 images");
    for (std::uint32_t i = 0; i < 3; ++i) {
        chaine.Enregistrer(i, 1000u + i);
    }
    SixTours(chaine);

    std::uint32_t index = 0;
    Verifier(chaine.Emprunter(index), "premier emprunt");
    Verifier(!chaine.Emprunter(index), "refus : emprunter deux fois de suite");
    Verifier(chaine.Rendre(), "rendre l'image empruntee");
    Verifier(!chaine.Rendre(), "refus : rendre sans avoir emprunte");
    Chaine tropGrande;
    bool refusTropGrande = !tropGrande.Creer(Chaine::kMaxImages + 1u);
    Verifier(refusTropGrande, "refus : creer une chaine de 5 images (max 4)");
    Verifier(!tropGrande.Creer(0u), "refus : creer une chaine de 0 image");

    std::printf("%s (%d echec(s))\n", echecs == 0 ? "BANC REUSSI" : "BANC EN ECHEC", echecs);
    if (echecs == 0) {
        return 0;
    }
    return 1;
}
