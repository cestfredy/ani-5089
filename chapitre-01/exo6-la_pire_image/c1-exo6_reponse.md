# c1-exo6 : La pire image

## Réponse

J'ai lancé le programme trois fois. Sur les 1000 images mesurées de chaque essai :

- Durée de la plus longue image : 7,00 ms au pire des trois essais (3,47 ms, 7,00 ms et 3,82 ms)
- Images au-delà de 11 ms : 0 / 1000 à chaque essai

Mon programme ne tiendrait pas dans un casque à 90 Hz. Il ne faut pas comparer la pire image aux 11,1 ms d'une image entière, mais aux 3,1 ms qui restent pour le code une fois retirées les 8 ms des capteurs, de la transmission, de la composition et de l'affichage (exercices 1 et 7). Or la pire image monte à 7,00 ms, et 6, 20 puis 25 images sur 1000 dépassent 3,1 ms selon l'essai. Et ce programme ne fait qu'effacer l'écran.

La chauffe cachait aussi des images lentes. La plus longue des 100 premières images, que j'écarte de la mesure, vaut 7,12 ms, 3,50 ms puis 5,44 ms : dans deux essais sur trois, elle est plus longue que la pire image mesurée ensuite, et elle dépasse 3,1 ms dans les trois. Dans un casque, l'utilisateur voit ces images-là aussi. Le démarrage, quand la fenêtre, le contexte graphique et le pilote se mettent en place, donne souvent la pire image d'un programme.

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
```

## Résultats

```text
pire image de la chauffe (100 premieres) : 7.12 ms
images mesurees : 1000
pire image : 3.47 ms
images > 11 ms : 0
images > 3.1 ms : 6
--
pire image de la chauffe (100 premieres) : 3.50 ms
images mesurees : 1000
pire image : 7.00 ms
images > 11 ms : 0
images > 3.1 ms : 20
--
pire image de la chauffe (100 premieres) : 5.44 ms
images mesurees : 1000
pire image : 3.82 ms
images > 11 ms : 0
images > 3.1 ms : 25
```
