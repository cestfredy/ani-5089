#pragma once
#include <string>
#include <utility>

struct EtatAction {
    bool courant = false;
    bool change = false;
    bool actif = false;
};

class Action {
public:
    Action(std::string nom, std::string usage) : mNom(std::move(nom)), mUsage(std::move(usage)) {}

    void Lier(const bool* source) {
        mSource = source;
    }

    void Synchroniser() {
        bool precedent = mEtat.courant;
        mEtat.actif = mSource != nullptr;
        mEtat.courant = mEtat.actif && *mSource;
        mEtat.change = mEtat.courant != precedent;
    }

    const EtatAction& Etat() const {
        return mEtat;
    }

    bool Saisie() const {
        return mEtat.courant && mEtat.change;
    }

    bool Lachee() const {
        return !mEtat.courant && mEtat.change;
    }

    const std::string& Nom() const {
        return mNom;
    }

    const std::string& Usage() const {
        return mUsage;
    }

private:
    std::string mNom;
    std::string mUsage;
    const bool* mSource = nullptr;
    EtatAction mEtat;
};
