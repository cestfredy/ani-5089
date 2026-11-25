#include <iostream>
#include <string>

enum class Etat {
    Repos,
    Attendue,
    Commencee
};

static std::string Geste(Etat& etat, const std::string& geste) {
    if (geste == "ATTEND") {
        if (etat == Etat::Attendue) {
            return "DEJA ATTENDUE";
        }
        if (etat == Etat::Commencee) {
            return "TRAME EN COURS";
        }
        etat = Etat::Attendue;
        return "OK";
    }
    if (geste == "COMMENCE") {
        if (etat == Etat::Repos) {
            return "PAS ATTENDUE";
        }
        if (etat == Etat::Commencee) {
            return "DEJA COMMENCEE";
        }
        etat = Etat::Commencee;
        return "OK";
    }
    if (etat != Etat::Commencee) {
        return "PAS COMMENCEE";
    }
    etat = Etat::Repos;
    return "OK";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    Etat etat = Etat::Repos;
    int fautes = 0;
    int bloque = 0;
    for (int i = 1; i <= n; ++i) {
        std::string geste;
        if (!(std::cin >> geste)) {
            break;
        }
        std::string verdict = Geste(etat, geste);
        if (verdict != "OK") {
            ++fautes;
        }
        if (verdict == "TRAME EN COURS" && bloque == 0) {
            bloque = i;
        }
        std::cout << i << ' ' << geste << ' ' << verdict << '\n';
    }
    std::cout << "FAUTES " << fautes << '\n';
    std::cout << "BLOQUE " << bloque << '\n';
    return 0;
}
