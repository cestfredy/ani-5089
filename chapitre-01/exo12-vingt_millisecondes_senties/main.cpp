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
