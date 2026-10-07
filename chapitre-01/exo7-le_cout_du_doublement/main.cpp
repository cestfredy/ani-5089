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
    double totalLogique = 0, totalEfface = 0, totalLot = 0, totalFin = 0;
    double pireRendu = 0;

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
        auto ta = std::chrono::steady_clock::now();
        for (auto& p : ps) DrawRectangle((int)p.x, (int)p.y, 4, 4, p.c);
        auto tb = std::chrono::steady_clock::now();
        EndDrawing();
        auto t2 = std::chrono::steady_clock::now();

        if (i >= chauffe) {
            totalLogique += ms(t0, t1);
            totalEfface += ms(t1, ta);
            totalLot += ms(ta, tb);
            totalFin += ms(tb, t2);
            double r = ms(t1, t2);
            if (r > pireRendu) pireRendu = r;
        }
    }
    CloseWindow();

    double logique = totalLogique / mesures;
    double efface = totalEfface / mesures;
    double lot = totalLot / mesures;
    double fin = totalFin / mesures;
    double rendu = efface + lot + fin;
    double partCode = 1000.0 / 90.0 - 8.0;

    double basse = rendu;
    double deuxPasses = rendu + efface + lot;
    double haute = 2 * rendu;

    printf("particules : %d\n", n);
    printf("logique seule : %.3f ms\n", logique);
    printf("rendu seul : %.3f ms (pire %.3f ms)\n", rendu, pireRendu);
    printf("  effacement : %.3f ms\n", efface);
    printf("  ajout des rectangles au lot : %.3f ms\n", lot);
    printf("  fin d'image (envoi du lot, dessin, presentation) : %.3f ms\n", fin);
    printf("rendu x2, borne basse (une passe) : %.3f ms\n", basse);
    printf("rendu x2, deux passes (effacement et lot refaits) : %.3f ms\n", deuxPasses);
    printf("rendu x2, borne haute (tout refait) : %.3f ms\n", haute);
    printf("reste sur %.1f ms : de %.3f a %.3f ms\n", partCode, partCode - haute, partCode - basse);
    return 0;
}
