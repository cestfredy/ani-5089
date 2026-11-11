#include <iostream>
#include <string>

struct Anneau {
    long long images = 0;
    long long suivant = 0;
    long long index = 0;
    bool emprunte = false;

    bool Emprunter() {
        if (emprunte || images <= 0) {
            return false;
        }
        index = suivant;
        emprunte = true;
        return true;
    }

    bool Rendre() {
        if (!emprunte) {
            return false;
        }
        emprunte = false;
        suivant = (suivant + 1) % images;
        return true;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    Anneau anneau;
    int n = 0;
    if (!(std::cin >> anneau.images >> n)) {
        return 0;
    }
    int prets = 0;
    for (int tour = 1; tour <= n; ++tour) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }
        bool accepte = false;
        if (commande == "EMPRUNTE") {
            accepte = anneau.Emprunter();
            if (accepte) {
                ++prets;
            }
        } else {
            accepte = anneau.Rendre();
        }
        std::cout << tour << ' ' << commande << ' ';
        if (accepte) {
            std::cout << anneau.index << '\n';
        } else {
            std::cout << "REFUSE\n";
        }
    }
    std::cout << "PRETS " << prets << '\n';
    return 0;
}
