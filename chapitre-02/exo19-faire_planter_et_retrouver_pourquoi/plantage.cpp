#include <android/log.h>

#define TAG "MaSalle"

// noinline pour que la fonction apparaisse dans la trace
__attribute__((noinline)) static int LireCapteur(const volatile int* capteur) {
    return *capteur;
}

extern "C" void android_main(void*) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "Demarrage de MaSalle");
    __android_log_print(ANDROID_LOG_WARN, TAG, "Plantage volontaire : lecture d'un pointeur nul");

    const volatile int* capteur = nullptr;
    int valeur = LireCapteur(capteur);

    __android_log_print(ANDROID_LOG_INFO, TAG, "Valeur lue : %d", valeur);
}
