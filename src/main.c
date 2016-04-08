#include <stdio.h>
#include <string.h>
#include <time.h>
#include <linux/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

xmlDoc *getDoc(char *docName) {
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

xmlNode *getChildNode(xmlNode *parent, char *childName) {
	xmlNode *cur = NULL;
	xmlNode *child = NULL;

	for(cur = parent->children; cur; cur = cur->next) {
		if(xmlStrcmp(cur->name, (const xmlChar *) childName) == 0) {
			child = cur;
		}
	}

	return child;
}

// Getting of note properties
//////////////////////////////
char *getNoteName(xmlNode *note) {
	xmlNode *pitch = getChildNode(note, "pitch");
	xmlNode *step = getChildNode(pitch, "step");

	return (char *) xmlNodeGetContent(step->xmlChildrenNode);
}

char *getNoteOctave(xmlNode *note) {
	xmlNode *pitch = getChildNode(note, "pitch");
	xmlNode *octave = getChildNode(pitch, "octave");

	return (char *) xmlNodeGetContent(octave->xmlChildrenNode);
}

//////////////////////////////

int fillNotes(xmlDoc *doc, xmlNodeSet *notes) {

	xmlChar *keyword = (xmlChar*) "//note";
	xmlXPathObject *notePath;

	// Set notePath
	notePath = xmlXPathEvalExpression(keyword, xmlXPathNewContext(doc));

	// Get notenames
	notes = notePath->nodesetval;

	for(int i=0; i<notes->nodeNr; i++) {
		int count = 0;
		for(int j=0; j<notes->nodeNr; j++) {
			if(strcmp(getNoteName(notes->nodeTab[i]), getNoteName(notes->nodeTab[j])) == 0) count++;
		}
		printf("Note %s has %i instances \n", getNoteName(notes->nodeTab[i]), count);
	}

	return 0;
}

int main(int argc, char **argv) {
	// Load xml-file
	if (argc != 2) {
		fprintf(stderr, "Need a document to operate on");
		return 1;
	}

	xmlDoc *doc;
	xmlNode *root;
	xmlNodeSet *notes;

	doc = getDoc(argv[1]);

	root = xmlDocGetRootElement(doc);

	if(fillNotes(doc, notes)) {
		fprintf(stderr, "Can't fill the notes");
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
