#include "doses.h"

#define DOSE_RECORD(VAL, DESC) { .volume = VAL, .desc = DESC, .title = #VAL" mm³" }

const dose_t doses[] = {
    DOSE_RECORD(0.041, "chip 0402"),
    DOSE_RECORD(0.053, "SOT-23"),
    DOSE_RECORD(0.085, "chip 0603"),
    DOSE_RECORD(0.170,  "chip 0805"),
    DOSE_RECORD(0.350,  "chip 1206"),
    DOSE_RECORD(0.700,  "chip 2512"),
    DOSE_RECORD(0, "") // Mark end of list
};
