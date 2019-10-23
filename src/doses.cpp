#include "doses.h"

dose_t doses[] = {
    { .volume = 0.041f, .desc = "chip 0402" },
    { .volume = 0.053f, .desc = "SOT-23"    },
    { .volume = 0.085f, .desc = "chip 0603" },
    { .volume = 0.17f,  .desc = "chip 0805" },
    { .volume = 0.35f,  .desc = "chip 1206" },
    { .volume = 0.70f,  .desc = "chip 2512" },
    { .volume = 0.0f } // Mark end of list
};
