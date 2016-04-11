#include <stdio.h>
#include <string.h>
#include <time.h>
#include <linux/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

struct Note {
	char *node;
	int nextNoteIndex[100];
};

struct Notes {
	struct Note *n;
	int size;
	int allocated;
};

// General xml parsing stuff
//////////////////////////////
xmlDoc *getDoc(char *docName) {
	xmlDocPtr doc = NULL;

	doc = xmlParseFile(docName);

	if(doc == NULL) {
		fprintf(stderr, "Couldn't parse file %s \n", docName);
		return NULL;
	}

	xmlNode *root_element = xmlDocGetRootElement(doc);

	if(xmlStrcmp(root_element->name, (const xmlChar *) "score-partwise")) {
		fprintf(stderr, "This doesn't seem to be a correct musixXml document \n");
		xmlFreeDoc(doc);
		return NULL;
	}

	return doc;
}

xmlNodeSet *makeNodeSet(xmlDoc *doc, char *_keyword) {
	xmlNodeSet *set;
	xmlXPathObject *notePath;
	xmlChar *keyword = (xmlChar *) _keyword;

	// Set notePath
	notePath = xmlXPathEvalExpression(keyword, xmlXPathNewContext(doc));

	// Get notenames
	set = notePath->nodesetval;

	if(!set) fprintf(stderr, "Couldn't make set from %s \n", (char *) keyword);

	return set;
}

int isStringInNotes(char *s, struct Note *n, int arraySize) {
	for(int i=0; i<arraySize; i++) {
		if(strcmp(s, n[i].node) == 0) {
			return 1;
		}
	}

	return 0;
}

int fillUniqueNoteArrayFromNodeSet(struct Notes *notes, xmlNodeSet *ns) {
	char *s1 = (char *) xmlNodeGetContent(ns->nodeTab[0]);

	notes->allocated = ns->nodeNr;
	notes->size = 0;
	notes->n = malloc(notes->allocated * sizeof(struct Note));

	if(!notes->n) {
		fprintf(stderr, "Failed to allocate memory");
		return 1;
	}

	for(int i=0; i<ns->nodeNr; i++) {
		s1 = (char *) xmlNodeGetContent(ns->nodeTab[i]);
		struct Note *n = &notes->n[notes->size];

		if(!isStringInNotes(s1, notes->n, notes->size)) {
			n->node = malloc(sizeof(s1));
			if(!n->node) {
				fprintf(stderr, "Failed to allocate memory");
				return 1;
			}

			n->node = s1;
			notes->size++;
		}
	}

	return 0;
}

xmlNode *getChildNode(xmlNode *parent, char *childName) {
	xmlNode *cur = NULL;
	xmlNode *child = NULL;

	for(cur = parent->children; cur; cur = cur->next) {
		if(xmlStrcmp(cur->name, (const xmlChar *) childName) == 0) {
			child = cur;
		}
	}

	if(!child) fprintf(stderr, "Couldn't find child %s \n", childName);

	return child;
}

int removeChildNode(xmlNode *parent, char *childName) {
	xmlNode *cur = NULL;
	xmlNode *child = NULL;

	for(cur = parent->children; cur; cur = cur->next) {
		if(xmlStrcmp(cur->name, (const xmlChar *) childName) == 0) {
			child = cur;
		}
	}

	if(!child) {
		fprintf(stderr, "Couldn't find child %s \n", childName);
		return 1;
	} else {
		xmlUnlinkNode(child);
		xmlFreeNode(child);
		return 0;
	}
}

//////////////////////////////

// Getting of note properties
//////////////////////////////
char *getNoteName(xmlNode *note) {
	xmlNode *pitch = getChildNode(note, "pitch");
	xmlNode *step = getChildNode(pitch, "step");

	if(step) return (char *) xmlNodeGetContent(step->xmlChildrenNode);
	else return NULL;
}

