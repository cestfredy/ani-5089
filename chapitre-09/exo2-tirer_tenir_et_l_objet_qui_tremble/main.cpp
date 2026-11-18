#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    int precedent = 0;
    long long fronts = 0;
    long long niveaux = 0;
    for (int image = 1; image <= n; ++image) {
        int presse = 0;
        if (!(std::cin >> presse)) {
            break;
        }
        int courant = presse != 0 ? 1 : 0;
        if (courant == 1 && precedent == 0) {
            ++fronts;
        }
        if (courant == 1) {
            ++niveaux;
        }
        precedent = courant;
    }
    std::cout << "BALLES FRONT " << fronts << '\n';
    std::cout << "BALLES NIVEAU " << niveaux << '\n';
    std::cout << "TENU NIVEAU " << niveaux << '\n';
    std::cout << "TENU FRONT " << fronts << '\n';
    return 0;
}
