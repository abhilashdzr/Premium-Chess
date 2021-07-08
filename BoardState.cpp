#include "BoardState.h"
#include "Board.h"

uint8_t **BoardState::board;
bool BoardState::whiteTurn;
bool BoardState::whitecanCastleKingSide, BoardState::whitecanCastleQueenSide;
bool BoardState::blackcanCastleKingSide, BoardState::blackcanCastleQueenSide;
pii BoardState::enPassant;

BoardState::~BoardState() {
	for (int x = 0; x < 8; x++) {
		delete[] board[x];
	}
	delete[] board;
}

uint8_t** BoardState::getBoard() {
	return board;
}

void BoardState::setBoard(uint8_t** newBoard) {
	board = newBoard;
}

pii BoardState::EnPassant() {
	return enPassant;
}

void BoardState::init() {
	whitecanCastleKingSide = whitecanCastleQueenSide = true; // king n rook unmoved
    blackcanCastleKingSide = blackcanCastleQueenSide = true; // king n rook unmoved
	whiteTurn = true;
	enPassant = mp(-1, -1);
}
