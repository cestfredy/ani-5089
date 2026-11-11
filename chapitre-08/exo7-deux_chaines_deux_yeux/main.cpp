#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

struct Anneau {
    int suivant = 0;
    bool emprunte = false;
    int index = 0;
    int rendus = 0;
};

static bool Emprunter(Anneau& a, int images) {
    if (a.emprunte || images <= 0) {
        return false;
    }
    a.emprunte = true;
    a.index = a.suivant;
    return true;
}

static bool Rendre(Anneau& a, int images) {
    if (!a.emprunte) {
        return false;
    }
    a.emprunte = false;
    ++a.rendus;
    a.suivant = (a.suivant + 1) % images;
    return true;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int images = 0;
    int n = 0;
    if (!(std::cin >> images >> n)) {
        return 0;
    }
    Anneau gauche;
    Anneau droit;
    int desync = 0;
    for (int tour = 1; tour <= n; ++tour) {
        std::string oeil;
        std::string commande;
        if (!(std::cin >> oeil >> commande)) {
            break;
        }
        Anneau& a = oeil == "G" ? gauche : droit;
        bool accepte = false;
        if (commande == "EMPRUNTE") {
            accepte = Emprunter(a, images);
        } else {
            accepte = Rendre(a, images);
        }
        if (accepte) {
            std::printf("%d %s %s %d\n", tour, oeil.c_str(), commande.c_str(), a.index);
        } else {
            std::printf("%d %s %s REFUSE\n", tour, oeil.c_str(), commande.c_str());
        }
        int difference = std::abs(gauche.rendus - droit.rendus);
        if (desync == 0 && difference > 1) {
            desync = tour;
        }
    }
    std::printf("DECALAGE %d\n", std::abs(gauche.rendus - droit.rendus));
    std::printf("DESYNC %d\n", desync);
    return 0;
}
