#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GameView.h"
#include "linkedList.h"
#include "Game.h"
#include "Places.h"
#include "helper.h"

// There is not listRep at the moment, just becuase there isnt enough time
// and because I will have to edit a lot of funtcions. 

void newNode (Player player, char *pastPlays, int i, node firstMove[], node lastMove[]) {

	node newElem = malloc(sizeof(struct nodeElement));

	newElem->turnStr = strndup(&pastPlays[i], 7);
	newElem->id = strToId(pastPlays, i + 1);
	newElem->next = NULL; 

	if (firstMove[player] == NULL) {
        newElem->moveno = 1; 
		newElem->prev = NULL; 
		firstMove[player] = newElem;
		lastMove[player] = newElem;
	} else {
		newElem->prev = lastMove[player]; 
		lastMove[player]->next = newElem;
		lastMove[player] = newElem; 
        newElem->moveno = newElem->prev->moveno + 1;
	}
	
}

void freeList (node curr) {
	while (curr != NULL) {
		node next = curr->next;
		free(curr->turnStr);
		free(curr);
		curr = next; 
	}
}

// Gets the nth latest node for dracula's double back stuff
// Recursively get location if getHiddenLoc lands on a HIDE or DOUBLE BACK
node getHiddenLoc(node lastnode, int n) {
	// Should change n to the actual movenumber 
	n -= 102;
	if (n == 0) n++;	// Because HIDE and DOUBLE BACK 1 are the same 
	for (int i = 0; i < n; i++) {
		lastnode = lastnode->prev; 
	}
	if (lastnode->id > 101 && lastnode->id < 108) {
		lastnode = getHiddenLoc(lastnode, lastnode->id); 
	}
	return lastnode;
}