#include "couches.h"

#include <cstdio>

const int64_t PERIODE_NS = 13888889;

class Session {
public:
    explicit Session(const Chaine& chaine) : mChaine(chaine) {}

    bool AttendreTrame(int64_t& instantPredit, bool& doitDessiner, int trame) {
        if (mOuverte) {
            return false;
        }
        instantPredit = (int64_t)trame * PERIODE_NS;
        doitDessiner = trame != 3 && trame != 4;
        mAttendue = true;
        return true;
    }

    bool CommencerTrame() {
        if (!mAttendue) {
            return false;
        }
        mAttendue = false;
        mOuverte = true;
        return true;
    }

    Pose LocaliserVue(int64_t instant) const {
        Pose p;
        p.position[1] = 1.70f;
        p.lacetDegres = (float)(instant / PERIODE_NS);
        return p;
    }

    uint32_t Acquerir() const {
        return mIndex;
    }

    void Liberer() {
        if (mChaine.images > 0) {
            mIndex = (mIndex + 1) % mChaine.images;
        }
    }

    bool FinTrame(const FinDeTrame& fin) {
        if (!mOuverte) {
            std::fprintf(stderr, "[session] fin de trame sans debut : refusee\n");
            return false;
        }
        for (uint32_t c = 0; c < fin.nombreCouches; ++c) {
            for (const VueDeCouche& v : fin.couches[c]->vues) {
                if (v.chaine == nullptr) {
                    std::fprintf(stderr, "[session] projection sans chaine pour un oeil\n");
                }
            }
        }
        mOuverte = false;
        return true;
    }

private:
    const Chaine& mChaine;
    uint32_t mIndex = 0;
    bool mAttendue = false;
    bool mOuverte = false;
};

int main() {
    const Chaine chaine{0x51A1u, 3};
    Session session(chaine);

    std::printf("image  on rend  couches soumises  index d'image  lacet du rendu  fin de trame\n");
    for (int trame = 1; trame <= 6; ++trame) {
        int64_t instant = 0;
        bool doitDessiner = false;
        if (!session.AttendreTrame(instant, doitDessiner, trame)) {
            std::printf("%5d  bloque a l'attente\n", trame);
            break;
        }
        session.CommencerTrame();

        CoucheProjection projection;
        const CoucheProjection* liste[1] = {&projection};
        FinDeTrame fin;
        fin.instantAffichage = instant;
        fin.couches = liste;
        fin.nombreCouches = 0;
        long index = -1;

        if (doitDessiner) {
            const Pose poseDuRendu = session.LocaliserVue(instant);
            const uint32_t i = session.Acquerir();
            session.Liberer();
            projection.espace = Espace::STAGE;
            for (VueDeCouche& v : projection.vues) {
                v.pose = poseDuRendu;
                v.chaine = &chaine;
                v.indexImage = i;
            }
            fin.nombreCouches = 1;
            index = (long)i;
        }

        const bool finie = session.FinTrame(fin);
        const char* etatFin = finie ? "appelee" : "refusee";
        if (index >= 0) {
            std::printf("%5d  %7s  %16u  %13ld  %14.1f  %s\n", trame, "oui",
                        fin.nombreCouches, index, (double)projection.vues[0].pose.lacetDegres,
                        etatFin);
        } else {
            std::printf("%5d  %7s  %16u  %13s  %14s  %s\n", trame, "non",
                        fin.nombreCouches, "-", "-", etatFin);
        }
    }
    return 0;
}
