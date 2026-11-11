#include <iostream>
#include <string>

struct Chaine {
    bool acquise = false;
    bool attendSoumission = false;
};

static const char* Appliquer(Chaine& c, const std::string& appel) {
    if (appel == "ACQUIERT") {
        if (c.acquise) {
            return "DEJA ACQUISE";
        }
        c.acquise = true;
        return "OK";
    }
    if (appel == "DESSINE") {
        if (!c.acquise) {
            return "PAS ACQUISE";
        }
        return "OK";
    }
    if (appel == "REND") {
        if (!c.acquise) {
            return "RIEN ACQUIS";
        }
        c.acquise = false;
        c.attendSoumission = true;
        return "OK";
    }
    if (!c.attendSoumission) {
        return "PAS RENDUE";
    }
    c.attendSoumission = false;
    return "OK";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }
    Chaine chaine;
    int fautes = 0;
    for (int tour = 1; tour <= n; ++tour) {
        std::string appel;
        if (!(std::cin >> appel)) {
            break;
        }
        std::string verdict = Appliquer(chaine, appel);
        if (verdict != "OK") {
            ++fautes;
        }
        std::cout << tour << ' ' << appel << ' ' << verdict << '\n';
    }
    std::cout << "FAUTES " << fautes << '\n';
    return 0;
}