char *getNoteOctave(xmlNode *note) {
	xmlNode *pitch = getChildNode(note, "pitch");
	xmlNode *octave = getChildNode(pitch, "octave");

	if(octave) return (char *) xmlNodeGetContent(octave->xmlChildrenNode);
	else return NULL;
}

char *getNoteDuration(xmlNode *note) {
	xmlNode *dur = getChildNode(note, "duration");

	if(dur) return (char *) xmlNodeGetContent(dur->xmlChildrenNode);
	else return NULL;
}


int stripNotes(xmlNodeSet *notes) {
	// Retain only the core components of note for comparison
	// TODO Rewrite to delete all that aren't duration or pitch
	for(int i=0; i<notes->nodeNr; i++) {
		removeChildNode(notes->nodeTab[i], "stem");
		removeChildNode(notes->nodeTab[i], "type");
		removeChildNode(notes->nodeTab[i], "beam");
	}

	return 0;
}
//////////////////////////////

int getNoteCount(char *origNote, char *wantedNote, xmlNodeSet *ns) {
	// Get count of how many wanted notes coming after original note
	int c = 0;
	for(int i=0; i<ns->nodeNr; i++) {
		char *curNote = (char *) xmlNodeGetContent(ns->nodeTab[i]);
		if((strcmp(origNote, curNote) == 0) && (i+1 < ns->nodeNr)) {
			char *nextNote = (char *) xmlNodeGetContent(ns->nodeTab[i+1]);
			if(strcmp(wantedNote, nextNote) == 0) {
				c++;
			}
		}
	}

	return c;
}

int makeIndexArrayInNotes(struct Notes *notes, xmlNodeSet *allNotes) {
	for(int i=0; i<notes->size; i++) {
		struct Note *note = &notes->n[i];
		printf("Filling note %s \n", note->node);

		for(int j=0; j<notes->size; j++) {
			char *s = notes->n[j].node;
			int count = getNoteCount(note->node, s, allNotes);
			float percentage = ((float) count/(float) allNotes->nodeNr) * 100;
			int playhead = 0;
			printf("Percentage is %f of count %i \n", percentage, count);

			for(int k=0; k<percentage; k++) {
				if(playhead<100) {
					note->nextNoteIndex[playhead] = j;
					playhead++;
				} else {
					fprintf(stderr, "Playhead of %i is too big %i \n", playhead, k);
				}
			}
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	// Load xml-file
	if (argc != 2) {
		fprintf(stderr, "Need a document to operate on \n");
		return 1;
	}

	xmlDoc *doc;
	xmlNode *root;
	xmlNodeSet *allNotes;

	// Get the notes from file as a set of nodes
	doc = getDoc(argv[1]);
	root = xmlDocGetRootElement(doc);
	allNotes = makeNodeSet(doc, "//note");
	printf("Made nodeset of notes with size %i \n", allNotes->nodeNr);

	// Remove irrelevant parts of the notes
	if(stripNotes(allNotes)) {
		fprintf(stderr, "Couldn't strip the notes \n");
		return 1;
	}
	printf("Stripped the notes \n");

	// Make an array of unique notes
	struct Notes *uniqueNotes;
	uniqueNotes = malloc(sizeof(struct Notes));
	fillUniqueNoteArrayFromNodeSet(uniqueNotes, allNotes);
	printf("Made string array of unique notes with size %i \n", uniqueNotes->size);

	makeIndexArrayInNotes(uniqueNotes, allNotes);

	// Production
	// getNext(note)
	// Go to note in transmat
	// Each note has one array of size 100,
	// made based on the chance of each note,
	// filled with numbers which are the index
	// of the note coming next, which can be found in the
	// array of individual notes.
	xmlFreeDoc(doc);

	// Get next note
	// Check if it exists in transition matrix
	// Fill transmat for note
	//
	// Get first note
	// Put next note from transition matrix
	// Continute ad nauseum
}
