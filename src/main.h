#include "stm32f4xx_conf.h"
#include "globals.h"
#include "setup.h"
#include "adsr.h"
#include "filter.h"
#include "note.h"
#include <math.h>

void fillInBuffer();
void incrementPhase(float* phase, float increment);
void swapBuffers();
float polyBlep(float phase, float phaseIncrement);
float getPhaseIncrementFromMIDI(float note);
float getPhaseIncrementFromFrequency(float frequency);
float sine(float phase, float phaseIncrement);
float square(float phase, float phaseIncrement, float pulseWidthMod);
float sawtooth(float phase, float phaseIncrement);
