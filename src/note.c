#include "note.h"

inline void initNote(struct Note* note, int numOfNotes) {
	note->pitch = 0;
	note->phase = 0;
	note->state = 0;
	note->position = numOfNotes;
	setADSROff(&note->ampEnvelope, &note->state);
	setADSROff(&note->filterEnvelope, &note->state);
}

inline void addNote(int pitch, struct Note notes[], int numOfNotes) {
	int i;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position == numOfNotes || notes[i].pitch == pitch) {
			notes[i].pitch = pitch;
			notes[i].position = 0;
			setADSROn(&notes[i].ampEnvelope, &notes[i].state);
			// setADSROn(&notes[i].filterEnvelope, &notes[i].state);
			break;
		}
	}
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position < numOfNotes) notes[i].position++;
	}
}

inline void removeNote(int pitch, struct Note notes[], int numOfNotes) {
	int i, j;
	struct Note* ourNote = NULL;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].pitch == pitch) {
			ourNote = &notes[i];
			int lastActiveNotePosition = ourNote->position;
			for (j = 0; j < numOfNotes; j++) {
				if (notes[j].position > ourNote->position && notes[j].state == 1) {
					if (lastActiveNotePosition < notes[j].position)
						lastActiveNotePosition = notes[j].position;
					notes[j].position--;
				}
			}
			ourNote->position = lastActiveNotePosition;
			setADSROff(&ourNote->ampEnvelope, &ourNote->state);
			setADSROff(&ourNote->filterEnvelope, &ourNote->state);
		}
	}
}
