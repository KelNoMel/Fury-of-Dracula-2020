// An ADT for the linked list used in the assignment


#include "GameView.h"
#include "Game.h"
#include "Places.h"

typedef struct nodeElement *node;

struct nodeElement{
	PlaceId id;		// stores the PlaceId for of the player's turn
	node next;		// Stores the node containing information on the player's next turn
	node prev;		// Stores the node containing information on the player's previous turn
	int moveno;		// The round which the player has acted in
	char *turnStr; 	// Contains a 7 btye string depicting the player's turn
};

// Appends a node to the list associated to player. 
void newNode (Player player, char *pastPlays, int i, node firstMove[], node lastMove[]);

// Frees all nodes starting from the start of the list
void freeList (node firstMove); 

// Finds the location of a HIDE or DOUBLE BACK, recursively if needed. 
node getHiddenLoc(node lastnode, int n);