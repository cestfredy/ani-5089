#include <iostream>
#include <string>

int main() {
    int n = 0;
    std::cin >> n;

    long long totalX = 0;
    long long totalY = 0;
    long long moteurX = 0;
    long long moteurY = 0;

    for (int i = 0; i < n; ++i) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }
        if (commande == "bouge") {
            long long dx = 0;
            long long dy = 0;
            std::cin >> dx >> dy;
            totalX += dx;
            totalY += dy;
            moteurX = dx;
            moteurY = dy;
        } else if (commande == "image") {
            std::cout << totalX << ' ' << totalY << ' ' << moteurX << ' ' << moteurY << '\n';
            totalX = 0;
            totalY = 0;
        }
    }
    return 0;
}
