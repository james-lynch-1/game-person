#ifndef CPU
#define CPU

#include "util.h"
#include "registers.h"
#include "global.h"
#include "ram.h"
#include "type.h"

void decrementReg16();

void writeByteToMemAndDec16();

void cpuTick();

#endif
