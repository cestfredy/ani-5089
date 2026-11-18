#include <iostream>
#include <string>

static const char* Diagnostic(const std::string& intention, const std::string& pose) {
    if (intention == "TENIR" && pose == "AIM") {
        return "FLOTTE";
    }
    if (intention == "POINTER" && pose == "GRIP") {
        return "PART DE TRAVERS";
    }
    return "OK";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }
    int fautes = 0;
    for (int i = 0; i < n; ++i) {
        std::string nom;
        std::string intention;
        std::string pose;
        if (!(std::cin >> nom >> intention >> pose)) {
            break;
        }
        std::string verdict = Diagnostic(intention, pose);
        if (verdict != "OK") {
            ++fautes;
        }
        std::cout << nom << ' ' << verdict << '\n';
    }
    std::cout << "FAUTES " << fautes << '\n';
    return 0;
}
