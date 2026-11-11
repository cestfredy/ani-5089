#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct Anneau {
    std::vector<std::uint64_t> valeurs;
    std::vector<bool> posees;
    long long nombrePosees = 0;

    explicit Anneau(long long images) {
        if (images > 0) {
            valeurs.assign((size_t)images, 0u);
            posees.assign((size_t)images, false);
        }
    }

    bool Dedans(long long index) const {
        return index >= 0 && index < (long long)valeurs.size();
    }

    bool Poser(long long index, std::uint64_t valeur) {
        if (!Dedans(index)) {
            return false;
        }
        if (!posees[(size_t)index]) {
            posees[(size_t)index] = true;
            ++nombrePosees;
        }
        valeurs[(size_t)index] = valeur;
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
    Anneau anneau(images);
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
        bool lisible = anneau.Dedans(index) && anneau.posees[(size_t)index];
        if (lisible) {
            std::cout << anneau.valeurs[(size_t)index] << '\n';
        } else {
            std::cout << "VIDE\n";
        }
    }
    std::cout << "POSEES " << anneau.nombrePosees << '\n';
    return 0;
}
