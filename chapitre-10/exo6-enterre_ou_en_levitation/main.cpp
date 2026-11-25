#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long hauteur = 0;
    int n = 0;
    if (!(std::cin >> hauteur >> n)) {
        n = 0;
    }
    int decalees = 0;
    for (int i = 0; i < n; ++i) {
        std::string nom;
        std::string pose;
        std::string couche;
        if (!(std::cin >> nom >> pose >> couche)) {
            break;
        }
        long long decalage = 0;
        std::string verdict = "OK";
        if (pose == "STAGE" && couche == "LOCAL") {
            decalage = hauteur;
            verdict = "LEVITATION";
        } else if (pose == "LOCAL" && couche == "STAGE") {
            decalage = -hauteur;
            verdict = "ENTERRE";
        }
        if (verdict != "OK") {
            ++decalees;
        }
        std::cout << nom << ' ' << decalage << ' ' << verdict << '\n';
    }
    std::cout << "DECALEES " << decalees << '\n';
    return 0;
}
