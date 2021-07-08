#pragma once

#include <list> // list
#include <utility> // pair
#include <stdint.h> // uint8_t
#include <iostream>
using namespace std;

#define pii pair<int, int>
#define mp make_pair
#define ff first
#define ss second

struct moveInfo
{
	pii from, to; // ff is col(file), ss is row(rank)
	bool tookAnotherPiece;
	bool kingSideCastle, queenSideCastle;
	bool pawnAdvanced, enPassant;
	uint8_t promotedPieceType; // only type, not color
	moveInfo() // defaultConstructor
	{
		tookAnotherPiece = kingSideCastle = queenSideCastle = pawnAdvanced = enPassant = promotedPieceType = 0;
	}
};

class Move
{
public:
	static int fiftyMovesCounter;
	static list<moveInfo> lastThree;
	static moveInfo prevMove;

	static void init();
	static void setprevMove(moveInfo);
	static bool isDraw();
};
