#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "markovAnalyser.h"

struct Note {
	char name;
	int count;
	float chance;
};

struct TransitionMatrix {
	struct Note *notes;
	int size;
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

int addNoteToMatrix(char n, struct TransitionMatrix tm) {
	// Check if n exists in tm.name
	// If then tm.count++
	// if not then new tm tm.name=n
	return 0;
}

struct TransitionMatrix getTransitionMatrix(xmlDocPtr doc) {
	struct TransitionMatrix tm;

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

	char **noteNames = calloc(noteCount, sizeof(struct Note));

	for(int i=0; i< xmlNotes->nodeNr; i++) {
		char xmlNoteName;
		xmlNoteName = (char)xmlNodeListGetString(doc, xmlNotes->nodeTab[i]->xmlChildrenNode, 1)[0];
		printf("Note: %c\n", xmlNoteName);

		noteNames[i] = malloc(sizeof(xmlNoteName));
//		strcpy(noteNames[i], xmlNoteName);

		addNoteToMatrix(xmlNoteName);
	}
	
	xmlXPathFreeObject(notePath);

	return tm;
}

int main(int argc, char **argv) {
	xmlDocPtr doc = NULL;

	if (argc != 2) {
		fprintf(stderr, "Need a document to operate on");
		return 1;
	}

	doc = getDoc(argv[1]);
	
	struct TransitionMatrix tm = getTransitionMatrix(doc);

	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0;
}
