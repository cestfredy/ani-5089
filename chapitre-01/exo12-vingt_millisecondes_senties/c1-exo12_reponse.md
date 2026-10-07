# c1-exo12 : Vingt millisecondes, senties

## Réponse

Un disque suit la souris avec un retard réglable de 0 à 200 ms, par pas de 5 ms (flèches ou molette). Pendant le test, le curseur Windows est masqué et la valeur du retard aussi (touche Espace) : la personne ne voit que le disque. Je pars de 0 ms et je monte de 5 en 5, en lui demandant de bouger la souris franchement. Je note le premier retard où elle dit sentir que le disque traîne.

| Personne | Seuil ressenti (ms) |
|---|:---:|
| Moi | 45 |
| Joseph | 55 |
| Yannick | 40 |
| Maikel | 70 |
| Luciano | 50 |
| **Moyenne** | 52 |

Les cinq seuils sont tous au-dessus des 20 ms du budget : sur un écran, personne n'a senti un retard de 20 ms, et la plupart ne remarquent quelque chose qu'autour de 50 ms. Ce retard s'ajoute en plus à celui que l'ordinateur a déjà (souris, rendu, écran), donc le retard total au moment où la personne le sent est encore plus grand.

Dans un casque, le seuil est bien plus bas parce que ce n'est plus la main qui commande l'image, c'est la tête. Sur l'écran, je compare le mouvement de ma main à un petit disque dans un coin de mon champ de vision, seulement avec les yeux. Dans le casque, l'oreille interne sent la rotation de la tête tout de suite, et l'image doit la suivre sur tout le champ de vision. Si elle arrive en retard, le monde entier glisse un peu à chaque mouvement de tête, alors qu'il devrait rester fixe. Comme la tête tourne vite, même 20 ms font un décalage visible : à 100° par seconde, 20 ms de retard décalent l'image de 2°. Ce conflit entre ce que voient les yeux et ce que sent l'oreille interne donne le malaise, alors que sur un écran le même retard est seulement un peu désagréable.

## Code

Fichier : [`main.cpp`](main.cpp)

```cpp
#include "raylib.h"
#include <algorithm>
#include <deque>

struct Echantillon {
    double t;
    Vector2 p;
};

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "c1-exo12");
    SetTargetFPS(0);
    HideCursor();

    std::deque<Echantillon> historique;
    int retardMs = 0;
    bool aveugle = false;
    bool curseur = false;

    while (!WindowShouldClose()) {
        double t = GetTime();
        historique.push_back({t, GetMousePosition()});
        while (historique.size() > 2 && historique[1].t < t - 0.25) {
            historique.pop_front();
        }

        int pas = (int)GetMouseWheelMove() * 5;
        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
            pas += 5;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
            pas -= 5;
        }
        retardMs = std::clamp(retardMs + pas, 0, 200);
        if (IsKeyPressed(KEY_R)) {
            retardMs = GetRandomValue(0, 40) * 5;
        }
        if (IsKeyPressed(KEY_SPACE)) {
            aveugle = !aveugle;
        }
        if (IsKeyPressed(KEY_C)) {
            curseur = !curseur;
            if (curseur) {
                ShowCursor();
            } else {
                HideCursor();
            }
        }

        double cible = t - retardMs / 1000.0;
        Vector2 p = historique.front().p;
        for (const auto& e : historique) {
            if (e.t > cible) {
                break;
            }
            p = e.p;
        }

        BeginDrawing();
        ClearBackground(Color{18, 18, 24, 255});
        DrawCircleV(p, 18, ORANGE);
        if (aveugle) {
            DrawText("Retard : ???", 20, 20, 32, GRAY);
        } else {
            DrawText(TextFormat("Retard : %d ms", retardMs), 20, 20, 32, RAYWHITE);
        }
        DrawText("Fleches/molette : +-5 ms   R : hasard   Espace : masquer   C : curseur",
                 20, GetScreenHeight() - 36, 20, GRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
```
