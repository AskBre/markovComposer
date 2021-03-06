#include <stdio.h> 
#include <string.h>
#include <time.h>
#include <math.h>
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

	free(nodePath);

	return set;
}

int isNodeInNodeArray(xmlNode *node, struct MarkovNode *nodes, int arraySize) {
	char *nodeString1 = (char *) xmlNodeGetContent(node);
	for(int i=0; i<arraySize; i++) {
		char *nodeString2 = (char *) xmlNodeGetContent(nodes[i].node);
		if(strcmp(nodeString1, nodeString2) == 0) {
			return 1;
		}
		free(nodeString2);
	}
	free(nodeString1);

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

	free(nodeString);

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

	free(cur);
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

	free(cur);

	if(!child) {
		fprintf(stderr, "Couldn't find child %s \n", childName);
		return 1;
	} else {
		xmlUnlinkNode(child);
		xmlFreeNode(child);
		return 0;
	}

}

int removeReduntantNodes(xmlNode *parent) {
	xmlNode *cur = NULL;
	xmlNode *child = NULL;

	for(cur = parent->children; cur; cur = cur->next) {
		int isPitch = (xmlStrcmp(cur->name, (const xmlChar *) "pitch") == 0);
		int isDuration = (xmlStrcmp(cur->name, (const xmlChar *) "duration") == 0);
		int isType = (xmlStrcmp(cur->name, (const xmlChar *) "type") == 0);
		int isNeeded = isPitch + isDuration + isType;

		if(isNeeded == 0) {
			printf("%s isn't needed \n", cur->name);
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}
	}

	free(cur);

	return 0;	
}


//////////////////////////////

// Getting of note properties
//////////////////////////////
char *getNoteName(xmlNode *note) {
	xmlNode *pitch = getChildNode(note, "pitch");
	xmlNode *step = getChildNode(pitch, "step");

	free(pitch);
	if(step) return (char *) xmlNodeGetContent(step->xmlChildrenNode);
	else {
		free(step);
		return NULL;
	}
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


/*
int stripNodes(xmlNodeSet *notes) {
	// Retain only the core components of note for comparison
	for(int i=0; i<notes->nodeNr; i++) {
		removeChildNode(notes->nodeTab[i], "beam");
		removeChildNode(notes->nodeTab[i], "stem");
		removeChildNode(notes->nodeTab[i], "alter");
	}

	return 0;
}
*/
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

	free(origNodeString);
	free(wantedNodeString);

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
		free(nodeString2);
	}

	free(nodeString1);

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
				if(playhead < RES) {
					node->nextNodeIndex[playhead] = j;
					playhead++;
				} else {
					fprintf(stderr, "Playhead of %i is too big %i \n", playhead, k);
				}
			}

			free(nodeString);
		}

	}

	return 0;
}

/*
struct MarkovNode *getNextNode(struct MarkovNode *currentNode, struct MarkovNodeArray *nodes) {
	struct MarkovNode *nextNode;
	printf("R is %i\n", r);
	printf("Index is %i\n", index);
	printf("Curnote is %s\n", xmlNodeGetContent(currentNode->node));

	if(index < 0) {
		return NULL;
	}

	printf("Nextnote is %s\n", xmlNodeGetContent(nextNode->node));
	return nextNode;
}
*/

int printNodeArray(struct MarkovNodeArray *a) {
	for(int n=0; n<a->size; n++) {
		printf("Array of note %s \n is \n", (char *) xmlNodeGetContent(a->n[n].node));
		for(int i=0; i<RES; i++) {
			printf("%i ", a->n[n].nextNodeIndex[i]);
		}
		printf("\n");
	}

	return 0;
}

int removeNodesFromDoc(xmlNodeSet *nodes) {
	for(int i=0; i<nodes->nodeNr; i++) {
		xmlUnlinkNode(nodes->nodeTab[i]);
		xmlFreeNode(nodes->nodeTab[i]);
	}

	return 0;
}

int removeAttrFromNodeSet(xmlNodeSet *nodes) {
	for(int i=0; i<nodes->nodeNr; i++) {
		while(xmlRemoveProp(nodes->nodeTab[i]->properties) == 0);
	}

	return 0;
}

