#ifndef CPU
#define CPU

#include "global.h"
#include "serial.h"

extern void(*opQ[6])(); extern int numOpsQueued; // counts down. Always pre-decrement when using.
extern int pQ[32]; extern int pIndex; // these act as parameters for the micro-ops. Counts up. Always post-inc.
extern void(*miscOp)(); // miscellaneous operation to be done at the end of the opqueue but doesn't incur cycles

void decrementReg16();

void writeByteToMem();

void writeByteToMemAndDec();

void jumpToRegAddr();

void cpuTick();

void readByteToReg();

void doNothing();

// interrupts

extern void(*iSROpQ[6])(); extern int numISROpsQueued;
extern int iSRParamQ[6]; extern int iSRParamIndex;
extern void(*iSRMiscOp)();

void requestInterrupt(int intr);

void handleInterrupt(int intr);

void checkUnhandledInterrupts();

#endif
