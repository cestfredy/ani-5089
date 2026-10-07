#include "raylib.h"
#include <chrono>
#include <cstdio>

int main() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "c1-exo6");
    SetTargetFPS(0);

    int chauffe = 100; // on ignore les premieres images
    int n = 1000;
    double seuil = 11.0;

    double partCode = 1000.0 / 90.0 - 8.0;

    double pire = 0;
    double pireChauffe = 0;
    int depasse = 0;
    int depassePartCode = 0;
    int image = 0;

    auto avant = std::chrono::steady_clock::now();
    while (!WindowShouldClose() && image < chauffe + n) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();

        auto maintenant = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(maintenant - avant).count();
        avant = maintenant;
        image++;

        if (image <= chauffe) {
            if (ms > pireChauffe) pireChauffe = ms;
        } else {
            if (ms > pire) pire = ms;
            if (ms > seuil) depasse++;
            if (ms > partCode) depassePartCode++;
        }
    }
    CloseWindow();

    printf("pire image de la chauffe (100 premieres) : %.2f ms\n", pireChauffe);
    printf("images mesurees : %d\n", image - chauffe);
    printf("pire image : %.2f ms\n", pire);
    printf("images > 11 ms : %d\n", depasse);
    printf("images > %.1f ms : %d\n", partCode, depassePartCode);
    return 0;
}
