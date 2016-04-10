#include <stdio.h>
#include <string.h>
#include <time.h>
#include <linux/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

// General xml parsing stuff
//////////////////////////////
xmlDoc *getDoc(char *docName) {
	xmlDocPtr doc = NULL;

	doc = xmlParseFile(docName);

	if(doc == NULL) {
		fprintf(stderr, "error: Couldn't parse file %s\n", docName);
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
	for(int i=0; i<notes->nodeNr; i++) {
		removeChildNode(notes->nodeTab[i], "stem");
		removeChildNode(notes->nodeTab[i], "type");
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
	xmlNodeSet *notes;

	doc = getDoc(argv[1]);
	root = xmlDocGetRootElement(doc);
	notes = makeNodeSet(doc, "//note");

	if(stripNotes(notes)) {
		fprintf(stderr, "Couldn't strip the notes \n");
		return 1;
	}

	for(int i=0; i<notes->nodeNr; i++) {
		int count = 0;
		for(int j=0; j<notes->nodeNr; j++) {
			char *note1 = (char *) xmlNodeGetContent(notes->nodeTab[i]);
			char *note2 = (char *) xmlNodeGetContent(notes->nodeTab[j]);
			if(strcmp(note1, note2) == 0) count++;
		}

		printf("Note %s %s has %i instances \n", getNoteName(notes->nodeTab[i]), getNoteDuration(notes->nodeTab[i]), count);
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
