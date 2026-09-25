# c1-exo1 : Les trois cadences

## Réponse

| Fréquence | Durée d'une image (1000 / f) | Reste pour le code (− 8 ms) |
|---|---|---|
| 72 Hz | **13,9 ms** | **5,9 ms** |
| 90 Hz | **11,1 ms** | **3,1 ms** |
| 120 Hz | **8,3 ms** | **0,3 ms** |

Les trois nombres à retenir pour le chapitre 10 : **5,9 ms, 3,1 ms et 0,3 ms**.

## Code

Fichier : [`main.cpp`](main.cpp)

```cpp
#include <cstdio>

int main() {
    double pipeline = 8.0; // pour capteurs, transmission, composition et affichage
    double freq[3] = {72, 90, 120};

    printf("frequence | duree image | reste pour le code\n");
    for (int i = 0; i < 3; i++) {
        double duree = 1000.0 / freq[i];
        double reste = duree - pipeline;
        printf("%6.0f Hz | %8.1f ms | %8.1f ms\n", freq[i], duree, reste);
    }
    return 0;
}
```

## Résultats

Sortie obtenue sur ma machine :

```text
frequence | duree image | reste pour le code
    72 Hz |     13.9 ms |      5.9 ms
    90 Hz |     11.1 ms |      3.1 ms
   120 Hz |      8.3 ms |      0.3 ms
```

Identiques au calcul à la main (1000/72 = 13,89 ; 1000/90 = 11,11 ; 1000/120 = 8,33).
