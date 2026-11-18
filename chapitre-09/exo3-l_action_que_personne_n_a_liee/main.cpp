#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int a = 0;
    if (!(std::cin >> a)) {
        a = 0;
    }
    std::vector<std::pair<std::string, std::string>> actions;
    for (int i = 0; i < a; ++i) {
        std::string nom;
        std::string usage;
        if (!(std::cin >> nom >> usage)) {
            break;
        }
        actions.emplace_back(nom, usage);
    }
    int l = 0;
    if (!(std::cin >> l)) {
        l = 0;
    }
    std::set<std::string> fournis;
    for (int i = 0; i < l; ++i) {
        std::string usage;
        if (!(std::cin >> usage)) {
            break;
        }
        fournis.insert(usage);
    }
    int mortes = 0;
    for (const auto& action : actions) {
        bool active = fournis.count(action.second) > 0;
        if (!active) {
            ++mortes;
        }
        std::cout << action.first << ' ' << (active ? "ACTIVE" : "MORTE") << '\n';
    }
    std::cout << "MORTES " << mortes << '\n';
    return 0;
}
