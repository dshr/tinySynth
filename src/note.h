#include "adsr.h"
#include "filter.h"

#ifndef _NOTE
#define _NOTE

struct Note {
	float pitch;
	float phase;
	struct ADSR ampEnvelope;
	struct Filter filter;
	struct ADSR filterEnvelope;
	int state;
	int position;
};

void initNote(struct Note* note, int numOfNotes);
void addNote(int pitch, struct Note notes[], int numOfNotes);
void removeNote(int pitch, struct Note notes[], int numOfNotes);
#endif
