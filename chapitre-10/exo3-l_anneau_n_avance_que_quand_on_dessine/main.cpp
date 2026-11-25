#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    long long images = 0;
    int n = 0;
    if (!(std::cin >> images >> n)) {
        n = 0;
    }
    long long index = 0;
    int rendues = 0;
    int fins = 0;
    for (int i = 1; i <= n; ++i) {
        int dessiner = 0;
        if (!(std::cin >> dessiner)) {
            break;
        }
        if (dessiner != 0) {
            std::cout << i << " OUI 1 " << index << '\n';
            ++rendues;
            if (images > 0) {
                index = (index + 1) % images;
            }
        } else {
            std::cout << i << " NON 0 -\n";
        }
        ++fins;
    }
    std::cout << "RENDUES " << rendues << '\n';
    std::cout << "FINS " << fins << '\n';
    return 0;
}
