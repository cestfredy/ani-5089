#include <iostream>

static long long Absolu(long long v) {
    return v < 0 ? -v : v;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long seuil = 0;
    long long maximum = 0;
    int n = 0;
    if (!(std::cin >> seuil >> maximum >> n)) {
        std::cout << "MORTS 0\n";
        return 0;
    }
    long long course = 1000 - seuil;
    int morts = 0;
    for (int i = 0; i < n; ++i) {
        long long brut = 0;
        if (!(std::cin >> brut)) {
            break;
        }
        long long a = Absolu(brut);
        long long vitesse = 0;
        if (a < seuil) {
            ++morts;
        } else if (course > 0) {
            long long reste = a - seuil;
            vitesse = maximum * reste * reste / (course * course);
            if (brut < 0) {
                vitesse = -vitesse;
            }
        }
        std::cout << vitesse << '\n';
    }
    std::cout << "MORTS " << morts << '\n';
    return 0;
}
