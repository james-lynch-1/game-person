#ifndef CPU
#define CPU

#include "util.h"
#include "global.h"
#include "type.h"

void decrementReg16();

void writeByteToMemAndDec();

void cpuTick();

#endif
