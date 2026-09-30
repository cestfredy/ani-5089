# c1-exo7 : Le coût du doublement

## Réponse

J'ai repris le programme de l'exo 6 en ajoutant 5000 particules qui bougent, pour avoir une logique et un rendu à séparer. J'ai lancé le programme trois fois ; les chiffres ci-dessous viennent du premier essai, les deux autres donnent presque la même chose.

| | Temps |
|---|:---:|
| Rendu seul (mesuré) | 0,87 ms (pire 3,78 ms) |
| Rendu fait deux fois (estimé, 2 × rendu) | 1,75 ms |
| Reste pour le reste sur 3,1 ms (part du code à 90 Hz, exo 1) | 1,37 ms |
| Logique seule (mesurée) | 0,016 ms |

En moyenne ça passe : le rendu doublé prend un peu plus de la moitié des 3,1 ms et la logique ne coûte presque rien. Mais la pire image de rendu (3,78 ms) fait déjà dépasser les 3,1 ms toute seule, et doublée elle donnerait environ 7,6 ms. Dans un casque, ces images-là seraient ratées.

Ce qu'il faudrait réduire, c'est le rendu, pas la logique. Le programme fait 5000 appels de dessin séparés par image, un par particule, et c'est eux qui coûtent. Il faudrait les regrouper (dessiner toutes les particules en un seul appel) ou en dessiner moins, et en VR utiliser un rendu qui dessine les deux yeux en une seule passe au lieu de refaire toute la scène deux fois.

## Code

Fichier : [`main.cpp`](main.cpp)

```cpp
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
```

## Résultats

```text
particules : 5000
logique seule : 0.016 ms
rendu seul : 0.873 ms (pire 3.783 ms)
rendu x2 estime : 1.746 ms
reste sur 3.1 ms : 1.365 ms
--
particules : 5000
logique seule : 0.014 ms
rendu seul : 0.806 ms (pire 2.409 ms)
rendu x2 estime : 1.612 ms
reste sur 3.1 ms : 1.499 ms
--
particules : 5000
logique seule : 0.016 ms
rendu seul : 0.808 ms (pire 2.603 ms)
rendu x2 estime : 1.616 ms
reste sur 3.1 ms : 1.495 ms
```
