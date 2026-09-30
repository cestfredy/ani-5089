#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

static bool CommencePar(const std::string& s, const std::string& debut) {
    return s.size() >= debut.size() && s.compare(0, debut.size(), debut) == 0;
}

int main() {
    std::string ligne;
    std::vector<std::pair<std::string, std::string>> prefixes;

    int p = 0;
    if (LireLigne(ligne)) {
        p = std::stoi(ligne);
    }
    for (int i = 0; i < p; ++i) {
        if (!LireLigne(ligne)) {
            break;
        }
        std::istringstream flux(ligne);
        std::string prefixe;
        std::string module;
        if (flux >> prefixe >> module) {
            prefixes.push_back({prefixe, module});
        }
    }

    int l = 0;
    if (LireLigne(ligne)) {
        l = std::stoi(ligne);
    }

    std::string marque = "undefined reference to '";
    std::set<std::string> modules;
    std::set<std::string> inconnus;

    for (int i = 0; i < l; ++i) {
        if (!std::getline(std::cin, ligne)) {
            break;
        }
        std::size_t pos = ligne.find(marque);
        while (pos != std::string::npos) {
            std::size_t debut = pos + marque.size();
            std::size_t fin = ligne.find('\'', debut);
            if (fin == std::string::npos) {
                break;
            }
            std::string symbole = ligne.substr(debut, fin - debut);

            std::string meilleur;
            std::size_t longueur = 0;
            for (const auto& paire : prefixes) {
                if (paire.first.size() > longueur && CommencePar(symbole, paire.first)) {
                    longueur = paire.first.size();
                    meilleur = paire.second;
                }
            }
            if (longueur > 0) {
                modules.insert(meilleur);
            } else {
                inconnus.insert(symbole);
            }
            pos = ligne.find(marque, fin + 1);
        }
    }

    for (const auto& module : modules) {
        std::cout << module << '\n';
    }
    if (!inconnus.empty()) {
        std::cout << "INCONNU " << inconnus.size() << '\n';
    }
    return 0;
}
