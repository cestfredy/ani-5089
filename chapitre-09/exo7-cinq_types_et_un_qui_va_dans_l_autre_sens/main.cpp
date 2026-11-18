#include <iostream>
#include <map>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int a = 0;
    if (!(std::cin >> a)) {
        a = 0;
    }
    std::map<std::string, bool> estSortie;
    int entrees = 0;
    int sorties = 0;
    for (int i = 0; i < a; ++i) {
        std::string nom;
        std::string type;
        if (!(std::cin >> nom >> type)) {
            break;
        }
        bool sortie = type == "HAPTIC";
        estSortie[nom] = sortie;
        if (sortie) {
            ++sorties;
        } else {
            ++entrees;
        }
    }
    int o = 0;
    if (!(std::cin >> o)) {
        o = 0;
    }
    for (int i = 0; i < o; ++i) {
        std::string nom;
        std::string operation;
        if (!(std::cin >> nom >> operation)) {
            break;
        }
        auto action = estSortie.find(nom);
        if (action == estSortie.end()) {
            continue;
        }
        if (operation == "LIT" && action->second) {
            std::cout << nom << " LIT UNE SORTIE\n";
        } else if (operation == "ECRIT" && !action->second) {
            std::cout << nom << " ECRIT UNE ENTREE\n";
        }
    }
    std::cout << "ENTREES " << entrees << '\n';
    std::cout << "SORTIES " << sorties << '\n';
    return 0;
}
