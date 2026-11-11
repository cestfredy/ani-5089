#include <iostream>
#include <string>

struct Cible {
    std::string nom;
    int cible = 0;
    int taille = 0;
    long long lc = 0;
    long long hc = 0;
};

struct Verdict {
    long long largeur = 0;
    long long hauteur = 0;
    const char* texte = "";
    bool bloque = false;
};

static Verdict Juger(const Cible& c, long long lf, long long hf) {
    Verdict v;
    v.largeur = lf;
    v.hauteur = hf;
    if (c.cible == 0) {
        v.texte = "PAS RELIEE";
        return v;
    }
    if (c.taille == 1) {
        v.largeur = c.lc;
        v.hauteur = c.hc;
        v.texte = "OK";
        return v;
    }
    bool differe = c.lc != lf || c.hc != hf;
    if (differe) {
        v.texte = "BLOQUE";
        v.bloque = true;
    } else {
        v.texte = "SANS EFFET";
    }
    return v;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long lf = 0;
    long long hf = 0;
    int n = 0;
    if (!(std::cin >> lf >> hf >> n)) {
        return 0;
    }
    int bloquantes = 0;
    for (int i = 0; i < n; ++i) {
        Cible c;
        if (!(std::cin >> c.nom >> c.cible >> c.taille >> c.lc >> c.hc)) {
            break;
        }
        Verdict v = Juger(c, lf, hf);
        if (v.bloque) {
            ++bloquantes;
        }
        std::cout << c.nom << ' ' << v.largeur << ' ' << v.hauteur << ' ' << v.texte << '\n';
    }
    std::cout << "BLOQUANTES " << bloquantes << '\n';
    return 0;
}
