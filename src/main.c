#include <stdio.h>
#include <string.h>
#include <time.h>
#include <linux/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

struct StringArray{
	char **strings;
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

int isStringInArray(char *string, char **array, int arraySize) {
	for(int i=0; i<arraySize; i++) {
		if(strcmp(string, array[i]) == 0) {
			return 1;
		}
	}

	return 0;
}

int fillUniqueStringListFromNodeSet(struct StringArray *sa, xmlNodeSet *ns) {
	char *s1 = (char *) xmlNodeGetContent(ns->nodeTab[0]);

	sa->allocated = ns->nodeNr;
	sa->strings = malloc(sa->allocated * sizeof(s1));

	for(int i=0; i<ns->nodeNr; i++) {
		s1 = (char *) xmlNodeGetContent(ns->nodeTab[i]);
		if(!isStringInArray(s1, sa->strings, sa->size)) {
			sa->strings[sa->size] = s1;	
			sa->size++;
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


int main(int argc, char **argv) {
	// Load xml-file
	if (argc != 2) {
		fprintf(stderr, "Need a document to operate on \n");
		return 1;
	}

	xmlDoc *doc;
	xmlNode *root;
	xmlNodeSet *allNotes;

	doc = getDoc(argv[1]);
	root = xmlDocGetRootElement(doc);
	allNotes = makeNodeSet(doc, "//note");
	printf("Made nodeset of notes with size %i \n", allNotes->nodeNr);

	if(stripNotes(allNotes)) {
		fprintf(stderr, "Couldn't strip the notes \n");
		return 1;
	}
	printf("Stripped the notes \n");

	struct StringArray *uniqueNotes;
	uniqueNotes = malloc(sizeof(struct StringArray));
	fillUniqueStringListFromNodeSet(uniqueNotes, allNotes);
	printf("Made string array of unique notes with size %i \n", uniqueNotes->size);

	for(int i=0; i<uniqueNotes->size; i++) {
	}

	xmlFreeDoc(doc);

	// Get next note
	// Check if it exists in transition matrix
	// Fill transmat for note
	//
	// Get first note
	// Put next note from transition matrix
	// Continute ad nauseum
}
