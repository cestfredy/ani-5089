#include <iostream>
#include <map>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    const std::map<std::string, std::string> causes = {
        {"BLOQUE_AU_WAIT", "FIN DE TRAME"},
        {"PAS_DE_REPRISE", "FIN SANS RENDU"},
        {"OEIL_NOIR", "CHAINE MANQUANTE"},
        {"TREMBLEMENT", "MAUVAIS INSTANT"},
        {"CORRECTION_DE_TRAVERS", "POSE FRAICHE"},
        {"SCENE_DECALEE", "MAUVAIS ESPACE"},
    };
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    int diagnostiques = 0;
    int inconnus = 0;
    for (int i = 0; i < n; ++i) {
        std::string symptome;
        if (!(std::cin >> symptome)) {
            break;
        }
        auto cause = causes.find(symptome);
        if (cause == causes.end()) {
            std::cout << symptome << " INCONNU\n";
            ++inconnus;
        } else {
            std::cout << symptome << ' ' << cause->second << '\n';
            ++diagnostiques;
        }
    }
    std::cout << "DIAGNOSTIQUES " << diagnostiques << '\n';
    std::cout << "INCONNUS " << inconnus << '\n';
    return 0;
}
