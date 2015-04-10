#include "note.h"

void initNote(struct Note* note, int numOfNotes) {
	note->pitch = 0;
	note->phase = 0;
	note->state = 0;
	note->position = numOfNotes;
	setADSROff(&note->envelope, &note->state);
}

void addNote(int pitch, struct Note notes[], int numOfNotes) {
	int i;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position == numOfNotes || notes[i].pitch == pitch) {
			notes[i].pitch = pitch;
			notes[i].position = 0;
			setADSROn(&notes[i].envelope, &notes[i].state);
			break;
		}
	}
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position < numOfNotes) notes[i].position++;
	}
}

void removeNote(int pitch, struct Note notes[], int numOfNotes) {
	int i;
	struct Note* ourNote = NULL;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].pitch == pitch) {
			ourNote = &notes[i];
			break;
		}
	}
	if (ourNote == NULL) return;
	int lastActiveNotePosition = ourNote->position;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position > ourNote->position && notes[i].state == 1) {
			if (lastActiveNotePosition < notes[i].position)
				lastActiveNotePosition = notes[i].position;
			notes[i].position--;
		}
	}
	ourNote->position = lastActiveNotePosition;
	setADSROff(&ourNote->envelope, &ourNote->state);
}
