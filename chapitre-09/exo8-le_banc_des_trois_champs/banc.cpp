#include "actions.h"

#include <cstdio>

static void Colonnes(const EtatAction& e) {
    std::printf(" |       %d       %d     %d", e.courant, e.change, e.actif);
}

int main() {
    bool toucheEspace = false;
    bool toucheE = false;
    Action saisir("saisir", "GRAB");
    Action menu("menu", "MENU");
    Action avancer("avancer", "MOVE");
    saisir.Lier(&toucheEspace);
    menu.Lier(&toucheE);
    avancer.Lier(nullptr);
    Action* actions[3] = {&saisir, &menu, &avancer};

    for (Action* a : actions) {
        a->Synchroniser();
        if (!a->Etat().actif) {
            std::printf("# action inactive : %s (%s)\n", a->Nom().c_str(), a->Usage().c_str());
        }
    }
    std::printf("image | saisir: courant change actif | menu: courant change actif"
                " | avancer: courant change actif\n");
    for (int image = 1; image <= 6; ++image) {
        toucheEspace = image >= 2 && image <= 4;
        toucheE = false;
        for (Action* a : actions) {
            a->Synchroniser();
        }
        std::printf("%5d", image);
        for (Action* a : actions) {
            Colonnes(a->Etat());
        }
        if (saisir.Saisie()) {
            std::printf("   <- saisie (front)");
        }
        if (saisir.Lachee()) {
            std::printf("   <- lacher (front)");
        }
        std::printf("\n");
    }
    return 0;
}
