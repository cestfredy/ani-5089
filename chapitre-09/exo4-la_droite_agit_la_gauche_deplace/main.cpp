#include <iostream>
#include <set>
#include <sstream>
#include <string>

enum class Main { Droite, Gauche, Inconnue };

static Main MainDe(const std::string& usage) {
    static const std::set<std::string> connus = {
        "SELECT",       "GRAB",          "MENU",        "MOVE",           "AIM_POSE",
        "GRIP_POSE",    "HAPTIC",        "SELECT_LEFT", "GRAB_LEFT",      "AIM_POSE_LEFT",
        "GRIP_POSE_LEFT", "HAPTIC_LEFT", "MOVE_RIGHT",
    };
    if (connus.count(usage) == 0) {
        return Main::Inconnue;
    }
    const std::string suffixe = "_LEFT";
    bool gauche = usage.size() >= suffixe.size() &&
                  usage.compare(usage.size() - suffixe.size(), suffixe.size(), suffixe) == 0;
    if (gauche || usage == "MENU" || usage == "MOVE") {
        return Main::Gauche;
    }
    return Main::Droite;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    std::string ligne;
    std::getline(std::cin, ligne);
    int deuxMains = 0;
    int lues = 0;
    while (lues < n && std::getline(std::cin, ligne)) {
        std::istringstream mots(ligne);
        std::string nom;
        if (!(mots >> nom)) {
            continue;
        }
        int droite = 0;
        int gauche = 0;
        std::string usage;
        while (mots >> usage) {
            Main m = MainDe(usage);
            if (m == Main::Droite) {
                ++droite;
            } else if (m == Main::Gauche) {
                ++gauche;
            }
        }
        bool deux = droite > 0 && gauche > 0;
        if (deux) {
            ++deuxMains;
        }
        std::cout << nom << ' ' << droite << ' ' << gauche << ' '
                  << (deux ? "DEUX MAINS" : "UNE MAIN") << '\n';
        ++lues;
    }
    std::cout << "DEUX MAINS " << deuxMains << '\n';
    return 0;
}
