#include "math.h"

struct ADSR {
	unsigned attackTime;
	unsigned decayTime;
	float sustainLevel;
	unsigned releaseTime;
	unsigned index;
	float level;
	float offLevel;
};

void initADSR(struct ADSR* adsr, unsigned a, unsigned d, float s, unsigned r);
void setADSROn(struct ADSR* adsr, int* on);
void setADSROff(struct ADSR* adsr, int* on);
void runADSR(struct ADSR* adsr, int* on);
float getADSRLevel(struct ADSR* adsr);
