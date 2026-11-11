#pragma once
#include <cstdint>

class Chaine {
public:
    static constexpr std::uint32_t kMaxImages = 4u;

    bool Creer(std::uint32_t nombre) {
        if (nombre == 0u || nombre > kMaxImages) {
            return false;
        }
        mNombre = nombre;
        mSuivante = 0u;
        mEmpruntee = false;
        for (std::uint32_t i = 0; i < kMaxImages; ++i) {
            mImages[i] = 0u;
        }
        return true;
    }

    void Enregistrer(std::uint32_t index, std::uint64_t poignee) {
        if (index < mNombre) {
            mImages[index] = poignee;
        }
    }

    std::uint64_t Poignee(std::uint32_t index) const {
        if (index < mNombre) {
            return mImages[index];
        }
        return 0u;
    }

    bool Emprunter(std::uint32_t& index) {
        if (mNombre == 0u || mEmpruntee) {
            return false;
        }
        index = mSuivante;
        mEmpruntee = true;
        return true;
    }

    bool Rendre() {
        if (!mEmpruntee) {
            return false;
        }
        mSuivante = (mSuivante + 1u) % mNombre;
        mEmpruntee = false;
        return true;
    }

    std::uint32_t DerniereRendue() const {
        if (mNombre == 0u) {
            return 0u;
        }
        return (mSuivante + mNombre - 1u) % mNombre;
    }

    bool Empruntee() const {
        return mEmpruntee;
    }

private:
    std::uint64_t mImages[kMaxImages]{};
    std::uint32_t mNombre = 0u;
    std::uint32_t mSuivante = 0u;
    bool mEmpruntee = false;
};
