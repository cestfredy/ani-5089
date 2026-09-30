# c1-exo6 : La pire image

## Réponse

J'ai lancé le programme trois fois. Sur les trois essais :

- Durée de la plus longue image : 4,03 ms (au pire des trois essais)
- Images au-delà de 11 ms : 0 / 1000 à chaque essai

Mon programme tiendrait dans un casque à 90 Hz : même la pire image reste bien sous les 11,1 ms, et aucune ne dépasse. Mais il ne fait qu'effacer l'écran. Avec une vraie scène à dessiner, il reste seulement environ 7 ms de marge avant de rater une image.

## Code

Fichier : [`main.cpp`](main.cpp)

```cpp
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

    double pire = 0;
    int depasse = 0;
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

        if (image > chauffe) {
            if (ms > pire) pire = ms;
            if (ms > seuil) depasse++;
        }
    }
    CloseWindow();

    printf("images mesurees : %d\n", image - chauffe);
    printf("pire image : %.2f ms\n", pire);
    printf("images > 11 ms : %d\n", depasse);
    return 0;
}
```

## Résultats

```text
images mesurees : 1000
pire image : 2.50 ms
images > 11 ms : 0
--
images mesurees : 1000
pire image : 3.55 ms
images > 11 ms : 0
--
images mesurees : 1000
pire image : 4.03 ms
images > 11 ms : 0
```
