#ifndef _ADSR
#define _ADSR
#include <math.h>

struct ADSR {
	unsigned attackTime;
	unsigned decayTime;
	float sustainLevel;
	unsigned releaseTime;
	float decayMultiplier;
	float releaseMultiplier;
	unsigned index;
	float level;
};

void initADSR(struct ADSR* adsr, unsigned a, unsigned d, float s, unsigned r);
void setADSROn(struct ADSR* adsr, int* on);
void setADSROff(struct ADSR* adsr);
void runADSR(struct ADSR* adsr, int* on);
float getADSRLevel(struct ADSR* adsr);
void setAttack(struct ADSR* adsr, unsigned a);
void setDecay(struct ADSR* adsr, unsigned d);
void setSustain(struct ADSR* adsr, float s);
void setRelease(struct ADSR* adsr, unsigned r);
#endif
