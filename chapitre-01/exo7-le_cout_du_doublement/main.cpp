#include "raylib.h"
#include <chrono>
#include <cstdio>
#include <vector>

struct Particule {
    float x, y, vx, vy;
    Color c;
};

double ms(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    int largeur = 1280, hauteur = 720;
    int n = 5000;
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(largeur, hauteur, "c1-exo7");
    SetTargetFPS(0);

    std::vector<Particule> ps(n);
    for (auto& p : ps) {
        p.x = GetRandomValue(0, largeur);
        p.y = GetRandomValue(0, hauteur);
        p.vx = GetRandomValue(-200, 200);
        p.vy = GetRandomValue(-200, 200);
        p.c = Color{(unsigned char)GetRandomValue(80, 255), (unsigned char)GetRandomValue(80, 255), 200, 255};
    }

    int chauffe = 100;
    int mesures = 1000;
    double totalLogique = 0, totalRendu = 0, pireRendu = 0;

    for (int i = 0; i < chauffe + mesures && !WindowShouldClose(); i++) {
        auto t0 = std::chrono::steady_clock::now();
        for (auto& p : ps) {
            p.x += p.vx / 90.0f;
            p.y += p.vy / 90.0f;
            if (p.x < 0 || p.x > largeur) p.vx = -p.vx;
            if (p.y < 0 || p.y > hauteur) p.vy = -p.vy;
        }
        auto t1 = std::chrono::steady_clock::now();

        BeginDrawing();
        ClearBackground(BLACK);
        for (auto& p : ps) DrawRectangle((int)p.x, (int)p.y, 4, 4, p.c);
        EndDrawing();
        auto t2 = std::chrono::steady_clock::now();

        if (i >= chauffe) {
            totalLogique += ms(t0, t1);
            double r = ms(t1, t2);
            totalRendu += r;
            if (r > pireRendu) pireRendu = r;
        }
    }
    CloseWindow();

    double logique = totalLogique / mesures;
    double rendu = totalRendu / mesures;
    double partCode = 1000.0 / 90.0 - 8.0;

    printf("particules : %d\n", n);
    printf("logique seule : %.3f ms\n", logique);
    printf("rendu seul : %.3f ms (pire %.3f ms)\n", rendu, pireRendu);
    printf("rendu x2 estime : %.3f ms\n", 2 * rendu);
    printf("reste sur %.1f ms : %.3f ms\n", partCode, partCode - 2 * rendu);
    return 0;
}
