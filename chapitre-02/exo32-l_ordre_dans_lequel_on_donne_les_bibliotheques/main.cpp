#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
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

int main() {
    std::string ligne;
    std::map<std::string, std::set<std::string>> besoins;

    int n = 0;
    if (LireLigne(ligne)) {
        n = std::stoi(ligne);
    }
    for (int i = 0; i < n; ++i) {
        if (!LireLigne(ligne)) {
            break;
        }
        std::istringstream flux(ligne);
        std::string module;
        std::string besoin;
        flux >> module;
        auto& liste = besoins[module];
        while (flux >> besoin) {
            liste.insert(besoin);
        }
    }

    int m = 0;
    std::cin >> m;
    std::vector<std::string> aTraiter;
    std::set<std::string> liste;
    for (int i = 0; i < m; ++i) {
        std::string nom;
        if (!(std::cin >> nom)) {
            break;
        }
        if (liste.insert(nom).second) {
            aTraiter.push_back(nom);
        }
    }

    while (!aTraiter.empty()) {
        std::string courant = aTraiter.back();
        aTraiter.pop_back();
        auto it = besoins.find(courant);
        if (it == besoins.end()) {
            continue;
        }
        for (const auto& b : it->second) {
            if (liste.insert(b).second) {
                aTraiter.push_back(b);
            }
        }
    }

    std::map<std::string, int> compte;
    for (const auto& module : liste) {
        compte[module] = 0;
    }
    for (const auto& module : liste) {
        auto it = besoins.find(module);
        if (it == besoins.end()) {
            continue;
        }
        for (const auto& b : it->second) {
            compte[b]++;
        }
    }

    std::set<std::string> prets;
    for (const auto& paire : compte) {
        if (paire.second == 0) {
            prets.insert(paire.first);
        }
    }
    std::vector<std::string> ordre;
    while (!prets.empty()) {
        std::string courant = *prets.begin();
        prets.erase(prets.begin());
        ordre.push_back(courant);
        auto it = besoins.find(courant);
        if (it == besoins.end()) {
            continue;
        }
        for (const auto& b : it->second) {
            compte[b]--;
            if (compte[b] == 0) {
                prets.insert(b);
            }
        }
    }

    if (ordre.size() != liste.size()) {
        std::cout << "CYCLE\n";
        return 0;
    }
    for (const auto& module : ordre) {
        std::cout << module << '\n';
    }
    return 0;
}
