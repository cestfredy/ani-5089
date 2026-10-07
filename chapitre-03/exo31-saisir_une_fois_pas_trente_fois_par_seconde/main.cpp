#include <iostream>
#include <string>

int main() {
    int n = 0;
    std::cin >> n;

    int sansGarde = 0;
    for (int i = 0; i < n; ++i) {
        std::string evenement;
        if (!(std::cin >> evenement)) {
            break;
        }
        if (evenement == "enfonce") {
            sansGarde++;
            std::cout << "SAISIR\n";
        } else if (evenement == "repete") {
            sansGarde++;
            std::cout << "RIEN\n";
        } else if (evenement == "relache") {
            std::cout << "LACHER\n";
        } else {
            std::cout << "RIEN\n";
        }
    }
    std::cout << "SANS_GARDE " << sansGarde << '\n';
    return 0;
}
