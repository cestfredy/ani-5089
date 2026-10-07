# c1-exo7 : Le coût du doublement

## Réponse

J'ai repris le programme de l'exo 6 en ajoutant 5000 particules qui bougent, pour avoir une logique et un rendu à séparer. J'ai découpé le rendu en trois morceaux chronométrés dans la même boucle : l'effacement, l'ajout des rectangles, et la fin d'image (`EndDrawing`). Trois essais, presque identiques ; les chiffres ci-dessous viennent du premier.

| | Temps moyen |
|---|:---:|
| Logique seule | 0,010 ms |
| Rendu seul (mesuré) | 0,655 ms (pire 5,19 ms) |
| dont effacement | 0,001 ms |
| dont ajout des 5000 rectangles au lot | 0,319 ms |
| dont fin d'image (envoi du lot, dessin, présentation) | 0,336 ms |
| Rendu fait deux fois (estimé) | entre 0,655 et 1,310 ms, environ 0,98 ms en deux passes |
| Reste sur 3,1 ms (part du code à 90 Hz, exo 1) | entre 1,80 et 2,46 ms |

L'estimation est encadrée parce que tout le rendu ne se refait pas pour le second œil :

- borne basse, 0,655 ms (une passe pour les deux yeux) : le lot est rempli une seule fois et la présentation n'a lieu qu'une fois. C'est ce que fait la stéréo de raylib : dans `rlDrawRenderBatch`, la boucle `for (int eye = 0; eye < eyeCount; eye++)` refait seulement les appels de dessin, les sommets ne sont envoyés qu'une fois. Ce qui double alors se passe sur la carte graphique (les sommets transformés une fois par œil et les pixels de deux images), et ma mesure côté processeur ne le voit pas ;
- deux passes, environ 0,98 ms : chaque œil efface et remplit son lot, mais la présentation reste unique. On ajoute donc une fois l'effacement et l'ajout des rectangles ;
- borne haute, 1,310 ms : tout refait, présentation comprise. C'est le pire cas, qu'on n'atteint pas puisqu'on ne présente qu'une image pour les deux yeux.

En moyenne ça passe dans tous les cas : il reste entre 1,80 et 2,46 ms sur les 3,1 ms, et la logique ne coûte presque rien. Mais la pire image de rendu (5,19 ms, 8,13 ms et 3,51 ms selon l'essai) dépasse déjà à elle seule les 3,1 ms. Dans un casque, ces images-là seraient ratées : ce sont les pics, pas la moyenne, qui posent problème.

Je m'étais trompé en parlant de « 5000 appels de dessin séparés ». Dans `rlgl.h`, un `DrawRectangle` n'envoie rien à la carte graphique : il ajoute quatre sommets à un lot (le « batch ») qui peut contenir 8192 éléments (`RL_DEFAULT_BATCH_BUFFER_ELEMENTS`). Le lot est envoyé d'un coup par `rlDrawRenderBatch`, avec un seul `glBufferSubData` puis un `glDrawElements` par changement de mode ou de texture. Mes 5000 rectangles de même mode et même texture font donc un seul appel de dessin, pas 5000. Ce qu'il faut réduire n'est pas le nombre d'appels, mais le travail par objet : les 0,32 ms passées à remplir le lot sur le processeur (moins d'objets, ou de l'instanciation qui envoie un seul rectangle et 5000 positions), et sur la carte graphique les sommets et les pixels, qui sont ce qui double vraiment pour deux yeux. Le rendu des deux yeux en une seule passe reste la bonne direction : il évite de remplir et d'envoyer le lot deux fois.

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
```

## Résultats

```text
particules : 5000
logique seule : 0.010 ms
rendu seul : 0.655 ms (pire 5.189 ms)
  effacement : 0.001 ms
  ajout des rectangles au lot : 0.319 ms
  fin d'image (envoi du lot, dessin, presentation) : 0.336 ms
rendu x2, borne basse (une passe) : 0.655 ms
rendu x2, deux passes (effacement et lot refaits) : 0.975 ms
rendu x2, borne haute (tout refait) : 1.310 ms
reste sur 3.1 ms : de 1.801 a 2.456 ms
--
particules : 5000
logique seule : 0.010 ms
rendu seul : 0.655 ms (pire 8.134 ms)
  effacement : 0.001 ms
  ajout des rectangles au lot : 0.338 ms
  fin d'image (envoi du lot, dessin, presentation) : 0.316 ms
rendu x2, borne basse (une passe) : 0.655 ms
rendu x2, deux passes (effacement et lot refaits) : 0.994 ms
rendu x2, borne haute (tout refait) : 1.310 ms
reste sur 3.1 ms : de 1.801 a 2.456 ms
--
particules : 5000
logique seule : 0.010 ms
rendu seul : 0.648 ms (pire 3.511 ms)
  effacement : 0.001 ms
  ajout des rectangles au lot : 0.349 ms
  fin d'image (envoi du lot, dessin, presentation) : 0.298 ms
rendu x2, borne basse (une passe) : 0.648 ms
rendu x2, deux passes (effacement et lot refaits) : 0.998 ms
rendu x2, borne haute (tout refait) : 1.296 ms
reste sur 3.1 ms : de 1.816 a 2.463 ms
```
