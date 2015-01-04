#include "stm32f4xx_conf.h"
#include "setup.h"

void fillInBuffer();
void incrementPhase(float* phase, float increment);
float polyBlep(float phase, float phaseIncrement);
float getPhaseIncrement(float note);
float sine(float phase, float phaseIncrement);
float square(float phase, float phaseIncrement);
float sawtooth(float phase, float phaseIncrement);

float getInterpolatedValue(float value, float* array);
