#include "stm32f4xx_conf.h"
#include "setup.h"

void fillInBuffer();
void incrementPhase(float* phase, float note);
float polyBlep(float phase, float phaseIncrement);
float getPhaseIncrement(float note);
float sine(float phase, float note);
float square(float phase, float note);
float sawtooth(float phase, float note);

float getInterpolatedValue(float value, float* array);
