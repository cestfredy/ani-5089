#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string ligne;
    int n = 0;
    if (std::getline(std::cin, ligne)) {
        std::istringstream flux(ligne);
        flux >> n;
    }

    for (int i = 0; i < n; ++i) {
        if (!std::getline(std::cin, ligne)) {
            ligne.clear();
        }

        bool avant = false;
        bool arriere = false;
        bool gauche = false;
        bool droite = false;
        std::istringstream flux(ligne);
        std::string touche;
        while (flux >> touche) {
            if (touche == "W" || touche == "Z") {
                avant = true;
            } else if (touche == "S") {
                arriere = true;
            } else if (touche == "A" || touche == "Q") {
                gauche = true;
            } else if (touche == "D") {
                droite = true;
            }
        }

        int avance = 0;
        if (avant) {
            avance++;
        }
        if (arriere) {
            avance--;
        }
        int cote = 0;
        if (droite) {
            cote++;
        }
        if (gauche) {
            cote--;
        }
        std::cout << avance << ' ' << cote << '\n';
    }
    return 0;
}
