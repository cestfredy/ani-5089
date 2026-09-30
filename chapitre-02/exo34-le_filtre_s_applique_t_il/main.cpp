#include <iostream>
#include <set>
#include <string>
#include <utility>

static bool LireLigne(std::string& ligne) {
    while (std::getline(std::cin, ligne)) {
        if (!ligne.empty() && ligne.back() == '\r') {
            ligne.pop_back();
        }
        if (ligne.find_first_not_of(" \t") != std::string::npos) {
            return true;
        }
    }
    return false;
}

static std::string Nettoyer(const std::string& s) {
    std::size_t debut = s.find_first_not_of(" \t");
    if (debut == std::string::npos) {
        return "";
    }
    std::size_t fin = s.find_last_not_of(" \t");
    return s.substr(debut, fin - debut + 1);
}

using Machine = std::set<std::pair<std::string, std::string>>;

static bool EvaluerTerme(std::string terme, const Machine& machine) {
    bool inverse = false;
    terme = Nettoyer(terme);
    while (!terme.empty() && terme[0] == '!') {
        inverse = !inverse;
        terme = Nettoyer(terme.substr(1));
    }
    bool vrai = false;
    std::size_t egal = terme.find('=');
    if (egal != std::string::npos) {
        std::string cle = Nettoyer(terme.substr(0, egal));
        std::string valeur = Nettoyer(terme.substr(egal + 1));
        vrai = machine.count({cle, valeur}) > 0;
    }
    return inverse ? !vrai : vrai;
}

int main() {
    std::string ligne;
    Machine machine;

    int v = 0;
    if (LireLigne(ligne)) {
        v = std::stoi(ligne);
    }
    for (int i = 0; i < v; ++i) {
        if (!LireLigne(ligne)) {
            break;
        }
        std::size_t egal = ligne.find('=');
        if (egal == std::string::npos) {
            continue;
        }
        machine.insert({Nettoyer(ligne.substr(0, egal)), Nettoyer(ligne.substr(egal + 1))});
    }

    int f = 0;
    if (LireLigne(ligne)) {
        f = std::stoi(ligne);
    }
    for (int i = 0; i < f; ++i) {
        if (!LireLigne(ligne)) {
            ligne.clear();
        }
        bool applique = true;
        std::size_t debut = 0;
        while (true) {
            std::size_t et = ligne.find("&&", debut);
            std::size_t longueur = std::string::npos;
            if (et != std::string::npos) {
                longueur = et - debut;
            }
            if (!EvaluerTerme(ligne.substr(debut, longueur), machine)) {
                applique = false;
            }
            if (et == std::string::npos) {
                break;
            }
            debut = et + 2;
        }
        std::cout << (applique ? "OUI" : "NON") << '\n';
    }
    return 0;
}
