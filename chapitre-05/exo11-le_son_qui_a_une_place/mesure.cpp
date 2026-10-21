#include "NKAudio/NkAudio.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::audio;

const int32 TRAMES = 48000;
const int32 BLOC = 256;

struct Position {
    const char* nom;
    float32 x;
    float32 y;
    float32 z;
};

static void Mesurer(AudioEngine& moteur, float32& rmsGauche, float32& rmsDroite) {
    std::vector<float32> bloc(BLOC * 2);
    double sommeG = 0.0;
    double sommeD = 0.0;
    int32 rendues = 0;
    while (rendues < TRAMES) {
        int32 n = BLOC;
        if (TRAMES - rendues < BLOC) {
            n = TRAMES - rendues;
        }
        moteur.RenderToBuffer(bloc.data(), n, 2);
        for (int32 i = 0; i < n; ++i) {
            sommeG += (double)bloc[i * 2] * bloc[i * 2];
            sommeD += (double)bloc[i * 2 + 1] * bloc[i * 2 + 1];
        }
        rendues += n;
    }
    rmsGauche = (float32)std::sqrt(sommeG / TRAMES);
    rmsDroite = (float32)std::sqrt(sommeD / TRAMES);
}

int main() {
    AudioEngineConfig cfg;
    cfg.sampleRate = 48000;
    cfg.backend = AudioBackendType::NULL_OUTPUT;
    AudioEngine& moteur = AudioEngine::Instance();
    if (!moteur.Initialize(cfg)) {
        std::printf("Le moteur de son ne s'est pas initialise\n");
        return 1;
    }

    moteur.SetListenerPosition(0.f, 0.f, 0.f);
    moteur.SetListenerOrientation(0.f, 0.f, -1.f, 0.f, 1.f, 0.f);

    AudioSample pluie = AudioGenerator::GenerateNoise(4.f, WaveformType::NOISE_PINK);
    const Position positions[3] = {
        {"gauche", -2.f, 0.f, 0.f},
        {"devant", 0.f, 0.f, -2.f},
        {"droite", 2.f, 0.f, 0.f},
    };

    std::FILE* f = std::fopen("mesures.txt", "w");
    if (!f) {
        return 2;
    }
    for (const Position& pos : positions) {
        for (int positional = 1; positional >= 0; --positional) {
            VoiceParams p;
            p.looping = true;
            p.source3d.position[0] = pos.x;
            p.source3d.position[1] = pos.y;
            p.source3d.position[2] = pos.z;
            p.source3d.positional = (positional == 1);
            AudioHandle voix = moteur.Play(pluie, p);

            float32 g = 0.f;
            float32 d = 0.f;
            Mesurer(moteur, g, d);
            moteur.Stop(voix);

            std::fprintf(f, "%s %d : G %.4f D %.4f\n", pos.nom, positional, g, d);
            std::printf("%s %d : G %.4f D %.4f\n", pos.nom, positional, g, d);
        }
    }
    std::fclose(f);

    moteur.Shutdown();
    AudioLoader::Free(pluie);
    std::printf("Mesures ecrites dans mesures.txt\n");
    return 0;
}
