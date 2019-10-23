#ifndef __DOSES__
#define __DOSES__

#include "etl/cstring.h"

typedef struct {
    float volume = 0.0;        // Volume to dispense, in mm³
    const char * desc = NULL;   // Description
    etl::string<16> title = 0;  // Title (volume + " mm³", in text form)
} dose_t;

extern dose_t doses[];

#endif
