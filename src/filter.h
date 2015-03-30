#include "stm32f4xx_conf.h"
#include "math.h"

struct Filter {
	float frequency;
	float g;
	float resonance;
	float drive;

	float y_a;
	float y_b;
	float y_c;
	float y_d;
	float y_d_1;
};

void initFilter(struct Filter* filter, float f, float r, float d);
void setFrequency(struct Filter* filter, float f);
void setResonance(struct Filter* filter, float r);
void setDrive(struct Filter* filter, float d);

void filterSamples(struct Filter* filter, float* samples);
