#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <set> // set
#include <utility> // pair
#include <stdint.h> // uint8_t
using namespace std;

#define pii pair<int, int>
#define mp make_pair
#define ff first
#define ss second

class Piece
{
public:
	static void init();
	static void clear();

	static void loadPieces();
	static void destroyPieces();

	static void setKingLocation(pii);
	static void removePiece(uint8_t, pii);
	static void addPiece(uint8_t, pii);
	static void updatePiece(uint8_t, pii, pii);

	static SDL_Texture *wk, *wq, *wr, *wb, *wn, *wp;
	static SDL_Texture *bk, *bq, *br, *bb, *bn, *bp;

	//first 6 bits for type
    const static uint8_t king = (1 << 0);
    const static uint8_t queen = (1 << 1);
    const static uint8_t rook = (1 << 2);
    const static uint8_t bishop = (1 << 3);
    const static uint8_t knight = (1 << 4);
    const static uint8_t pawn = (1 << 5);
    //last 2 bits for color (both zero means empty)
    const static uint8_t white = (1 << 6);
    const static uint8_t black = (1 << 7);
    
	static pii wkLoc, bkLoc;
	static set<pii> wqLoc, wrLoc, wbLoc, wnLoc, wpLoc, bqLoc, brLoc, bbLoc, bnLoc, bpLoc;
};