xmlDoc *makeNewScoreFromOld(xmlDoc *oldDoc) {
	xmlDoc *doc = xmlCopyDoc(oldDoc, 1);

	xmlNodeSet *ns;
	ns = makeNodeSet(doc, "//note");
	removeNodesFromDoc(ns);
	ns = makeNodeSet(doc, "//measure");
	removeNodesFromDoc(ns);
	free(ns);

	return doc;
}

int stripNodes(xmlDoc *doc, xmlNodeSet *notes) {
	xmlNodeSet *ns;
	removeAttrFromNodeSet(notes);


	ns = makeNodeSet(doc, "//stem");
	removeNodesFromDoc(ns);

	ns = makeNodeSet(doc, "//grace");
	removeNodesFromDoc(ns);

	/*
	ns = makeNodeSet(doc, "//beam");
	removeNodesFromDoc(ns);

	ns = makeNodeSet(doc, "//alter");
	removeNodesFromDoc(ns);

	ns = makeNodeSet(doc, "//notations");
	removeNodesFromDoc(ns);

	ns = makeNodeSet(doc, "//accidental");
	removeNodesFromDoc(ns);
	*/
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

	struct MarkovNodeArray *uniqueNotes;

	srand(time(NULL));

	// Get the notes from file as a set of nodes
	doc = getDoc(argv[1]);
	root = xmlDocGetRootElement(doc);
	allNotes = makeNodeSet(doc, "//note");
	printf("Made nodeset of notes with size %i \n", allNotes->nodeNr);

	// Remove irrelevant parts of the notes
	if(stripNodes(doc, allNotes)) {
		fprintf(stderr, "Couldn't strip the notes \n");
		return 1;
	}
	printf("Stripped the notes \n");

	// Make an array of unique notes
	uniqueNotes = malloc(sizeof(struct MarkovNodeArray));
	fillUniqueNodeArrayFromNodeSet(uniqueNotes, allNotes);
	printf("Made string array of unique notes with size %i \n", uniqueNotes->size);

	makeIndexArrayInNodes(uniqueNotes, allNotes);

//	printNodeArray(uniqueNotes);

	xmlDoc *score = makeNewScoreFromOld(doc);
	xmlNode *scoreRoot = xmlDocGetRootElement(score);
	xmlNode *part = getChildNode(scoreRoot, "part");
	xmlNode *measure = xmlNewChild(part, NULL, BAD_CAST "measure", NULL);
	xmlNewProp (measure, BAD_CAST "number", BAD_CAST "1");
	
	int r = rand() % RES;
	int index = uniqueNotes->n[0].nextNodeIndex[r];
	int beatCount = 0;
	int measureNum = 1;
	int count = 0;

	while(count < 5000) {
		if(measureNum < 65) {
			if(index >= 0) {
				//			printf("Note is \n %s \n", xmlNodeGetContent(uniqueNotes->n[index].node));
				//			printf("New note %i \n", index);

				struct MarkovNode *newNode = &uniqueNotes->n[index];

				char *beat = getNoteDuration(newNode->node);
				if(beat) {
					beatCount += atoi(beat);
					//				printf("Beatcount %i \n", beatCount);
					if(beatCount%24 == 0) {
						//printf("New measure \n");
						measureNum++;
						measure = xmlNewChild(part, NULL, BAD_CAST "measure", NULL);
						char n[10];
						sprintf(n, "%i", measureNum);
						xmlNewProp(measure, BAD_CAST "number",  BAD_CAST n);
					}

				} else {
					fprintf(stderr, "Couldn't find note duration \n");
					xmlNode *dur = xmlNewChild(newNode->node, NULL, BAD_CAST "duration", BAD_CAST "6");
					xmlNode *voi = getChildNode(newNode->node, "voice");
					xmlAddPrevSibling(voi, dur);
				}

				if(measureNum%46 == 0) {
					xmlNode *newPage = xmlNewChild(measure, NULL, BAD_CAST "print", NULL);
					xmlNewProp(newPage, BAD_CAST "new-page",  BAD_CAST "yes");
				}

				xmlAddChild(measure, xmlCopyNode(newNode->node, 1));

				r = rand() % RES;
				index = newNode->nextNodeIndex[r];

				free(beat);
			}
		}
		count++;
	}

	printf("Åsså trøkker du på DONE \n");
	xmlSaveFormatFile("data/MarkovScore.xml", score, 1);
	xmlFreeDoc(score);
	xmlFreeDoc(doc);
}
