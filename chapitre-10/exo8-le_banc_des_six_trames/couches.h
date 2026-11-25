#pragma once
#include <cstdint>

enum class Espace {
    LOCAL,
    STAGE
};

struct Pose {
    float position[3] = {0.f, 0.f, 0.f};
    float lacetDegres = 0.f;
};

struct ChampDeVision {
    float gauche = -45.f;
    float droite = 45.f;
    float haut = 45.f;
    float bas = -45.f;
};

struct Chaine {
    uint64_t poignee = 0;
    uint32_t images = 3;
};

struct VueDeCouche {
    Pose pose;
    ChampDeVision champ;
    const Chaine* chaine = nullptr;
    uint32_t indexImage = 0;
};

struct CoucheProjection {
    Espace espace = Espace::STAGE;
    VueDeCouche vues[2];
};

struct FinDeTrame {
    int64_t instantAffichage = 0;
    const CoucheProjection* const* couches = nullptr;
    uint32_t nombreCouches = 0;
};
