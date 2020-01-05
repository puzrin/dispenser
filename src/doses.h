#ifndef __DOSES__
#define __DOSES__

typedef struct {
    const float volume;         // Volume to dispense, in mm³
    const char * const desc;    // Description
    const char * const title;   // Title (volume + " mm³", in text form)
} dose_t;

extern const dose_t doses[];

#endif
