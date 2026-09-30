#include <iostream>
#include <map>
#include <string>

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

static bool EvaluerTerme(std::string terme, const std::map<std::string, std::string>& machine) {
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
        auto it = machine.find(cle);
        vrai = it != machine.end() && it->second == valeur;
    }
    return inverse ? !vrai : vrai;
}

int main() {
    std::string ligne;
    std::map<std::string, std::string> machine;

    int v = 0;
    if (LireLigne(ligne)) {
        v = std::stoi(ligne);
    }
    for (int i = 0; i < v && LireLigne(ligne); ++i) {
        std::size_t egal = ligne.find('=');
        if (egal == std::string::npos) {
            continue;
        }
        machine[Nettoyer(ligne.substr(0, egal))] = Nettoyer(ligne.substr(egal + 1));
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
