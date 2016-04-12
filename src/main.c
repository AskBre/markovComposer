#include <stdio.h>
#include <string.h>
#include <time.h>
#include <linux/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define RES 100

struct MarkovNode {
	xmlNode *node;
	int nextNodeIndex[RES];
};

struct MarkovNodeArray {
	struct MarkovNode *n;
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
	xmlXPathObject *nodePath;
	xmlChar *keyword = (xmlChar *) _keyword;

	// Set nodePath
	nodePath = xmlXPathEvalExpression(keyword, xmlXPathNewContext(doc));

	// Get notenames
	set = nodePath->nodesetval;

	if(!set) fprintf(stderr, "Couldn't make set from %s \n", (char *) keyword);

	return set;
}

int isNodeInNodeArray(xmlNode *node, struct MarkovNode *nodes, int arraySize) {
	char *nodeString1 = (char *) xmlNodeGetContent(node);
	for(int i=0; i<arraySize; i++) {
		char *nodeString2 = (char *) xmlNodeGetContent(nodes[i].node);
		if(strcmp(nodeString1, nodeString2) == 0) {
			return 1;
		}
	}

	return 0;
}

int fillUniqueNodeArrayFromNodeSet(struct MarkovNodeArray *nodes, xmlNodeSet *nodeSet) {
	char *nodeString = (char *) xmlNodeGetContent(nodeSet->nodeTab[0]);

	nodes->allocated = nodeSet->nodeNr;
	nodes->size = 0;
	nodes->n = malloc(nodes->allocated * sizeof(struct MarkovNode));

	if(!nodes->n) {
		fprintf(stderr, "Failed to allocate memory");
		return 1;
	}

	for(int i=0; i<nodeSet->nodeNr; i++) {
		xmlNode *node = nodeSet->nodeTab[i];
		nodeString = (char *) xmlNodeGetContent(node);
		struct MarkovNode *n = &nodes->n[nodes->size];

		if(!isNodeInNodeArray(node, nodes->n, nodes->size)) {
			n->node = malloc(sizeof(node));
			if(!n->node) {
				fprintf(stderr, "Failed to allocate memory");
				return 1;
			}

			n->node = node;
			nodes->size++;
		}

		for(int i=0; i<RES; i++) {
			n->nextNodeIndex[i] = -1;
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

int getNextNodeCount(xmlNode *origNode, xmlNode *wantedNode, xmlNodeSet *nodeSet) {
	// Get count of how many wanted notes coming after original note
	int count = 0;
	char *origNodeString = (char *) xmlNodeGetContent(origNode);
	char *wantedNodeString = (char *) xmlNodeGetContent(wantedNode);

	for(int i=0; i<nodeSet->nodeNr; i++) {
		char *curNodeString = (char *) xmlNodeGetContent(nodeSet->nodeTab[i]);
		if((strcmp(origNodeString, curNodeString) == 0) && (i+1 < nodeSet->nodeNr)) {
			char *nextNodeString = (char *) xmlNodeGetContent(nodeSet->nodeTab[i+1]);
			if(strcmp(wantedNodeString, nextNodeString) == 0) {
				count++;
			}
		}
	}

	return count;
}

int getNodeCount(xmlNode *node, xmlNodeSet *ns) {
	int count = 0;
	char *nodeString1 = (char *) xmlNodeGetContent(node);
	for(int i=0; i<ns->nodeNr; i++) {
		char *nodeString2 = (char *) xmlNodeGetContent(ns->nodeTab[i]);
		if((strcmp(nodeString1, nodeString2) == 0)) {
			count++;
		}
	}

	return count;
}

int makeIndexArrayInNodes(struct MarkovNodeArray *nodes, xmlNodeSet *allNodes) {
	for(int i=0; i<nodes->size; i++) {
		struct MarkovNode *node = &nodes->n[i];
		int playhead = 0;

		for(int j=0; j<nodes->size; j++) {
			xmlNode *curNode = nodes->n[j].node;
			char *nodeString = (char *) xmlNodeGetContent(node->node);
			int nodeCount = getNodeCount(node->node, allNodes);
			int count = getNextNodeCount(node->node, curNode, allNodes);
			float percentage = ((float) count/(float) nodeCount) * RES;

			for(int k=0; k<percentage; k++) {
				if(playhead<RES) {
					node->nextNodeIndex[playhead] = j;
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
	struct MarkovNodeArray *uniqueNotes;
	uniqueNotes = malloc(sizeof(struct MarkovNodeArray));
	fillUniqueNodeArrayFromNodeSet(uniqueNotes, allNotes);
	printf("Made string array of unique notes with size %i \n", uniqueNotes->size);

	makeIndexArrayInNodes(uniqueNotes, allNotes);

	for(int n=0; n<uniqueNotes->size; n++) {
		printf("Array of note %s \n is \n", (char *) xmlNodeGetContent(uniqueNotes->n[n].node));
		for(int i=0; i<RES; i++) {
			printf("%i ", uniqueNotes->n[n].nextNodeIndex[i]);
		}
		printf("\n");
	}

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
