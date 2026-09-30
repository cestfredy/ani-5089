#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Appareil {
    std::string serie;
    std::string etat;
    std::string modele;
};

int main() {
    int d = 0;
    std::cin >> d;
    std::vector<Appareil> appareils;
    for (int i = 0; i < d; ++i) {
        Appareil a;
        if (!(std::cin >> a.serie >> a.etat >> a.modele)) {
            break;
        }
        appareils.push_back(a);
    }
    std::string cible = "-";
    std::cin >> cible;

    if (cible != "-") {
        for (const auto& a : appareils) {
            if (a.serie != cible) {
                continue;
            }
            if (a.etat != "device") {
                std::cout << "ERREUR " << a.serie << " est " << a.etat << '\n';
            } else {
                std::cout << a.serie << '\n';
            }
            return 0;
        }
        std::cout << "ERREUR cible introuvable\n";
        return 0;
    }

    std::vector<std::string> prets;
    for (const auto& a : appareils) {
        if (a.etat == "device") {
            prets.push_back(a.serie);
        }
    }
    if (prets.empty()) {
        std::cout << "ERREUR aucun appareil\n";
    } else if (prets.size() == 1) {
        std::cout << prets[0] << '\n';
    } else {
        std::sort(prets.begin(), prets.end());
        std::cout << "ERREUR plusieurs appareils\n";
        for (const auto& s : prets) {
            std::cout << s << '\n';
        }
    }
    return 0;
}
