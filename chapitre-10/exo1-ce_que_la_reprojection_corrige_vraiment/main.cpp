#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    int justes = 0;
    int pires = 0;
    for (int i = 0; i < n; ++i) {
        long long rendu = 0;
        long long reel = 0;
        long long soumis = 0;
        if (!(std::cin >> rendu >> reel >> soumis)) {
            break;
        }
        long long sans = rendu - reel;
        long long avec = rendu - soumis;
        std::string verdict;
        if (avec == 0) {
            verdict = "JUSTE";
            ++justes;
        } else if (std::llabs(avec) < std::llabs(sans)) {
            verdict = "PARTIEL";
        } else if (std::llabs(avec) == std::llabs(sans)) {
            verdict = "SANS EFFET";
        } else {
            verdict = "PIRE";
            ++pires;
        }
        std::cout << "SANS " << sans << " AVEC " << avec << ' ' << verdict << '\n';
    }
    std::cout << "JUSTES " << justes << '\n';
    std::cout << "PIRES " << pires << '\n';
    return 0;
}
