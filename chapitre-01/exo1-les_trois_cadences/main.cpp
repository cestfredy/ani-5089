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
