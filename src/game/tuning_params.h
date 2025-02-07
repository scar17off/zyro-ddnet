#ifndef GAME_TUNING_PARAMS_H
#define GAME_TUNING_PARAMS_H

// Define the tuning parameter macro
#define MACRO_TUNING_PARAM(Name, Value, Default, Description) \
    float Name = Default;

#include "tuning.h"

#undef MACRO_TUNING_PARAM

#endif // GAME_TUNING_PARAMS_H 