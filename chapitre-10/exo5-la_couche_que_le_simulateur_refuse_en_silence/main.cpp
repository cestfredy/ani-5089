#include <iostream>
#include <string>

static std::string Verdict(int commence, int rend, int gauche, int droite) {
    if (commence == 0) {
        return "REFUSEE";
    }
    bool aucune = gauche == 0 && droite == 0;
    if (rend == 0) {
        if (aucune) {
            return "OK";
        }
        return "COUCHE SANS RENDU";
    }
    if (aucune) {
        return "SOUMISSION VIDE";
    }
    if (gauche == 0) {
        return "OEIL NOIR G";
    }
    if (droite == 0) {
        return "OEIL NOIR D";
    }
    return "OK";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    int refusees = 0;
    int averties = 0;
    for (int i = 0; i < n; ++i) {
        std::string nom;
        int commence = 0;
        int rend = 0;
        int gauche = 0;
        int droite = 0;
        if (!(std::cin >> nom >> commence >> rend >> gauche >> droite)) {
            break;
        }
        std::string verdict = Verdict(commence, rend, gauche, droite);
        if (verdict == "REFUSEE") {
            ++refusees;
        } else if (verdict != "OK") {
            ++averties;
        }
        std::cout << nom << ' ' << verdict << '\n';
    }
    std::cout << "REFUSEES " << refusees << '\n';
    std::cout << "AVERTIES " << averties << '\n';
    return 0;
}
