#include <iostream>
#include <string>

static const char* Verdict(long long pixels, long long seuil) {
    if (pixels >= seuil) {
        return "LISIBLE";
    }
    return "FLOU";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long rendements[3] = {1000, 1000, 1000};
    long long seuil = 0;
    int n = 0;
    if (!(std::cin >> rendements[0] >> rendements[1] >> rendements[2] >> seuil >> n)) {
        n = 0;
    }
    int gagnes = 0;
    int perdus = 0;
    for (int i = 0; i < n; ++i) {
        std::string nom;
        long long pixels = 0;
        if (!(std::cin >> nom >> pixels)) {
            break;
        }
        long long scene = pixels;
        for (long long rendement : rendements) {
            scene = scene * rendement / 1000;
        }
        bool sceneLisible = scene >= seuil;
        bool quadLisible = pixels >= seuil;
        if (!sceneLisible && quadLisible) {
            ++gagnes;
        }
        if (sceneLisible && !quadLisible) {
            ++perdus;
        }
        std::cout << nom << ' ' << scene << ' ' << pixels << ' ' << Verdict(scene, seuil) << ' '
                  << Verdict(pixels, seuil) << '\n';
    }
    std::cout << "GAGNES " << gagnes << '\n';
    std::cout << "PERDUS " << perdus << '\n';
    return 0;
}
