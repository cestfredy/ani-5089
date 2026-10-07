#include <iostream>
#include <string>
#include <vector>

struct Rappel {
    long long id = 0;
    std::string type;
};

static void Retirer(std::vector<Rappel>& registre, long long id) {
    std::vector<Rappel> reste;
    for (const auto& r : registre) {
        if (r.id != id) {
            reste.push_back(r);
        }
    }
    registre = reste;
}

int main() {
    int n = 0;
    std::cin >> n;

    std::vector<Rappel> registre;
    for (int i = 0; i < n; ++i) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }
        if (commande == "poser") {
            Rappel r;
            std::cin >> r.id >> r.type;
            Retirer(registre, r.id);
            registre.push_back(r);
        } else if (commande == "retirer") {
            long long id = 0;
            std::cin >> id;
            Retirer(registre, id);
        } else if (commande == "envoyer") {
            std::string type;
            std::cin >> type;
            bool premier = true;
            for (const auto& r : registre) {
                if (r.type != type) {
                    continue;
                }
                if (!premier) {
                    std::cout << ' ';
                }
                std::cout << r.id;
                premier = false;
            }
            if (premier) {
                std::cout << "AUCUN";
            }
            std::cout << '\n';
        }
    }
    return 0;
}
