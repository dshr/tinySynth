#include "filter.h"

inline void initFilter(struct Filter* filter, float f, float r, float d) {
	filter->y_a = 0.0f;
	filter->y_b = 0.0f;
	filter->y_c = 0.0f;
	filter->y_d = 0.0f;
	filter->y_d_1 = 0.0f;
	filter->g = 0.0f;

	setFrequency(filter, f);
	setResonance(filter, r);
	setDrive(filter, d);
}

inline float polytan(float x) {
	return x + (0.33333333333f * x * x * x)
					 + (0.13333333333f * x * x * x * x * x);
					 // + (0.05396825397f * x * x * x * x * x * x * x);
}

inline float polyexp(float x) {
	return 1 + x
					 + (0.5f * x * x)
					 + (0.1666666667f * x * x * x)
					 + (0.04166666667f * x * x * x * x)
					 + (0.008333333333f * x * x * x * x * x);
}

inline float polytanhf(float x) {
	return x - (0.33333333333f * x * x * x)
					 + (0.13333333333f * x * x * x * x * x);
					 // - (0.05396825397f * x * x * x * x * x * x * x);
}

void setFrequency(struct Filter* filter, float f) {
	if (f > 12000.0f) f = 12000.0f;
	if (f < 0.0f) f = 0.0f;
	filter->frequency = f;
	filter->g = 1 - polyexp(-2 * polytan(2 * PI * filter->frequency/(2 * SAMPLING_FREQ)));
}

void setResonance(struct Filter* filter, float r) {
	if (r > 4.0f) r = 4.0f;
	if (r < 0.0f) r = 0.0f;
	filter->resonance = r;
}

void setDrive(struct Filter* filter, float d) {
	if (d > 20.0f) d = 20.0f;
	if (d < 0.05f) d = 0.05f;
	filter->drive = d;
}

void filterSample(struct Filter* filter, float* sample) {
		// *sample = tanhf(*sample * filter->drive);
		filter->y_a = filter->y_a + filter->g *
			(polytanhf(*sample - filter->resonance *
				((filter->y_d_1 + filter->y_d)*0.5f) - polytanhf(filter->y_a)));
		filter->y_b = filter->y_b + filter->g * (polytanhf(filter->y_a) - polytanhf(filter->y_b));
		filter->y_c = filter->y_c + filter->g * (polytanhf(filter->y_b) - polytanhf(filter->y_c));

		filter->y_d_1 = filter->y_d;

		filter->y_d = filter->y_d + filter->g * (filter->y_c - (filter->y_d));
		*sample = filter->y_d;
}
