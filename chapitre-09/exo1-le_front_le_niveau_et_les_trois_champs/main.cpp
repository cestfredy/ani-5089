#include <iostream>

struct EtatAction {
    int courant = 0;
    int change = 0;
    int actif = 0;
};

static EtatAction Suivant(const EtatAction& avant, int presse, int lie) {
    EtatAction e;
    e.actif = lie != 0 ? 1 : 0;
    e.courant = e.actif == 1 && presse != 0 ? 1 : 0;
    e.change = e.courant != avant.courant ? 1 : 0;
    return e;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }
    EtatAction etat;
    int fronts = 0;
    int niveaux = 0;
    for (int image = 1; image <= n; ++image) {
        int presse = 0;
        int lie = 0;
        if (!(std::cin >> presse >> lie)) {
            break;
        }
        etat = Suivant(etat, presse, lie);
        if (etat.courant == 1 && etat.change == 1) {
            ++fronts;
        }
        if (etat.courant == 1) {
            ++niveaux;
        }
        std::cout << image << ' ' << etat.courant << ' ' << etat.change << ' ' << etat.actif
                  << '\n';
    }
    std::cout << "FRONTS " << fronts << '\n';
    std::cout << "NIVEAUX " << niveaux << '\n';
    return 0;
}
