#include "filter.h"

void initFilter(struct Filter* filter, float f, float r, float d) {
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

void setFrequency(struct Filter* filter, float f) {
	if (f > 12000.0f) f = 12000.0f;
	if (f < 0.0f) f = 0.0f;
	filter->frequency = f;
	filter->g = 1 - expf(-2 * tanf(2 * PI * filter->frequency/(2 * SAMPLING_FREQ)));
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

void filterSamples(struct Filter* filter, float* samples) {
	int i;
	for (i = 0; i < BUFFER_LENGTH; i++) {
		float sample = samples[i];
		samples[i] = tanhf(sample * filter->drive);
		filter->y_a = filter->y_a + filter->g *
			(tanhf(samples[i] - filter->resonance *
				((filter->y_d_1 + filter->y_d)/2) - tanhf(filter->y_a)));
		filter->y_b = filter->y_b + filter->g * (tanhf(filter->y_a) - tanhf(filter->y_b));
		filter->y_c = filter->y_c + filter->g * (tanhf(filter->y_b) - tanhf(filter->y_c));

		filter->y_d_1 = filter->y_d;

		filter->y_d = filter->y_d + filter->g * (tanhf(filter->y_c) - tanhf(filter->y_d));

		samples[i] = filter->y_d;
	}
}
