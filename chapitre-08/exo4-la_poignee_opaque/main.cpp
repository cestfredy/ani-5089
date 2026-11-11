#include <cstdint>
#include <iostream>
#include <map>
#include <string>

struct Anneau {
    long long images = 0;
    std::map<long long, std::uint64_t> valeurs;

    bool Dedans(long long index) const {
        return index >= 0 && index < images;
    }

    bool Poser(long long index, std::uint64_t valeur) {
        if (!Dedans(index)) {
            return false;
        }
        valeurs[index] = valeur;
        return true;
    }

    bool Lire(long long index, std::uint64_t& valeur) const {
        if (!Dedans(index)) {
            return false;
        }
        auto trouvee = valeurs.find(index);
        if (trouvee == valeurs.end()) {
            return false;
        }
        valeur = trouvee->second;
        return true;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long images = 0;
    int n = 0;
    if (!(std::cin >> images >> n)) {
        return 0;
    }
    Anneau anneau;
    anneau.images = images;
    for (int i = 0; i < n; ++i) {
        std::string commande;
        long long index = 0;
        if (!(std::cin >> commande >> index)) {
            break;
        }
        if (commande == "POSE") {
            std::uint64_t valeur = 0;
            if (!(std::cin >> valeur)) {
                break;
            }
            if (!anneau.Poser(index, valeur)) {
                std::cout << "HORS ANNEAU\n";
            }
            continue;
        }
        std::uint64_t valeur = 0;
        if (anneau.Lire(index, valeur)) {
            std::cout << valeur << '\n';
        } else {
            std::cout << "VIDE\n";
        }
    }
    std::cout << "POSEES " << anneau.valeurs.size() << '\n';
    return 0;
}
