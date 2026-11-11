#include <iostream>
#include <string>

static bool Soumettre(long long& enVol, long long vol) {
    if (enVol >= vol) {
        return false;
    }
    ++enVol;
    return true;
}

static void Finir(long long& enVol) {
    if (enVol > 0) {
        --enVol;
    }
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long vol = 0;
    int n = 0;
    if (!(std::cin >> vol >> n)) {
        return 0;
    }
    long long enVol = 0;
    int bloque = 0;
    for (int tour = 1; tour <= n; ++tour) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }
        bool passe = true;
        if (commande == "SOUMET") {
            passe = Soumettre(enVol, vol);
        } else {
            Finir(enVol);
        }
        if (!passe && bloque == 0) {
            bloque = tour;
        }
        std::cout << tour << ' ' << commande << ' ' << enVol << ' ' << (passe ? "PASSE" : "ATTEND")
                  << '\n';
    }
    std::cout << "BLOQUE " << bloque << '\n';
    return 0;
}
