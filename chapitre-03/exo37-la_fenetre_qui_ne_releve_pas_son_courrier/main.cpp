#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long p = 0;
    int f = 0;
    std::cin >> p >> f;

    long long sansReleve = 0;
    int premier = 0;
    for (int i = 1; i <= f; ++i) {
        std::string tour;
        if (!(std::cin >> tour)) {
            break;
        }
        if (tour == "releve") {
            sansReleve = 0;
        } else {
            sansReleve++;
        }

        bool morte = sansReleve > 0 && sansReleve >= p;
        if (morte && premier == 0) {
            premier = i;
        }
        std::cout << sansReleve << ' ' << (morte ? "MORTE" : "VIVANTE") << '\n';
    }
    std::cout << "PREMIER " << premier << '\n';
    return 0;
}
