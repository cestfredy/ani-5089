#include <iostream>
#include <string>
#include <vector>

struct Commande {
    std::string nom;
    long long echelle = 0;
    long long seuil = 0;
};

int main() {
    int c = 0;
    std::cin >> c;
    if (c < 0) {
        c = 0;
    }
    std::vector<Commande> commandes(c);
    for (auto& commande : commandes) {
        std::cin >> commande.nom >> commande.echelle >> commande.seuil;
    }

    int t = 0;
    std::cin >> t;
    for (int tour = 0; tour < t; ++tour) {
        long long axe = 0;
        for (const auto& commande : commandes) {
            long long brut = 0;
            std::cin >> brut;
            long long contribution = brut * commande.echelle / 1000;
            long long absolue = contribution;
            if (absolue < 0) {
                absolue = -absolue;
            }
            if (absolue >= commande.seuil) {
                axe += contribution;
            }
        }
        std::cout << axe << '\n';
    }
    return 0;
}
