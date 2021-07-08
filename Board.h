#pragma once

#include "Move.h"
#include "BoardState.h"
#include "Piece.h"
#include "KingThreatenedInfo.h"

#include <SDL.h>
#include <vector>
using namespace std;

class Board
{
private:

	KingThreatenedInfo whiteThreatened;
	KingThreatenedInfo blackThreatened;

	BoardState* boardState;

	//moving piece stuff.
	bool draggingPiece;
	pii draggingPieceBox;
	SDL_Texture* draggingPieceTexture;

	//Move stuff
	vector<moveInfo> pseudoLegalMoves;
	vector<moveInfo> legalMoves;

	vector<pii> highlightBoxes;

	pii highlightKingBox;
	pii winnerKing;

	moveInfo promotionMove;
	bool gameOver;															//reqd

	bool waitingForPromotionChoice;											//required

	bool whiteWinner;															//required

public:

	~Board();
	void render(BoardState*);
	void init();
	bool boundCheck(int x, int y);
	void renderDraggedPiece(int type);
	void loadBoardFromFen(const char*, BoardState*);

	void handleMouseButtonDown(SDL_MouseButtonEvent&, BoardState*);
	void attemptPickupPiece(int x, int y, BoardState*);
	void attemptPlacePiece(int x, int y, BoardState*);
	void stopDraggingPiece();

	BoardState* getBoardState();

	void renderPieces(BoardState*);

	SDL_Texture* getTextureAtLocation(int x, int y, BoardState*);
	void display_piece(SDL_Renderer* , SDL_Texture* , pii , int );


	bool pieceIsCurrentPlayersPiece(int x, int y, BoardState*);

	void switchTurns(BoardState*);

	//updates the en passant squares.
	void updateEnPassant(pii fromBox, pii toBox, BoardState*);			//**
	bool isEnPassant(pii fromBox, pii toBox, BoardState*);				//**

	//updates castling stuff.
	void updateCastling(pii fromBox, pii toBox, BoardState*);			//**

	//calculating legal move stuff
	void clearMoves();
	//vector<moveInfo> calculatePseudoLegalMoves(BoardState*);			//**
	vector<moveInfo> calculateLegalMoves(BoardState*);					//**

	void makeMove(moveInfo, BoardState*);


	void calculateRookMoves(pii, BoardState*, vector<moveInfo>&);
	void calculateBishopMoves(pii, BoardState*, vector<moveInfo>&);
	void calculateKnightMoves(pii, BoardState*, vector<moveInfo>&);
	void calculateQueenMoves(pii, BoardState*, vector<moveInfo>&);
	void calculatePawnUpMoves(pii, BoardState*, vector<moveInfo>&);
	void calculatePawnDownMoves(pii, BoardState*, vector<moveInfo>&);

	void calculateRookLegalMoves(pii, pii, BoardState*, vector<moveInfo>&, KingThreatenedInfo);
	void calculateKingLegalMoves(pii, BoardState*, vector<moveInfo>&, KingThreatenedInfo);
	void calculateBishopLegalMoves(pii, pii, BoardState*, vector<moveInfo>&, KingThreatenedInfo);
	void calculateQueenLegalMoves(pii, pii, BoardState*, vector<moveInfo>&, KingThreatenedInfo);
	void calculatePawnLegalMoves(pii, pii, BoardState*, vector<moveInfo>&, KingThreatenedInfo);
	void calculateKnightLegalMoves(pii, pii, BoardState*, vector<moveInfo>&, KingThreatenedInfo);
	void calculateCastlingLegalMoves(pii, BoardState*, vector<moveInfo>&);

	void attemptAddMove(moveInfo move, BoardState* currentBoardState, vector<moveInfo>& moves); 		//**

	void addStraightUpMoves(pii, BoardState*, vector<moveInfo>&);									//**
	void addStraightDownMoves(pii, BoardState*, vector<moveInfo>&);								//**
	void addStraightLeftMoves(pii, BoardState*, vector<moveInfo>&);								//**
	void addStraightRightMoves(pii, BoardState*, vector<moveInfo>&);								//**

	void addUpRightMoves(pii, BoardState*, vector<moveInfo>&);										//**
	void addDownRightMoves(pii, BoardState*, vector<moveInfo>&);									//**
	void addUpLeftMoves(pii, BoardState*, vector<moveInfo>&);										//**
	void addDownLeftMoves(pii, BoardState*, vector<moveInfo>&);									//**

	void addStraightUpPawnMoves(pii, BoardState*, vector<moveInfo>&);								//**
	void addStraightDownPawnMoves(pii, BoardState*, vector<moveInfo>&);							//**
	void addDownLeftPawnMoves(pii, BoardState*, vector<moveInfo>&);								//**
	void addDownRightPawnMoves(pii, BoardState*, vector<moveInfo>&);								//**
	void addUpRightPawnMoves(pii, BoardState*, vector<moveInfo>&);									//**
	void addUpLeftPawnMoves(pii, BoardState*, vector<moveInfo>&);									//**


	bool inSameRow(pii, pii);
	bool inSameCol(pii, pii);
	//bottom left to top right
	bool inSameDiagonal(pii, pii);

	//bottom left to top right.
	bool inSameReverseDiagonal(pii, pii);


	void nextTurn(BoardState* boardState);
	bool inLegalMoves(moveInfo&);
	bool inPseudoMoves(moveInfo&);


	bool canEnPassant(pii, pii, BoardState*);


	void updateThreats(moveInfo, BoardState*);
	void updateAllThreats(BoardState* currentBoardState);
	void updateStraightUpThreats( BoardState*);
	void updateStraightLeftThreats( BoardState* boardState);
	void updateStraightDownThreats( BoardState*);
	void updateStraightRightThreats( BoardState* boardState);

	void updateUpLeftThreats( BoardState*);
	void updateUpRightThreats( BoardState* boardState);
	void updateDownLeftThreats( BoardState*);
	void updateDownRightThreats( BoardState* boardState);
	//king threatened stuff
	void initializeKingsThreatened(BoardState*);


	bool kingAttacked(BoardState* currentBoardState);

	bool doesBoxBlockAttack(pii, BoardState* currentBoardState);


	//void renderAttackedSquares(BoardState* currentBoardState);

	void renderBox(pii, SDL_Color);
	void renderHighlightMoves();
	void renderPreviousMove();
	void createHighlightMoves(int x, int y);

	//check and checkmate stuff
	bool kingInCheck(BoardState* currentBoardState);
	void findKingLocation(pii*, BoardState*);
	bool squareAttacked(pii box, BoardState* currentBoardState);


	void updateHighlightKingBox(BoardState*);																//**																	//dorkar nei

	//game over stuff.
	int isGameOver(BoardState* currentBoardState);												//done

	void reset();

	void renderPromotionOptions(BoardState*);
	void togglePromotionOptions();
	void tryPickingPromotionPiece(int, int, BoardState*);


	//random moves down here:


	//piece location stuff
	void initializePieceLocations(BoardState*);												//done


};