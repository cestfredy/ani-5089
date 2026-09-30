#include <iostream>
#include <string>

static bool CommencePar(const std::string& s, const std::string& debut) {
    return s.size() >= debut.size() && s.compare(0, debut.size(), debut) == 0;
}

static bool FinitPar(const std::string& s, const std::string& fin) {
    return s.size() >= fin.size() && s.compare(s.size() - fin.size(), fin.size(), fin) == 0;
}

int main() {
    std::string architecture;
    int f = 0;
    std::cin >> architecture >> f;

    std::string dossierAbi = "lib/" + architecture + "/";
    long long total = 0;
    bool signe = false;
    bool abi = false;
    int inutiles = 0;

    for (int i = 0; i < f; ++i) {
        std::string chemin;
        long long taille = 0;
        if (!(std::cin >> chemin >> taille)) {
            break;
        }
        total += taille;

        bool certificat = FinitPar(chemin, ".RSA") || FinitPar(chemin, ".DSA") || FinitPar(chemin, ".EC");
        if (CommencePar(chemin, "META-INF/") && certificat) {
            signe = true;
        }
        if (CommencePar(chemin, dossierAbi)) {
            abi = true;
        } else if (CommencePar(chemin, "lib/")) {
            ++inutiles;
        }
    }

    std::cout << total << '\n';
    std::cout << (signe ? "SIGNE" : "NON SIGNE") << '\n';
    std::cout << (abi ? "ABI OUI" : "ABI NON") << '\n';
    std::cout << "INUTILE " << inutiles << '\n';
    return 0;
}
