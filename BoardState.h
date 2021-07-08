#pragma once

#include <utility> // pair
#include <stdint.h> // uint8_t
using namespace std;

class BoardState
{
public:

	~BoardState();
	static uint8_t ** getBoard();
	void setBoard(uint8_t** );
	static void init();
	static pair<int,int> EnPassant();


	static uint8_t **board;
	static bool whiteTurn;

	static bool whitecanCastleKingSide, whitecanCastleQueenSide;
	static bool blackcanCastleKingSide, blackcanCastleQueenSide;

	static pair<int,int> enPassant;
};