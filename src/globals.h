#ifndef _GLOBALS
#define _GLOBALS

#define PI 3.14159265f
#define EXP 2.7182818285f
#define SEMITONE 1.0594630943592953f
#define BUFFER_LENGTH 256
#define NOTES 6
#define SAMPLING_FREQ 48000

#define AMPLITUDE 32000
#define A_FOUR 440.0f

float getInterpolatedValue(float value, float* array);

#endif