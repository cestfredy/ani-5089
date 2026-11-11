#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

struct Programme {
    std::string nom;
    int detruit = -1;
    int recree = -1;
    bool reenregistre = false;
};

static Programme Lire(const std::string& ligne) {
    Programme p;
    std::istringstream mots(ligne);
    mots >> p.nom;
    std::string mot;
    int position = 0;
    while (mots >> mot) {
        if (mot == "DETRUIT" && p.detruit < 0) {
            p.detruit = position;
        } else if (mot == "RECREE" && p.recree < 0) {
            p.recree = position;
        } else if (mot == "REENREGISTRE") {
            p.reenregistre = true;
        }
        ++position;
    }
    return p;
}

static const char* Verdict(const Programme& p) {
    bool lesDeux = p.detruit >= 0 && p.recree >= 0;
    if (lesDeux && p.recree < p.detruit) {
        return "MAUVAIS ORDRE";
    }
    if (!lesDeux) {
        return "CHAINE PERIMEE";
    }
    if (!p.reenregistre) {
        return "POIGNEES PERDUES";
    }
    return "SURVIT";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }
    std::string ligne;
    std::getline(std::cin, ligne);
    int survivants = 0;
    int lus = 0;
    while (lus < n && std::getline(std::cin, ligne)) {
        if (ligne.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        Programme p = Lire(ligne);
        const char* verdict = Verdict(p);
        if (std::string(verdict) == "SURVIT") {
            ++survivants;
        }
        std::cout << p.nom << ' ' << verdict << '\n';
        ++lus;
    }
    std::cout << "SURVIVANTS " << survivants << '\n';
    return 0;
}
