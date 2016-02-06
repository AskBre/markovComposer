#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

struct Note {
	char *name;
	int count;
	float chance;
};

struct TransitionMatrix {
	struct Note *notes;
	int elements;
	int allocated;
};

// XML-reading
xmlDocPtr getDoc(char *docName) {
	xmlDocPtr doc = NULL;

	doc = xmlParseFile(docName);

	if(doc == NULL) {
		fprintf(stderr, "error: Couldn't parse file %s\n", docName);
		return NULL;
	}

	xmlNodePtr root_element = xmlDocGetRootElement(doc);

	if(xmlStrcmp(root_element->name, (const xmlChar *) "score-partwise")) {
		fprintf(stderr, "This doesn't seem to be a correct musixXml document");
		xmlFreeDoc(doc);
		return NULL;
	}

	return doc;
}

//////////////////
// Analyzation //
////////////////
int getNotePos(char *n, struct TransitionMatrix *tm) {
	int notePos = -1;
	for(int i=0; i<tm->allocated; i++){
		if(strcmp(n, tm->notes[i].name) == 0) notePos = i;
	}
	return notePos;
}

int addNoteToMatrix(char *n, struct TransitionMatrix *tm) {

	int notePos = getNotePos(n, tm);

	if(notePos >= 0) {
		tm->notes[notePos].count++;
	} else {
		if(tm->elements < tm->allocated) {
			struct Note *note = &tm->notes[tm->elements];
			note->name = n;
			note->count = 1;
			tm->elements++;
		} else {
			fprintf(stderr, "No more room in transition matrix. Sure you've got the notes right?\n");
			return 1;
		}
	}

	return 0;
}

int setTransitionMatricPercentage(struct TransitionMatrix *tm) {
	unsigned allCounts = 0;
	for(int i=0;i < tm->elements; i++) {
		allCounts += tm->notes[i].count;
	}

	for(int i=0;i < tm->elements; i++) {
		tm->notes[i].chance = (float)tm->notes[i].count/allCounts * 100;
	}

	return 0;
}

struct TransitionMatrix getTransitionMatrix(xmlDocPtr doc) {
	// Setup the transition matrix
	struct TransitionMatrix tm;
	tm.allocated = 12;
	tm.elements = 0;
	void * _notes = malloc(tm.allocated * sizeof(struct TransitionMatrix));

	if(!_notes) {
		fprintf(stderr,  "Couldn't allocate notes for the transition matrix");
	}

	tm.notes = (struct Note *) _notes;

	for(int i=0; i<tm.allocated; i++){
		tm.notes[i].name = (char *) calloc(2, sizeof(char));
	}

	////////////////////
	// Get all the notes!
	xmlChar *keyword = (xmlChar*) "//step";
	xmlNodeSetPtr xmlNotes;
	xmlXPathObjectPtr notePath;

	// Set notePath
	notePath = xmlXPathEvalExpression(keyword, xmlXPathNewContext(doc));

	// Get notenames
	xmlNotes = notePath->nodesetval;
	int noteCount = xmlNotes->nodeNr;

	for(int i=0; i< xmlNotes->nodeNr; i++) {
		char *xmlNoteName;
		xmlNoteName = (char *)xmlNodeListGetString(doc, xmlNotes->nodeTab[i]->xmlChildrenNode, 1);

		addNoteToMatrix(xmlNoteName, &tm);
	}

	setTransitionMatricPercentage(&tm);
	
	xmlXPathFreeObject(notePath);

	return tm;
}

int printTransitionMatrix(struct TransitionMatrix *tm) {
	printf("\n\n\nThe transition matrix:\n");
	for(int i=0; i<tm->elements; i++) {
		struct Note note = tm->notes[i];
		printf("Element %i:\n", i);
		printf("\tName:\t%s\n\tCount:\t%i\n\tChance:\t%f\n\n", note.name, note.count, note.chance);
	}

	return 0;
}

int main(int argc, char **argv) {
	xmlDocPtr doc = NULL;

	if (argc != 2) {
		fprintf(stderr, "Need a document to operate on");
		return 1;
	}

	doc = getDoc(argv[1]);
	
	struct TransitionMatrix tm = getTransitionMatrix(doc);

	printTransitionMatrix(&tm);

	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0;
}
