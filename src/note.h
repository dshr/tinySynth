#include "adsr.h"

#ifndef _NOTE
#define _NOTE

struct Note {
	float pitch;
	float phase;
	struct ADSR envelope;
	int state;
	int position;
};

void initNote(struct Note* note, int numOfNotes);
void addNote(int pitch, struct Note notes[], int numOfNotes);
void removeNote(int pitch, struct Note notes[], int numOfNotes);
#endif
