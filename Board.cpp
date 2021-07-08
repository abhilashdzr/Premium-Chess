#include "Board.h"
#include "Window.h"
#include "KingThreatenedInfo.h"
#include "BoardState.h"
#include "Piece.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
using namespace std;

#define HIGHLIGHT_COLOR {125, 235, 225, 100} // possible_moves
#define ATTACK_COLOR {225,25, 25, 100} // king_in_check
#define WIN_COLOR {225, 225, 25, 100} // king_won
#define LAST_MOVE_COLOR {10, 210, 100, 100} // previous_move

void Board::reset() { init(); }

void Board::init()
{
    Piece::init();
    Move::init();
    boardState = new BoardState();
    boardState->init();

    uint8_t** board = new uint8_t * [8];
    for (int i = 0; i < 8; ++i)
        board[i] = new uint8_t[8];

    for (int ff = 0; ff < 8; ++ff)
        for (int ss = 0; ss < 8; ++ss)
            board[ff][ss] = 0;

    boardState->setBoard(board);

    loadBoardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", boardState);
    initializePieceLocations(boardState);
    initializeKingsThreatened(boardState);
    legalMoves = calculateLegalMoves(boardState);

    draggingPieceBox = highlightKingBox = winnerKing = mp(-1, -1);
    gameOver = draggingPiece = false;
}

bool Board::boundCheck(int x, int y) { return (x >= 0 && y >= 0 && x < 8 && y < 8); }

BoardState* Board::getBoardState() { return boardState; }

void Board::loadBoardFromFen(const char* fenNotation, BoardState* currentBoardState) // without moveClock
{
    int column, index = 0;
    uint8_t** board = currentBoardState->getBoard();
    for (int rank = 0; rank < 8; rank++)
    {
        column = 0;
        while (fenNotation[index] != '/' && fenNotation[index] != ' ')
        {
            if (isdigit(fenNotation[index]))
                column += (fenNotation[index++] - '0');
            else
            {
                switch (fenNotation[index])
                {
                case 'P':
                    board[column][rank] = Piece::white | Piece::pawn;
                    break;
                case 'p':
                    board[column][rank] = Piece::black | Piece::pawn;
                    break;
                case 'R':
                    board[column][rank] = Piece::white | Piece::rook;
                    break;
                case 'r':
                    board[column][rank] = Piece::black | Piece::rook;
                    break;
                case 'N':
                    board[column][rank] = Piece::white | Piece::knight;
                    break;
                case 'n':
                    board[column][rank] = Piece::black | Piece::knight;
                    break;
                case 'B':
                    board[column][rank] = Piece::white | Piece::bishop;
                    break;
                case 'b':
                    board[column][rank] = Piece::black | Piece::bishop;
                    break;
                case 'Q':
                    board[column][rank] = Piece::white | Piece::queen;
                    break;
                case 'q':
                    board[column][rank] = Piece::black | Piece::queen;
                    break;
                case 'K':
                    board[column][rank] = Piece::white | Piece::king;
                    break;
                case 'k':
                    board[column][rank] = Piece::black | Piece::king;
                    break;
                }
                index++, column++;
            }
        }
        index++;
    }

    currentBoardState->init();
    currentBoardState->whiteTurn = (fenNotation[index] == 'w') ? 1 : 0;
    index += 2;

    if (fenNotation[index] == '-')
        index += 2;
    else
    {
        while (fenNotation[index] != ' ')
        {
            switch (fenNotation[index])
            {
            case 'K':
                currentBoardState->whitecanCastleKingSide = true;
                break;
            case 'k':
                currentBoardState->blackcanCastleKingSide = true;
                break;
            case 'Q':
                currentBoardState->whitecanCastleQueenSide = true;
                break;
            case 'q':
                currentBoardState->blackcanCastleQueenSide = true;
                break;
            }
            index++;
        }
        index++;
    }

    //now we are onto the en-passant option.

    if (fenNotation[index] == '-')
        index += 2, currentBoardState->enPassant = mp(-1, -1);
    else
    {
        currentBoardState->enPassant.ff = (fenNotation[index++] - 'a');
        currentBoardState->enPassant.ss = (8 - (fenNotation[index] - '0'));
        index++;
    }
}

void Board::display_piece(SDL_Renderer* rend, SDL_Texture* pieceTexture, pair<int, int> pos, int type)
{
    // struct to hold the position and size of the sprite
    SDL_Rect src, dest;

    // set the dimensions of src
    SDL_QueryTexture(pieceTexture, NULL, NULL, &src.w, &src.h);
    src.x = 0, src.y = 0;

    // origin is at top left corner, +ve direction is at right and down
    dest.x = (100 * pos.first) + 23, dest.y = 100 * pos.second, dest.w = 54;
    if (type == Piece::king || type == Piece::queen)
        dest.h = 95;
    else if (type == Piece::bishop)
        dest.h = 85;
    else if (type == Piece::rook || type == Piece::knight)
        dest.h = 80;
    else if (type == Piece::pawn)
        dest.h = 70;
    dest.y += (95 - dest.h);

    SDL_RenderCopy(rend, pieceTexture, &src, &dest);
    // display_piece(rend, Piece::wk, make_pair(4, 7), Piece::king);
}

void Board::render(BoardState* currentBoardState)
{
    if (draggingPiece)
        renderHighlightMoves();
    else if (Move::prevMove.from != mp(-1, -1) && Move::prevMove.to != mp(-1, -1))
        renderPreviousMove();

    if (highlightKingBox != mp(-1, -1))
        renderBox(highlightKingBox, ATTACK_COLOR);
    if (winnerKing != mp(-1, -1))
        renderBox(winnerKing, WIN_COLOR);

    renderPieces(currentBoardState);

    if (draggingPiece)
        renderDraggedPiece(currentBoardState->getBoard()[draggingPieceBox.ff][draggingPieceBox.ss]);
    if (waitingForPromotionChoice)
        renderPromotionOptions(currentBoardState);
}

void Board::renderDraggedPiece(int piece)
{
    uint8_t type = piece & (~(Piece::white | Piece::black));

    SDL_Rect src, dest;
    SDL_QueryTexture(draggingPieceTexture, NULL, NULL, &src.w, &src.h);
    src.x = 0, src.y = 0;
    SDL_GetMouseState(&dest.x, &dest.y);
    dest.x -= 25, dest.y -= 40, dest.w = 54;

    if (type == Piece::king || type == Piece::queen)
        dest.h = 95;
    else if (type == Piece::bishop)
        dest.h = 85;
    else if (type == Piece::rook || type == Piece::knight)
        dest.h = 80;
    else if (type == Piece::pawn)
        dest.h = 70;
    dest.y += (95 - dest.h);

    SDL_RenderCopy(Window::rend, draggingPieceTexture, &src, &dest);
}

void Board::renderPromotionOptions(BoardState* currentBoardState)
{
    SDL_Rect renderRect, fromRect;
    renderRect.w = 200, renderRect.h = 300, renderRect.x = 0, renderRect.y = 300;
    fromRect.x = fromRect.y = 0;

    if (currentBoardState->whiteTurn)
    {
        SDL_QueryTexture(Piece::wq, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::wq, &fromRect, &renderRect);
        renderRect.x += renderRect.w;

        SDL_QueryTexture(Piece::wr, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::wr, &fromRect, &renderRect);
        renderRect.x += renderRect.w;

        SDL_QueryTexture(Piece::wb, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::wb, &fromRect, &renderRect);
        renderRect.x += renderRect.w;

        SDL_QueryTexture(Piece::wn, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::wn, &fromRect, &renderRect);
        renderRect.x += renderRect.w;
    }
    else
    {
        SDL_QueryTexture(Piece::bq, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::bq, &fromRect, &renderRect);
        renderRect.x += renderRect.w;

        SDL_QueryTexture(Piece::br, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::br, &fromRect, &renderRect);
        renderRect.x += renderRect.w;

        SDL_QueryTexture(Piece::bb, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::bb, &fromRect, &renderRect);
        renderRect.x += renderRect.w;

        SDL_QueryTexture(Piece::bn, NULL, NULL, &fromRect.w, &fromRect.h);
        SDL_RenderCopy(Window::rend, Piece::bn, &fromRect, &renderRect);
        renderRect.x += renderRect.w;
    }
}

bool Board::inLegalMoves(moveInfo &newMove)
{
    for (int i = 0; i < legalMoves.size(); i++)
    {
        if (newMove.to == legalMoves[i].to && newMove.from == legalMoves[i].from)
        {
            newMove = legalMoves[i];
            return true;
        }
    }
    return false;
}

bool Board::inPseudoMoves( moveInfo& newMove) {

    for (int i = 0; i < pseudoLegalMoves.size(); i++) {

        if (newMove.from == pseudoLegalMoves[i].from && newMove.to == pseudoLegalMoves[i].to)
        {
            newMove = pseudoLegalMoves[i];
            return true;
        }
    }
    return false;
}

void Board::handleMouseButtonDown(SDL_MouseButtonEvent& b, BoardState* currentBoardState)
{
    int x, y, boardX, boardY;
    if (b.button == SDL_BUTTON_LEFT)
    {
        SDL_GetMouseState(&x, &y);
        if (x >= 0 && y < 800 && y >= 0 && y < 800)
        {
            boardX = x / 100;
            boardY = y / 100;
            cout << boardX << ", " << boardY << "\n";
            if (waitingForPromotionChoice)
                tryPickingPromotionPiece(boardX, boardY, currentBoardState);
            else if (!draggingPiece)
                attemptPickupPiece(boardX, boardY, currentBoardState);
            else
                attemptPlacePiece(boardX, boardY, currentBoardState);
        }
    }
}

void Board::attemptPickupPiece(int x, int y, BoardState* currentBoardState)
{
    if (currentBoardState->getBoard()[x][y] && pieceIsCurrentPlayersPiece(x, y, currentBoardState))
    {
        draggingPiece = true, draggingPieceBox = mp(x, y);
        uint8_t piece = currentBoardState->getBoard()[x][y];
        if (piece & Piece::white)
        {
            if (piece & Piece::king)
                draggingPieceTexture = Piece::wk;
            else if (piece & Piece::queen)
                draggingPieceTexture = Piece::wq;
            else if (piece & Piece::rook)
                draggingPieceTexture = Piece::wr;
            else if (piece & Piece::bishop)
                draggingPieceTexture = Piece::wb;
            else if (piece & Piece::knight)
                draggingPieceTexture = Piece::wn;
            else if (piece & Piece::pawn)
                draggingPieceTexture = Piece::wp;
        }
        else
        {
            if (piece & Piece::king)
                draggingPieceTexture = Piece::bk;
            else if (piece & Piece::queen)
                draggingPieceTexture = Piece::bq;
            else if (piece & Piece::rook)
                draggingPieceTexture = Piece::br;
            else if (piece & Piece::bishop)
                draggingPieceTexture = Piece::bb;
            else if (piece & Piece::knight)
                draggingPieceTexture = Piece::bn;
            else if (piece & Piece::pawn)
                draggingPieceTexture = Piece::bp;
        }
        createHighlightMoves(x, y);
        Piece::removePiece(piece, mp(x, y));
    }
}

void Board::createHighlightMoves(int x, int y) {
    highlightBoxes.clear();
    for (int i = 0; i < legalMoves.size(); i++) {
        if (legalMoves[i].from == mp(x, y))
            highlightBoxes.push_back(legalMoves[i].to);
    }
}

void Board::attemptPlacePiece(int x, int y, BoardState* currentBoardState)
{
    uint8_t** board = currentBoardState->getBoard();
    moveInfo newMove;
    newMove.from = draggingPieceBox, newMove.to = mp(x, y);

    if (inLegalMoves(newMove)) // inLegalMoves passes in a reference so we update the castling and stuff here
    {
        waitingForPromotionChoice = newMove.promotedPieceType;
        makeMove(newMove, currentBoardState);
        nextTurn(currentBoardState);
    }
    else
        Piece::updatePiece(board[draggingPieceBox.ff][draggingPieceBox.ss], draggingPieceBox, draggingPieceBox);
    stopDraggingPiece();
}

void Board::stopDraggingPiece() { draggingPiece = false, draggingPieceBox = mp(-1, -1); }

void Board::updateHighlightKingBox(BoardState* currentBoardState)
{
    if (kingInCheck(currentBoardState))
        findKingLocation(&highlightKingBox, currentBoardState);
    else
        highlightKingBox = mp(-1, -1);
    if (gameOver)
    {
        switchTurns(currentBoardState);
        findKingLocation(&winnerKing, currentBoardState);
    }
}

void Board::findKingLocation(pii* Pos, BoardState* currentBoardState)
{
    if (currentBoardState->whiteTurn)
        *Pos = Piece::wkLoc;
    else
        *Pos = Piece::bkLoc;
}

bool Board::pieceIsCurrentPlayersPiece(int x, int y, BoardState* currentBoardState)
{
    int color = ((currentBoardState->whiteTurn) * Piece::white) | (((currentBoardState->whiteTurn) ^ 1) * Piece::black);
    return ((currentBoardState->getBoard()[x][y] & color) == color);
}

bool Board::inSameRow(pii box1, pii box2) { return box1.ss == box2.ss; }
bool Board::inSameCol(pii box1, pii box2) { return box1.ff == box2.ff; }
bool Board::inSameDiagonal(pii box1, pii box2) { return ((box1.ss - box2.ss) == -(box1.ff - box2.ff)); }
bool Board::inSameReverseDiagonal(pii box1, pii box2) { return ((box1.ss - box2.ss) == (box1.ff - box2.ff)); }

void Board::clearMoves()
{
    pseudoLegalMoves.clear();
    legalMoves.clear();
}


bool Board::canEnPassant(pii pawnBox, pii to, BoardState* currentBoardState) {           //pawnBox takes to
    pii kingBox;
    uint8_t enemyPiece;
    uint8_t** board = currentBoardState->getBoard();
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        enemyPiece = Piece::black;
    }
    else {
        kingBox = Piece::bkLoc;
        enemyPiece = Piece::white;
    }

    if (inSameRow(kingBox, mp(pawnBox.ff, pawnBox.ss))) {
        //if the Piece::pawn is on the right of the Piece::king
        if (pawnBox.ff > kingBox.ff) {
            //enPassanting to the right, therefore taking a Piece::pawn away from pawnBox.ff + 1
            if (to.ff > pawnBox.ff) {
                for (int i = pawnBox.ff + 2; i < 8; ++i) {
                    if (board[i][pawnBox.ss] == 0)
                        continue;
                    else if ((board[i][pawnBox.ss] == (enemyPiece | Piece::rook)) || (board[i][pawnBox.ss] == (enemyPiece | Piece::queen))) {
                        for (int j = pawnBox.ff - 1; j > kingBox.ff; --j) {
                            if (board[j][pawnBox.ss] != 0)  return true;
                        }
                        return false;
                    }
                    else   //if there's some other piece there.
                        break;
                }
            }
            //enPassanting to the left, therefore taking a Piece::pawn away from pawnBox.ff -1 but we still search from the right side
            else {
                for (int i = pawnBox.ff + 1; i < 8; ++i) {
                    if (board[i][pawnBox.ss] == 0)
                        continue;
                    else if ((board[i][pawnBox.ss] == (enemyPiece | Piece::rook)) || (board[i][pawnBox.ss] == (enemyPiece | Piece::queen))) {
                        for (int j = pawnBox.ff - 2; j > kingBox.ff; --j) {
                            if (board[j][pawnBox.ss] != 0) return true;
                        }
                        return false;
                    }
                    else   //if there's some other piece there.
                        break;

                }
            }

        }
        else {
            //en passanting to right on the left side of the Piece::king
            if (to.ff > pawnBox.ff) {
                for (int i = pawnBox.ff - 1; i >= 0; --i) {
                    if (board[i][pawnBox.ss] == 0) {
                        continue;
                    }
                    else if ((board[i][pawnBox.ss] == (enemyPiece | Piece::rook)) || (board[i][pawnBox.ss] == (enemyPiece | Piece::queen))) {
                        for (int j = pawnBox.ff + 2; j < kingBox.ff; j++)
                            if (board[j][pawnBox.ss] != 0)  return true;

                        return false;
                    }
                    else   //if there's some other piece there.
                        break;
                }
            }
            //en passanting to the left on the left side of the Piece::king.
            else {
                for (int i = pawnBox.ff - 2; i >= 0; --i) {
                    if (board[i][pawnBox.ss] == 0) {
                        continue;
                    }
                    else if ((board[i][pawnBox.ss] == (enemyPiece | Piece::rook)) || (board[i][pawnBox.ss] == (enemyPiece | Piece::queen))) {
                        for (int j = pawnBox.ff + 1; j < kingBox.ff; j++)
                            if (board[j][pawnBox.ss] != 0)   return true;

                        return false;
                    }
                    else   //if there's some other piece there.
                        break;
                }
            }

        }

    }
    return true;
}

bool Board::isEnPassant(pii from, pii to, BoardState * currentBoardState) {
    if ((currentBoardState->getBoard()[from.ff][from.ss] & Piece::pawn) == Piece::pawn) {
        if (to == currentBoardState->EnPassant())
            return true;
    }
    return false;
}

void Board::updateEnPassant(pii from, pii to, BoardState * currentBoardState) {
    if ((currentBoardState->getBoard()[from.ff][from.ss] & Piece::pawn) == Piece::pawn) {
        if (abs(from.ss - to.ss) == 2)
            currentBoardState->enPassant = mp(from.ff, (from.ss + to.ss) / 2);
    }
    else
        currentBoardState->enPassant = mp(-1, -1);
}

//update castling status.
void Board::updateCastling(pii from, pii to, BoardState * currentBoardState) {
    int queenSideX = 0;
    int kingSideX = 7;
    int whiteY = 7;
    int blackY = 0;

    uint8_t** board = currentBoardState->getBoard();
    if (currentBoardState->whiteTurn) {
        // king moves
        if ((board[from.ff][from.ss] & Piece::king) == Piece::king) {
            currentBoardState->whitecanCastleKingSide = false;
            currentBoardState->whitecanCastleQueenSide = false;
        }

        // rook moves
        if (from.ff == queenSideX && from.ss == whiteY)
            currentBoardState->whitecanCastleQueenSide = false;
        else if (from.ff == kingSideX && from.ss == whiteY)
            currentBoardState->whitecanCastleKingSide = false;

        // rook is taken
        if (to.ff == kingSideX && to.ss == blackY)
            currentBoardState->whitecanCastleKingSide = false;
        else if (to.ff == queenSideX && to.ss == blackY)
            currentBoardState->whitecanCastleQueenSide = false;

    }
    else {
        if ((board[from.ff][from.ss] & Piece::king) == Piece::king) {
            currentBoardState->blackcanCastleKingSide = false;
            currentBoardState->blackcanCastleQueenSide = false;
        }

        if (from.ff == queenSideX && from.ss == blackY)
            currentBoardState->blackcanCastleQueenSide = false;
        else if (from.ff == kingSideX && from.ss == blackY)
            currentBoardState->blackcanCastleKingSide = false;

        if (to.ff == kingSideX && to.ss == whiteY)
            currentBoardState->blackcanCastleKingSide = false;
        else if (to.ff == queenSideX && to.ss == whiteY)
            currentBoardState->blackcanCastleQueenSide = false;
    }
}

void Board::makeMove(moveInfo move, BoardState * currentBoardState) {

    uint8_t** board = currentBoardState->getBoard();

    int enPassantX = currentBoardState->enPassant.ff, enPassantY = currentBoardState->enPassant.ss; // moveInfo.to
    if (isEnPassant(move.from, move.to, currentBoardState))
    {
        if (currentBoardState->whiteTurn) {
            board[enPassantX][enPassantY + 1] = 0;
            Piece::removePiece((Piece::black | Piece::pawn), mp(enPassantX, enPassantY + 1));
        }
        else {
            board[enPassantX][enPassantY - 1] = 0;
            Piece::removePiece((Piece::white | Piece::pawn), mp(enPassantX, enPassantY - 1));
        }
    }

    if (move.kingSideCastle)
    {
        board[move.to.ff - 1][move.to.ss] = board[move.to.ff + 1][move.to.ss];
        board[move.to.ff + 1][move.to.ss] = 0;
        Piece::updatePiece(board[move.to.ff - 1][move.to.ss], mp(move.to.ff + 1, move.to.ss), mp(move.to.ff - 1, move.to.ss));
    }
    else if (move.queenSideCastle)
    {
        board[move.to.ff + 1][move.to.ss] = board[move.to.ff - 2][move.to.ss];
        board[move.to.ff - 2][move.to.ss] = 0;
        Piece::updatePiece(board[move.to.ff + 1][move.to.ss], mp(move.to.ff - 2, move.to.ss), mp(move.to.ff + 1, move.to.ss));
    }

    updateEnPassant(move.from, move.to, currentBoardState);
    updateCastling(move.from, move.to, currentBoardState);

    if (board[move.to.ff][move.to.ss] != 0) {
        move.tookAnotherPiece = true;
        Piece::removePiece(board[move.to.ff][move.to.ss], move.to);
    }

    if (move.promotedPieceType) {
        switch (move.promotedPieceType) {
        case Piece::queen:
            if (currentBoardState->whiteTurn) {
                board[move.to.ff][move.to.ss] = Piece::white | Piece::queen;
                Piece::wqLoc.insert(move.to);
                Piece::wpLoc.erase(move.from);
            }
            else {
                board[move.to.ff][move.to.ss] = Piece::black | Piece::queen;
                Piece::bqLoc.insert(move.to);
                Piece::bpLoc.erase(move.from);
            }
            break;

        case Piece::rook:
            if (currentBoardState->whiteTurn) {
                board[move.to.ff][move.to.ss] = Piece::white | Piece::rook;
                Piece::wrLoc.insert(move.to);
                Piece::wpLoc.erase(move.from);
            }
            else {
                board[move.to.ff][move.to.ss] = Piece::black | Piece::rook;
                Piece::brLoc.insert(move.to);
                Piece::bpLoc.erase(move.from);
            }
            break;

        case Piece::knight:
            if (currentBoardState->whiteTurn) {
                board[move.to.ff][move.to.ss] = Piece::white | Piece::knight;
                Piece::wnLoc.insert(move.to);
                Piece::wpLoc.erase(move.from);
            }
            else {
                board[move.to.ff][move.to.ss] = Piece::black | Piece::knight;
                Piece::bnLoc.insert(move.to);
                Piece::bpLoc.erase(move.from);
            }
            break;

        case Piece::bishop:
            if (currentBoardState->whiteTurn) {
                board[move.to.ff][move.to.ss] = Piece::white | Piece::bishop;
                Piece::wbLoc.insert(move.to);
                Piece::wpLoc.erase(move.from);
            }
            else {
                board[move.to.ff][move.to.ss] = Piece::black | Piece::bishop;
                Piece::bbLoc.insert(move.to);
                Piece::bpLoc.erase(move.from);
            }
            break;
        }
    }
    else
    {
        board[move.to.ff][move.to.ss] = board[move.from.ff][move.from.ss];
        Piece::updatePiece(board[move.to.ff][move.to.ss], move.from, move.to);
    }

    board[move.from.ff][move.from.ss] = 0; //empty the prev position

    if (currentBoardState->whiteTurn)
        whiteThreatened.attackedInfo = 0, whiteThreatened.amountAttacked = 0;
    else
        blackThreatened.attackedInfo = 0, blackThreatened.amountAttacked = 0;

    Move::setprevMove(move);
    updateThreats(move, currentBoardState); // threat for current player
    switchTurns(currentBoardState);
    updateThreats(move, currentBoardState); // threat for next player
}

vector<moveInfo> Board::calculateLegalMoves(BoardState *currentBoardState) {
    vector<moveInfo> currentLegal;
    if (currentBoardState->whiteTurn) {
        calculateKingLegalMoves(Piece::wkLoc, currentBoardState, currentLegal, whiteThreatened);
        if (whiteThreatened.amountAttacked != 2) {
            for (auto it : Piece::wrLoc)
                calculateRookLegalMoves(it, Piece::wkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::wnLoc)
                calculateKnightLegalMoves(it, Piece::wkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::wqLoc)
                calculateQueenLegalMoves(it, Piece::wkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::wpLoc)
                calculatePawnLegalMoves(it, Piece::wkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::wbLoc)
                calculateBishopLegalMoves(it, Piece::wkLoc, currentBoardState, currentLegal, whiteThreatened);
        }
    }
    else {
        calculateKingLegalMoves(Piece::bkLoc, currentBoardState, currentLegal, blackThreatened);
        if (blackThreatened.amountAttacked != 2) {
            for (auto it : Piece::brLoc)
                calculateRookLegalMoves(it, Piece::bkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::bnLoc)
                calculateKnightLegalMoves(it, Piece::bkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::bqLoc)
                calculateQueenLegalMoves(it, Piece::bkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::bpLoc)
                calculatePawnLegalMoves(it, Piece::bkLoc, currentBoardState, currentLegal, whiteThreatened);
            for (auto it : Piece::bbLoc)
                calculateBishopLegalMoves(it, Piece::bkLoc, currentBoardState, currentLegal, whiteThreatened);
        }
    }
    return currentLegal;
}

void Board::calculateRookLegalMoves(pii rookBox, pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves, KingThreatenedInfo currentKingInfo) {
    //programming this assuming the Piece::king is not in check.
    bool addAll = true;
    if (rookBox.ff == kingBox.ff) {

        //if the Piece::rook is above the Piece::king.
        if (rookBox.ss < kingBox.ss) {
            //if the Piece::king is threatened from above we only add the Piece::rook moves that are straight up and down.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightUpThreatened) == KingThreatenedInfo::straightUpThreatened) {
                if (currentKingInfo.straightUpBox.ss < rookBox.ss) {
                    addStraightDownMoves(rookBox, currentBoardState, currentLegalMoves);
                    addStraightUpMoves(rookBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }


        }
        //the Piece::rook is below the Piece::king.
        else if (rookBox.ss > kingBox.ss) {
            //if the Piece::king is threatened from below we only add moves that add the Piece::rook up and down.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightDownThreatened) == KingThreatenedInfo::straightDownThreatened) {
                if (currentKingInfo.straightDownBox.ss > rookBox.ss) {
                    addStraightDownMoves(rookBox, currentBoardState, currentLegalMoves);
                    addStraightUpMoves(rookBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }
    }
    else if (rookBox.ss == kingBox.ss) {
        //Piece::rook is to the right of the Piece::king.
        if (rookBox.ff > kingBox.ff) {
            //if the Piece::king is threatened from right we only add the Piece::rook moves that are straight up and down.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightRightThreatened) == KingThreatenedInfo::straightRightThreatened) {
                if (currentKingInfo.straightRightBox.ff > rookBox.ff) {
                    addStraightRightMoves(rookBox, currentBoardState, currentLegalMoves);
                    addStraightLeftMoves(rookBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }
            }

        }
        else if (rookBox.ff < kingBox.ff) {
            //if the Piece::king is threatened from left we only add the Piece::rook moves that are straight up and down.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightLeftThreatened) == KingThreatenedInfo::straightLeftThreatened) {
                if (currentKingInfo.straightLeftBox.ff < rookBox.ff) {
                    addStraightRightMoves(rookBox, currentBoardState, currentLegalMoves);
                    addStraightLeftMoves(rookBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }

    }
    else if (inSameDiagonal(rookBox, kingBox)) {
        //if the Piece::rook is above the Piece::king.
        if (rookBox.ss < kingBox.ss) {

            //if the Piece::king is attacked diagonally, we can't moveInfo the Piece::rook at all.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upRightThreatened) == KingThreatenedInfo::upRightThreatened) {
                if (currentKingInfo.upRightBox.ff > rookBox.ff) {
                    addAll = false;
                }

            }
        }
        //if the Piece::rook is below the Piece::king
        if (rookBox.ss > kingBox.ss) {
            //if the Piece::king is attacked diagonally, we can't moveInfo the Piece::rook at all.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downLeftThreatened) == KingThreatenedInfo::downLeftThreatened) {
                if (currentKingInfo.downLeftBox.ff < rookBox.ff) {
                    addAll = false;
                }

            }
        }
    }
    else if (inSameReverseDiagonal(rookBox, kingBox)) {
        //if the Piece::rook is above the Piece::king.
        if (rookBox.ss < kingBox.ss) {

            //if the Piece::king is attacked diagonally, we can't moveInfo the Piece::rook at all.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upLeftThreatened) == KingThreatenedInfo::upLeftThreatened) {
                if (currentKingInfo.upLeftBox.ff < rookBox.ff) {
                    addAll = false;
                }

            }
        }
        //if the Piece::rook is below the Piece::king
        if (rookBox.ss > kingBox.ss) {
            //if the Piece::king is attacked diagonally, we can't moveInfo the Piece::rook at all.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downRightThreatened) == KingThreatenedInfo::downRightThreatened) {
                if (currentKingInfo.downRightBox.ff > rookBox.ff) {
                    addAll = false;
                }

            }
        }
    }
    //if it's not in any of the things, we can just add on all the possible Piece::rook moves.
    if (addAll) {
        calculateRookMoves(rookBox, currentBoardState, currentLegalMoves);
    }
}

void Board::calculateKingLegalMoves(pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves, KingThreatenedInfo currentKingInfo)
{
    uint8_t** board = currentBoardState->getBoard();
    uint8_t myColor = (currentBoardState->whiteTurn) ? Piece::white : Piece::black;
    int dx[] = {0, 0, 1, 1, 1, -1, -1, -1}, dy[] = {1, -1, -1, 0, 1, -1, 0, 1};

    moveInfo kingMove;
    kingMove.from = kingBox;
    for (int k = 0; k < 8; k++)
    {
        int nx = kingBox.ff + dx[k], ny = kingBox.ss + dy[k];
        if (nx < 0 || ny < 0 || nx >= 8 || ny >= 8)
            continue;
        if ((board[nx][ny] & myColor) != myColor && (!squareAttacked(mp(nx, ny), currentBoardState)))
        {
            kingMove.to = mp(nx, ny), kingMove.tookAnotherPiece = board[nx][ny];
            currentLegalMoves.push_back(kingMove);
        }
    }
    calculateCastlingLegalMoves(kingBox, currentBoardState, currentLegalMoves);
}

void Board::calculateBishopLegalMoves(pii bishopBox, pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves, KingThreatenedInfo currentKingInfo) {
    //programming this assuming the Piece::king is not in check.
    bool addAll = true;
    if (inSameCol(bishopBox, kingBox)) {
        //if the Piece::bishop is above the Piece::king.
        if (bishopBox.ss < kingBox.ss) {
            //if the Piece::king is threatened from above we don't add Piece::bishop moves.
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightUpThreatened) == KingThreatenedInfo::straightUpThreatened) {
                if (currentKingInfo.straightUpBox.ss < bishopBox.ss) {
                    addAll = false;
                }
            }
        }
        //the Piece::bishop is below the Piece::king.
        else if (bishopBox.ss > kingBox.ss) {
            //no Piece::bishop moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightDownThreatened) == KingThreatenedInfo::straightDownThreatened) {
                if (currentKingInfo.straightDownBox.ss > bishopBox.ss) {
                    addAll = false;
                }
            }
        }
    }
    else if (inSameRow(bishopBox, kingBox)) {
        //Piece::bishop is to the right
        if (bishopBox.ff > kingBox.ff) {
            //no Piece::bishop moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightRightThreatened) == KingThreatenedInfo::straightRightThreatened) {
                if (currentKingInfo.straightRightBox.ff > bishopBox.ff) {
                    addAll = false;
                }
            }
        }
        else if (bishopBox.ff < kingBox.ff) {
            //add no Piece::bishop moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightLeftThreatened) == KingThreatenedInfo::straightLeftThreatened) {
                if (currentKingInfo.straightLeftBox.ff < bishopBox.ff) {
                    addAll = false;
                }
            }
        }
    }
    else if (inSameDiagonal(bishopBox, kingBox)) {
        //if the Piece::bishop is above the Piece::king.
        if (bishopBox.ss < kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upRightThreatened) == KingThreatenedInfo::upRightThreatened) {
                if (currentKingInfo.upRightBox.ff > bishopBox.ff) {
                    addUpRightMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addDownLeftMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }
            }
        }
        //if the Piece::bishop is below the Piece::king
        if (bishopBox.ss > kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downLeftThreatened) == KingThreatenedInfo::downLeftThreatened) {
                if (currentKingInfo.downLeftBox.ff < bishopBox.ff) {
                    addUpRightMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addDownLeftMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }
            }
        }
    }
    else if (inSameReverseDiagonal(bishopBox, kingBox)) {
        //if the Piece::bishop is above the Piece::king
        if (bishopBox.ss < kingBox.ss) {
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upLeftThreatened) == KingThreatenedInfo::upLeftThreatened) {
                if (currentKingInfo.upLeftBox.ff < bishopBox.ff) {
                    addUpLeftMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addDownRightMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }
            }
        }
        //if the Piece::rook is below the Piece::king
        if (bishopBox.ss > kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downRightThreatened) == KingThreatenedInfo::downRightThreatened) {
                if (currentKingInfo.downRightBox.ff > bishopBox.ff) {
                    addUpLeftMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addDownRightMoves(bishopBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }
            }
        }
    }

    //if it's not in any of the things, we can just add on all the possible Piece::rook moves.
    if (addAll) {
        calculateBishopMoves(bishopBox, currentBoardState, currentLegalMoves);
    }
}
void Board::calculateQueenLegalMoves(pii queenBox, pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves, KingThreatenedInfo currentKingInfo) {

    //programming this assuming the Piece::king is not in check.
    bool addAll = true;
    if (inSameCol(queenBox, kingBox)) {

        //if the Piece::queen is above the Piece::king.
        if (queenBox.ss < kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightUpThreatened) == KingThreatenedInfo::straightUpThreatened) {
                if (currentKingInfo.straightUpBox.ss < queenBox.ss) {
                    addStraightUpMoves(queenBox, currentBoardState, currentLegalMoves);
                    addStraightDownMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }


        }
        //the Piece::queen is below the Piece::king.
        else if (queenBox.ss > kingBox.ss) {
            //no Piece::queen moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightDownThreatened) == KingThreatenedInfo::straightDownThreatened) {
                if (currentKingInfo.straightDownBox.ss > queenBox.ss) {
                    addStraightUpMoves(queenBox, currentBoardState, currentLegalMoves);
                    addStraightDownMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }
    }
    else if (inSameRow(queenBox, kingBox)) {
        //Piece::queen is to the right
        if (queenBox.ff > kingBox.ff) {
            //no Piece::queen moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightRightThreatened) == KingThreatenedInfo::straightRightThreatened) {
                if (currentKingInfo.straightRightBox.ff > queenBox.ff) {
                    addStraightLeftMoves(queenBox, currentBoardState, currentLegalMoves);
                    addStraightRightMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }

        }
        else if (queenBox.ff < kingBox.ff) {
            //add no Piece::queen moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightLeftThreatened) == KingThreatenedInfo::straightLeftThreatened) {
                if (currentKingInfo.straightLeftBox.ff < queenBox.ff) {
                    addStraightLeftMoves(queenBox, currentBoardState, currentLegalMoves);
                    addStraightRightMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }

    }
    else if (inSameDiagonal(queenBox, kingBox)) {
        //Piece::queen above Piece::king
        if (queenBox.ss < kingBox.ss) {


            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upRightThreatened) == KingThreatenedInfo::upRightThreatened) {
                if (currentKingInfo.upRightBox.ff > queenBox.ff) {
                    addUpRightMoves(queenBox, currentBoardState, currentLegalMoves);
                    addDownLeftMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }
        //Piece::king below Piece::king
        if (queenBox.ss > kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downLeftThreatened) == KingThreatenedInfo::downLeftThreatened) {
                if (currentKingInfo.downLeftBox.ff < queenBox.ff) {
                    addUpRightMoves(queenBox, currentBoardState, currentLegalMoves);
                    addDownLeftMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }
    }
    else if (inSameReverseDiagonal(queenBox, kingBox)) {
        //if the Piece::queen is above the Piece::king
        if (queenBox.ss < kingBox.ss) {


            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upLeftThreatened) == KingThreatenedInfo::upLeftThreatened) {
                if (currentKingInfo.upLeftBox.ff < queenBox.ff) {
                    addUpLeftMoves(queenBox, currentBoardState, currentLegalMoves);
                    addDownRightMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }

            }
        }
        //if the Piece::queen is below the Piece::king
        if (queenBox.ss > kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downRightThreatened) == KingThreatenedInfo::downRightThreatened) {
                if (currentKingInfo.downRightBox.ff > queenBox.ff) {
                    addUpLeftMoves(queenBox, currentBoardState, currentLegalMoves);
                    addDownRightMoves(queenBox, currentBoardState, currentLegalMoves);
                    addAll = false;
                }
            }
        }
    }

    //if it's not in any of the things, we can just add on all the possible Piece::rook moves.
    if (addAll) {

        calculateQueenMoves(queenBox, currentBoardState, currentLegalMoves);
    }
}
void Board::calculatePawnLegalMoves(pii pawnBox, pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves, KingThreatenedInfo currentKingInfo) {
    bool addAll = true;

    if (currentBoardState->whiteTurn) {
        if (inSameCol(pawnBox, kingBox)) {

            //if the Piece::pawn is above the Piece::king.
            if (pawnBox.ss < kingBox.ss) {
                //if the Piece::king is threatened from above we only add the Piece::pawn moves that are straight up and down.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightUpThreatened) == KingThreatenedInfo::straightUpThreatened) {
                    if (currentKingInfo.straightUpBox.ss < pawnBox.ss) {
                        addStraightUpPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }
                }


            }
            //the Piece::pawn is below the Piece::king.
            else if (pawnBox.ss > kingBox.ss) {
                //if the Piece::king is threatened from below we only add moves that add the Piece::pawn up and down.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightDownThreatened) == KingThreatenedInfo::straightDownThreatened) {
                    if (currentKingInfo.straightDownBox.ss > pawnBox.ss) {
                        addStraightUpPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }
                }
            }
        }
        //if it's in the same row and i's attacked we reallllly can't moveInfo it.
        else if (inSameRow(pawnBox, kingBox)) {
            //Piece::pawn is to the right of the Piece::king.
            if (pawnBox.ff > kingBox.ff) {
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightRightThreatened) == KingThreatenedInfo::straightRightThreatened) {
                    if (currentKingInfo.straightRightBox.ff > pawnBox.ff) {
                        addAll = false;
                    }

                }

            }
            else if (pawnBox.ff < kingBox.ff) {
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightLeftThreatened) == KingThreatenedInfo::straightLeftThreatened) {
                    if (currentKingInfo.straightLeftBox.ff < pawnBox.ff) {
                        addAll = false;
                    }

                }
            }

        }
        else if (inSameDiagonal(pawnBox, kingBox)) {
            //if the Piece::pawn is above the Piece::king.
            if (pawnBox.ss < kingBox.ss) {

                //if the Piece::king is attacked diagonally, we can only moveInfo the Piece::pawn to the top right.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upRightThreatened) == KingThreatenedInfo::upRightThreatened) {
                    if (currentKingInfo.upRightBox.ff > pawnBox.ff) {
                        addUpRightPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }

                }
            }
            //if the Piece::pawn is below the Piece::king
            if (pawnBox.ss > kingBox.ss) {
                //if the Piece::king is attacked diagonally from below we can't moveInfo the Piece::pawn.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downLeftThreatened) == KingThreatenedInfo::downLeftThreatened) {
                    if (currentKingInfo.downLeftBox.ff < pawnBox.ff) {
                        addAll = false;
                    }

                }
            }
        }
        else if (inSameReverseDiagonal(pawnBox, kingBox)) {

            //if the Piece::pawn is above the Piece::king.
            if (pawnBox.ss < kingBox.ss) {
                //if the Piece::king is attacked diagonally, we can't moveInfo the Piece::pawn at all.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upLeftThreatened) == KingThreatenedInfo::upLeftThreatened) {
                    if (currentKingInfo.upLeftBox.ff < pawnBox.ff) {
                        addUpLeftPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }
                }
            }
            //if the Piece::pawn is below the Piece::king
            if (pawnBox.ss > kingBox.ss) {
                //if the Piece::king is attacked diagonally from below, we can't moveInfo the Piece::pawn.`
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downRightThreatened) == KingThreatenedInfo::downRightThreatened) {
                    if (currentKingInfo.downRightBox.ff > pawnBox.ff) {
                        addAll = false;
                    }

                }
            }
        }

        //if it's not in any of the things, we can just add on all the possible Piece::pawn moves.
        if (addAll) {
            calculatePawnUpMoves(pawnBox, currentBoardState, currentLegalMoves);
        }
    }
    //if it's blacks turn.
    else {
        if (inSameCol(pawnBox, kingBox)) {

            //if the Piece::pawn is above the Piece::king.
            if (pawnBox.ss < kingBox.ss) {
                //if the Piece::king is threatened from above we only add the Piece::pawn moves that are straight up and down.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightUpThreatened) == KingThreatenedInfo::straightUpThreatened) {
                    if (currentKingInfo.straightUpBox.ss < pawnBox.ss) {
                        addStraightDownPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }

                }


            }
            //the Piece::pawn is below the Piece::king.
            else if (pawnBox.ss > kingBox.ss) {
                //if the Piece::king is threatened from below we only add moves that add the Piece::pawn up and down.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightDownThreatened) == KingThreatenedInfo::straightDownThreatened) {
                    if (currentKingInfo.straightDownBox.ss > pawnBox.ss) {
                        addStraightDownPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }

                }
            }
        }
        //if it's in the same row and i's attacked we reallllly can't moveInfo it.
        else if (inSameRow(pawnBox, kingBox)) {
            //Piece::pawn is to the right of the Piece::king.
            if (pawnBox.ff > kingBox.ff) {
                //if the Piece::king is threatened from right we only add the Piece::pawn moves that are straight up and down.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightRightThreatened) == KingThreatenedInfo::straightRightThreatened) {
                    if (currentKingInfo.straightRightBox.ff > pawnBox.ff) {
                        addAll = false;
                    }

                }

            }
            else if (pawnBox.ff < kingBox.ff) {
                //if the Piece::king is threatened from left we only add the Piece::pawn moves that are straight up and down.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightLeftThreatened) == KingThreatenedInfo::straightLeftThreatened) {
                    if (currentKingInfo.straightLeftBox.ff < pawnBox.ff) {
                        addAll = false;
                    }

                }
            }

        }
        else if (inSameDiagonal(pawnBox, kingBox)) {
            //if the Piece::pawn is above the Piece::king.
            if (pawnBox.ss < kingBox.ss) {

                //if the Piece::king is attacked diagonally, we can only moveInfo the Piece::pawn to the bottom left.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upRightThreatened) == KingThreatenedInfo::upRightThreatened) {
                    if (currentKingInfo.upRightBox.ff > pawnBox.ff) {
                        addAll = false;
                    }

                }
            }
            //if the Piece::pawn is below the Piece::king
            if (pawnBox.ss > kingBox.ss) {
                //if the Piece::king is attacked diagonally from below we can't moveInfo the Piece::pawn.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downLeftThreatened) == KingThreatenedInfo::downLeftThreatened) {
                    if (currentKingInfo.downLeftBox.ff < pawnBox.ff) {
                        addDownLeftPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }

                }
            }
        }
        else if (inSameReverseDiagonal(pawnBox, kingBox)) {
            //if the Piece::pawn is above the Piece::king.
            if (pawnBox.ss < kingBox.ss) {

                //if the Piece::king is attacked diagonally, we can't moveInfo the Piece::pawn at all.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upLeftThreatened) == KingThreatenedInfo::upLeftThreatened) {
                    if (currentKingInfo.upLeftBox.ff < pawnBox.ff) {
                        addAll = false;
                    }

                }
            }
            //if the Piece::pawn is below the Piece::king
            if (pawnBox.ss > kingBox.ss) {
                //if the Piece::king is attacked diagonally from below, we can't moveInfo the Piece::pawn.
                if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downRightThreatened) == KingThreatenedInfo::downRightThreatened) {
                    if (currentKingInfo.downRightBox.ff > pawnBox.ff) {
                        addDownRightPawnMoves(pawnBox, currentBoardState, currentLegalMoves);
                        addAll = false;
                    }

                }
            }
        }

        //if it's not in any of the things, we can just add on all the possible Piece::pawn moves.
        if (addAll) {
            calculatePawnDownMoves(pawnBox, currentBoardState, currentLegalMoves);
        }
    }
}
void Board::calculateKnightLegalMoves(pii knightBox, pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves, KingThreatenedInfo currentKingInfo) {
    //programming this assuming the Piece::king is not in check.
    bool addAll = true;
    if (inSameCol(knightBox, kingBox)) {

        //if the Piece::knight is above the Piece::king.
        if (knightBox.ss < kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightUpThreatened) == KingThreatenedInfo::straightUpThreatened) {
                if (currentKingInfo.straightUpBox.ss < knightBox.ss) {
                    addAll = false;
                }
            }
        }
        //the Piece::knight is below the Piece::king.
        else if (knightBox.ss > kingBox.ss) {
            //no Piece::knight moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightDownThreatened) == KingThreatenedInfo::straightDownThreatened) {
                if (currentKingInfo.straightDownBox.ss > knightBox.ss) {
                    addAll = false;
                }

            }
        }
    }
    else if (inSameRow(knightBox, kingBox)) {
        //Piece::knight is to the right
        if (knightBox.ff > kingBox.ff) {
            //no Piece::knight moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightRightThreatened) == KingThreatenedInfo::straightRightThreatened) {
                if (currentKingInfo.straightRightBox.ff > knightBox.ff) {
                    addAll = false;
                }

            }

        }
        //Piece::knight is to the left
        else if (knightBox.ff < kingBox.ff) {
            //add no Piece::knight moves
            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::straightLeftThreatened) == KingThreatenedInfo::straightLeftThreatened) {
                if (currentKingInfo.straightLeftBox.ff < knightBox.ff) {
                    addAll = false;
                }
            }
        }

    }
    else if (inSameDiagonal(knightBox, kingBox)) {
        //Piece::knight above Piece::king
        if (knightBox.ss < kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upRightThreatened) == KingThreatenedInfo::upRightThreatened) {
                if (currentKingInfo.upRightBox.ff > knightBox.ff) {
                    addAll = false;
                }

            }
        }
        //Piece::knight below Piece::king
        if (knightBox.ss > kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downLeftThreatened) == KingThreatenedInfo::downLeftThreatened) {
                if (currentKingInfo.downLeftBox.ff < knightBox.ff) {
                    addAll = false;
                }

            }
        }
    }
    else if (inSameReverseDiagonal(knightBox, kingBox)) {
        //if the Piece::knight is above the Piece::king
        if (knightBox.ss < kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::upLeftThreatened) == KingThreatenedInfo::upLeftThreatened) {
                if (currentKingInfo.upLeftBox.ff < knightBox.ff) {
                    addAll = false;
                }

            }
        }
        //if the Piece::knight is below the Piece::king
        if (knightBox.ss > kingBox.ss) {

            if ((currentKingInfo.threatenedInfo & KingThreatenedInfo::downRightThreatened) == KingThreatenedInfo::downRightThreatened) {
                if (currentKingInfo.downRightBox.ff > knightBox.ff) {
                    addAll = false;
                }

            }
        }
    }

    //if it's not in any of the things, we can just add on all the possible Piece::knight moves.
    if (addAll) {
        calculateKnightMoves(knightBox, currentBoardState, currentLegalMoves);
    }
}
void Board::calculateCastlingLegalMoves(pii kingBox, BoardState* currentBoardState, vector<moveInfo>& currentLegalMoves) {
    int ff = kingBox.ff;
    int ss = kingBox.ss;
    uint8_t** board = currentBoardState->getBoard();
    int attackedAmount;

    if (currentBoardState->whiteTurn) attackedAmount = whiteThreatened.amountAttacked;
    else attackedAmount = blackThreatened.amountAttacked;
    //if the Piece::king is attacked you can't castle.
    if (attackedAmount != 0) return;

    moveInfo temp;
    temp.from = mp(ff, ss);

    if ((board[ff][ss] & Piece::white) == Piece::white) {
        if (currentBoardState->whitecanCastleKingSide) {
            if (board[ff + 1][ss] == 0 && board[ff + 2][ss] == 0) {
                if (!squareAttacked({ ff + 1, ss }, currentBoardState) && !squareAttacked({ ff + 2, ss }, currentBoardState)) {
                    temp.to = mp(ff + 2, ss);
                    temp.kingSideCastle = true;
                    currentLegalMoves.push_back(temp);
                }
            }
        }
        if (currentBoardState->whitecanCastleQueenSide) {
            if (board[ff - 1][ss] == 0 && board[ff - 2][ss] == 0 && board[ff - 3][ss] == 0) {
                if (!squareAttacked({ ff - 1, ss }, currentBoardState) && !squareAttacked({ ff - 2, ss }, currentBoardState) && !squareAttacked({ ff - 3, ss }, currentBoardState)) {
                    temp.to = mp(ff - 2, ss);
                    temp.queenSideCastle = true;
                    currentLegalMoves.push_back(temp);
                }
            }
        }
    }
    else {
        if (currentBoardState->blackcanCastleKingSide) {
            if (board[ff + 1][ss] == 0 && board[ff + 2][ss] == 0) {
                if (!squareAttacked({ ff + 1, ss }, currentBoardState) && !squareAttacked({ ff + 2, ss }, currentBoardState)) {
                    temp.to = mp(ff + 2, ss);
                    temp.kingSideCastle = true;
                    currentLegalMoves.push_back(temp);
                }
            }
        }
        if (currentBoardState->blackcanCastleQueenSide) {
            if (board[ff - 1][ss] == 0 && board[ff - 2][ss] == 0 && board[ff - 3][ss] == 0) {
                if (!squareAttacked({ ff - 1, ss }, currentBoardState) && !squareAttacked({ ff - 2, ss }, currentBoardState) && !squareAttacked({ ff - 3, ss }, currentBoardState)) {
                    temp.to = mp(ff - 2, ss);
                    temp.queenSideCastle = true;
                    currentLegalMoves.push_back(temp);
                }
            }
        }
    }
}

void Board::calculateRookMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& currentPseudo) {

    addStraightRightMoves(box, currentBoardState, currentPseudo);
    addStraightLeftMoves(box, currentBoardState, currentPseudo);

    addStraightUpMoves(box, currentBoardState, currentPseudo);
    addStraightDownMoves(box, currentBoardState, currentPseudo);
}
void Board::calculateBishopMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& currentPseudo) {

    addDownRightMoves(box, currentBoardState, currentPseudo);
    addDownLeftMoves(box, currentBoardState, currentPseudo);
    addUpLeftMoves(box, currentBoardState, currentPseudo);
    addUpRightMoves(box, currentBoardState, currentPseudo);
}
void Board::calculateKnightMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    int ff = box.ff, ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    vector<int>dx = {2, -2, 2, -2, 1, 1, -1, -1};
    vector<int>dy = {1, 1, -1, -1, 2, -2, 2, -2};
    int nx, ny;
    moveInfo temp; temp.from = box;
    for (int i = 0; i < 8; i++) {
        nx = ff + dx[i];
        ny = ss + dy[i];
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            if (board[nx][ny] == 0 || !pieceIsCurrentPlayersPiece(nx, ny, currentBoardState)) {
                temp.to = mp(nx, ny);
                temp.tookAnotherPiece = board[nx][ny];
                attemptAddMove(temp, currentBoardState, moves);
            }
        }
    }
}
void Board::calculateQueenMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& currentPseudo) {

    addDownRightMoves(box, currentBoardState, currentPseudo);
    addDownLeftMoves(box, currentBoardState, currentPseudo);
    addUpLeftMoves(box, currentBoardState, currentPseudo);
    addUpRightMoves(box, currentBoardState, currentPseudo);
    addStraightRightMoves(box, currentBoardState, currentPseudo);
    addStraightLeftMoves(box, currentBoardState, currentPseudo);
    addStraightUpMoves(box, currentBoardState, currentPseudo);
    addStraightDownMoves(box, currentBoardState, currentPseudo);
}
void Board::calculatePawnDownMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    addStraightDownPawnMoves(box, currentBoardState, moves);
    addDownLeftPawnMoves(box, currentBoardState, moves);
    addDownRightPawnMoves(box, currentBoardState, moves);
}
void Board::calculatePawnUpMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    addStraightUpPawnMoves(box, currentBoardState, moves);
    addUpLeftPawnMoves(box, currentBoardState, moves);
    addUpRightPawnMoves(box, currentBoardState, moves);
}



void Board::attemptAddMove(moveInfo move, BoardState* currentBoardState, vector<moveInfo>& moves)
{
    if (currentBoardState->whiteTurn)
    {
        if (whiteThreatened.amountAttacked >= 1)
        {
            if (doesBoxBlockAttack(move.to, currentBoardState))
                moves.push_back(move);
        }
        else
            moves.push_back(move);
    }
    else
    {
        if (blackThreatened.amountAttacked >= 1)
        {
            if (doesBoxBlockAttack(move.to, currentBoardState))
                moves.push_back(move);
        }
        else
            moves.push_back(move);
    }
}

bool Board::doesBoxBlockAttack(pii box, BoardState* currentBoardState) {
    uint8_t attackedInfo;
    pii attackedFrom;
    pii kingBox;
    bool attackedByKnight;

    if (currentBoardState->whiteTurn) {
        attackedInfo = whiteThreatened.attackedInfo;
        attackedFrom =  whiteThreatened.attackedFrom;
        attackedByKnight = whiteThreatened.attackedByKnight;
        kingBox = Piece::wkLoc;
    }
    else {
        attackedInfo = blackThreatened.attackedInfo;
        attackedFrom = blackThreatened.attackedFrom;
        attackedByKnight = blackThreatened.attackedByKnight;
        kingBox = Piece::bkLoc;
    }

    //when you're attacked by a Piece::knight the only way to block it with a piece is to take it.
    if (attackedByKnight) {
        if (box == attackedFrom) return true;
        else return false;
    }
    if (box == attackedFrom)
        return true;


    switch (attackedInfo) {
    case KingThreatenedInfo::straightDownThreatened:
        if (box.ff == attackedFrom.ff && box.ss < attackedFrom.ss && box.ss > kingBox.ss) return true;
        else return false;
        break;

    case KingThreatenedInfo::straightUpThreatened:
        if (box.ff == attackedFrom.ff && box.ss > attackedFrom.ss && box.ss < kingBox.ss) return true;
        else return false;
        break;

    case KingThreatenedInfo::straightLeftThreatened:
        if (box.ss == attackedFrom.ss && box.ff > attackedFrom.ff && box.ff < kingBox.ff) return true;
        else return false;
        break;

    case KingThreatenedInfo::straightRightThreatened:
        if (box.ss == attackedFrom.ss && box.ff < attackedFrom.ff && box.ff > kingBox.ff) return true;
        else return false;
        break;

    case KingThreatenedInfo::upLeftThreatened:
        if (inSameReverseDiagonal(box, kingBox)) {
            if (box.ff < kingBox.ff && box.ff > attackedFrom.ff) return true;
            else return false;
        }
        else return false;
        break;
    case KingThreatenedInfo::upRightThreatened:
        if (inSameDiagonal(box, kingBox)) {
            if (box.ff > kingBox.ff && box.ff < attackedFrom.ff) return true;
            else return false;
        }
        else return false;
        break;

    case KingThreatenedInfo::downLeftThreatened:
        if (inSameDiagonal(box, kingBox)) {
            if (box.ff < kingBox.ff && box.ff > attackedFrom.ff) return true;
            else return false;
        }
        else return false;
        break;

    case KingThreatenedInfo::downRightThreatened:
        if (inSameReverseDiagonal(box, kingBox)) {
            if (box.ff > kingBox.ff && box.ff < attackedFrom.ff) return true;
            else return false;
        }
        else return false;
        break;

    default:
        std::cout << "error in switch" << std::endl;
    }
}


void Board::addStraightUpMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    //going up on the board
    moveInfo temp;
    for (int currY = ss - 1; currY >= 0; --currY) {
        if (board[ff][currY] == 0)
        {
            temp.from = mp(ff, ss), temp.to = mp(ff, currY),  temp.tookAnotherPiece = board[ff][currY];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(ff, currY, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(ff, currY),  temp.tookAnotherPiece = board[ff][currY];
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}

void Board::addStraightDownMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int currY = ss + 1; currY < 8; ++currY) {
        if (board[ff][currY] == 0)
        {
            temp.from = mp(ff, ss), temp.to = mp(ff, currY),  temp.tookAnotherPiece = board[ff][currY];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(ff, currY, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(ff, currY),  temp.tookAnotherPiece = board[ff][currY];
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}

void Board::addStraightLeftMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    //going to the left on the board.
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int currX = ff - 1; currX >= 0; --currX) {
        if (board[currX][ss] == 0)
        {
            temp.from = mp(ff, ss), temp.to = mp(currX, ss),  temp.tookAnotherPiece = board[currX][ss];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(currX, ss, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(currX, ss),  temp.tookAnotherPiece = board[currX][ss];
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}

void Board::addStraightRightMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int currX = ff + 1; currX < 8; currX++) {
        if (board[currX][ss] == 0)
        {
            temp.from = mp(ff, ss), temp.to = mp(currX, ss);
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(currX, ss, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(currX, ss);
            attemptAddMove(temp, currentBoardState, moves),  temp.tookAnotherPiece = board[currX][ss];
            break;
        }
        else
            break;
    }
}
void Board::addUpRightMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    //going up and to the right
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int change = 1; ff + change < 8 && ss - change >= 0; ++change) {
        if (board[ff + change][ss - change] == 0) {
            temp.from = mp(ff, ss), temp.to = mp(ff + change, ss - change),  temp.tookAnotherPiece = board[ff + change][ss - change];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(ff + change, ss - change, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(ff + change, ss - change),  temp.tookAnotherPiece = board[ff + change][ss - change];
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}
void Board::addUpLeftMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    //going up and to the left.
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int change = 1; ff - change >= 0 && ss - change >= 0; ++change) {
        if (board[ff - change][ss - change] == 0) {
            temp.from = mp(ff, ss), temp.to = mp(ff - change, ss - change),  temp.tookAnotherPiece = board[ff - change][ss - change];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(ff - change, ss - change, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(ff - change, ss - change),  temp.tookAnotherPiece = board[ff - change][ss - change];
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}
void Board::addDownRightMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int change = 1; ff + change < 8 && ss + change < 8; ++change) {
        if (board[ff + change][ss + change] == 0) {
            temp.from = mp(ff, ss), temp.to = mp(ff + change, ss + change),  temp.tookAnotherPiece = board[ff + change][ss + change];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(ff + change, ss + change, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(ff + change, ss + change);
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}
void Board::addDownLeftMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    //going down and to the left
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    for (int change = 1; ff - change >= 0 && ss + change < 8; ++change) {
        if (board[ff - change][ss + change] == 0) {
            temp.from = mp(ff, ss), temp.to = mp(ff - change, ss + change),  temp.tookAnotherPiece = board[ff - change][ss + change];
            attemptAddMove(temp, currentBoardState, moves);
        }
        else if (!pieceIsCurrentPlayersPiece(ff - change, ss + change, currentBoardState)) {
            temp.from = mp(ff, ss), temp.to = mp(ff - change, ss + change),  temp.tookAnotherPiece = board[ff - change][ss + change];
            attemptAddMove(temp, currentBoardState, moves);
            break;
        }
        else
            break;
    }
}


void Board::addStraightUpPawnMoves(pii box, BoardState* currentBoardState, std::vector<moveInfo>& moves) {
    //moving forward one.
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    temp.from = mp(ff, ss);
    uint8_t myColor = ((currentBoardState->whiteTurn) * Piece::white) | (((currentBoardState->whiteTurn) ^ 1) * Piece::black);
    if (boundCheck(ff, ss - 1)) temp.tookAnotherPiece = board[ff][ss - 1];
    if (ss - 1 >= 0) {
        //pawns cant take vertically.
        if (board[ff][ss - 1] == 0) {
            //if it's a promotion
            if (ss - 1 == 0) {
                temp.to = mp(ff, ss - 1), temp.promotedPieceType = (myColor | Piece::queen), attemptAddMove(temp, currentBoardState, moves);
                temp.to = mp(ff, ss - 1), temp.promotedPieceType = (myColor | Piece::rook), attemptAddMove(temp, currentBoardState, moves);
                temp.to = mp(ff, ss - 1), temp.promotedPieceType = (myColor | Piece::knight), attemptAddMove(temp, currentBoardState, moves);
                temp.to = mp(ff, ss - 1), temp.promotedPieceType = (myColor | Piece::bishop), attemptAddMove(temp, currentBoardState, moves);
            }
            else {
                temp.to = mp(ff, ss - 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
            }
            //we only can move forward 2 if the space is open as well.
            if (ss == 8 - 2) {   //if it's in the starting position.
                if (ss - 2 >= 0) {   //this shouldn't be necessary except for tiny boards
                    if (currentBoardState->getBoard()[ff][ss - 2] == 0) {
                        temp.to = mp(ff, ss - 2), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                    }
                }
            }
        }
    }
}
void Board::addStraightDownPawnMoves(pii box, BoardState* currentBoardState, vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    temp.from = mp(ff, ss);
    if (boundCheck(ff, ss + 1)) temp.tookAnotherPiece = board[ff ][ss + 1];
    //moving forward one.
    if (ss + 1 < 8) {
        //pawns cant take vertically.
        if (board[ff][ss + 1] == 0) {
            if (ss + 1 == 8 - 1) {
                temp.to = mp(ff, ss + 1), temp.promotedPieceType = Piece::queen, attemptAddMove(temp, currentBoardState, moves);
                temp.to = mp(ff, ss + 1), temp.promotedPieceType = Piece::rook, attemptAddMove(temp, currentBoardState, moves);
                temp.to = mp(ff, ss + 1), temp.promotedPieceType = Piece::knight, attemptAddMove(temp, currentBoardState, moves);
                temp.to = mp(ff, ss + 1), temp.promotedPieceType = Piece::bishop, attemptAddMove(temp, currentBoardState, moves);
            }
            else {
                temp.to = mp(ff, ss + 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
            }
            //we only can moveInfo forward 2 if the space is open as well.
            if (ss == 1) {   //if it's in the starting position.
                if (ss + 2 < 8) {    //this shouldn't be necessary except for tiny boards
                    if (currentBoardState->getBoard()[ff][ss + 2] == 0) {
                        temp.to = mp(ff, ss + 2), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                    }
                }
            }
        }
    }
}
void Board::addDownLeftPawnMoves(pii box, BoardState* currentBoardState, std::vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    temp.from = mp(ff, ss);
    if (boundCheck(ff - 1, ss + 1)) temp.tookAnotherPiece = board[ff - 1][ss + 1];
    //pawns capture diagonally.
    if (ss + 1 < 8) {
        //if we're not at the edge of the board
        if (ff - 1 >= 0) {
            if (board[ff - 1][ss + 1] != 0 && !pieceIsCurrentPlayersPiece(ff - 1, ss + 1, currentBoardState)) {
                //promotion time
                if (ss + 1 == 8 - 1) {
                    temp.to = mp(ff - 1, ss + 1), temp.promotedPieceType = Piece::queen, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff - 1, ss + 1), temp.promotedPieceType = Piece::rook, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff - 1, ss + 1), temp.promotedPieceType = Piece::knight, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff - 1, ss + 1), temp.promotedPieceType = Piece::bishop, attemptAddMove(temp, currentBoardState, moves);
                }
                else {
                    temp.to = mp(ff - 1, ss + 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                }
            }
            else if (mp(ff - 1, ss + 1) == currentBoardState->EnPassant()) {
                if (canEnPassant({ ff, ss }, { ff - 1, ss + 1 }, currentBoardState)) {
                    temp.to = mp(ff - 1, ss + 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                }
            }
        }
    }
}
void Board::addDownRightPawnMoves(pii box, BoardState* currentBoardState, std::vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    temp.from = mp(ff, ss);
    if (boundCheck(ff + 1, ss + 1)) temp.tookAnotherPiece = board[ff + 1][ss + 1];
    if (ss + 1 < 8) {
        if (ff + 1 < 8) {
            if (board[ff + 1][ss + 1] != 0 && !pieceIsCurrentPlayersPiece(ff + 1, ss + 1, currentBoardState)) {
                if (ss + 1 == 8 - 1) {
                    temp.to = mp(ff + 1, ss + 1), temp.promotedPieceType = Piece::queen, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff + 1, ss + 1), temp.promotedPieceType = Piece::rook, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff + 1, ss + 1), temp.promotedPieceType = Piece::knight, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff + 1, ss + 1), temp.promotedPieceType = Piece::bishop, attemptAddMove(temp, currentBoardState, moves);
                }
                else {
                    temp.to = mp(ff + 1, ss + 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                }
            }
            else if (mp(ff + 1, ss + 1) == currentBoardState->EnPassant()) {
                if (canEnPassant({ ff, ss }, { ff + 1, ss + 1 }, currentBoardState)) {
                    temp.to = mp(ff + 1, ss + 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                }
            }
        }
    }
}
void Board::addUpRightPawnMoves(pii box, BoardState* currentBoardState, std::vector<moveInfo>& moves) {
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    temp.from = mp(ff, ss);
    if (boundCheck(ff + 1, ss - 1)) temp.tookAnotherPiece = board[ff + 1][ss - 1];
    //pawns capture diagonally.
    if (ss - 1 >= 0) {
        if (ff + 1 < 8) {
            if (board[ff + 1][ss - 1] != 0 && !pieceIsCurrentPlayersPiece(ff + 1, ss - 1, currentBoardState)) {
                if (ss - 1 == 0) {
                    temp.to = mp(ff + 1, ss - 1), temp.promotedPieceType = Piece::queen, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff + 1, ss - 1), temp.promotedPieceType = Piece::rook, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff + 1, ss - 1), temp.promotedPieceType = Piece::knight, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff + 1, ss - 1), temp.promotedPieceType = Piece::bishop, attemptAddMove(temp, currentBoardState, moves);
                }
                else {
                    temp.to = mp(ff + 1, ss - 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                }
            }
            //en passant
            else if (mp(ff + 1, ss - 1) == currentBoardState->EnPassant()) {
                //we can only en passant if it doesn't put our Piece::king in check.
                if (canEnPassant({ ff, ss }, { ff + 1, ss - 1 }, currentBoardState)) {
                    temp.to = mp(ff + 1, ss - 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
                }
            }
        }
    }
}
void Board::addUpLeftPawnMoves(pii box, BoardState* currentBoardState, std::vector<moveInfo>& moves) {
    //if we're not at the edge of the board
    int ff = box.ff;
    int ss = box.ss;
    uint8_t** board = currentBoardState->getBoard();
    moveInfo temp;
    temp.from = mp(ff, ss);
    if (boundCheck(ff - 1, ss - 1)) temp.tookAnotherPiece = board[ff - 1][ss - 1];
    if (ss - 1 >= 0) {
        if (ff - 1 >= 0) {
            if (board[ff - 1][ss - 1] != 0 && !pieceIsCurrentPlayersPiece(ff - 1, ss - 1, currentBoardState)) {
                if (ss - 1 == 0) {
                    temp.to = mp(ff - 1, ss - 1), temp.promotedPieceType = Piece::queen, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff - 1, ss - 1), temp.promotedPieceType = Piece::rook, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff - 1, ss - 1), temp.promotedPieceType = Piece::knight, attemptAddMove(temp, currentBoardState, moves);
                    temp.to = mp(ff - 1, ss - 1), temp.promotedPieceType = Piece::bishop, attemptAddMove(temp, currentBoardState, moves);
                }
                else
                    temp.to = mp(ff - 1, ss - 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
            }
            //en passant
            else if (mp(ff - 1, ss - 1) == currentBoardState->EnPassant())
                if (canEnPassant(mp( ff, ss ), mp( ff - 1, ss - 1 ), currentBoardState))
                    temp.to = mp(ff - 1, ss - 1), temp.promotedPieceType = 0, attemptAddMove(temp, currentBoardState, moves);
        }
    }
}

void Board::updateThreats(moveInfo lastMove, BoardState * currentBoardState) {
    //see where they moved from first.
    pii kingBox;
    uint8_t **board = currentBoardState->getBoard();
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        if ((board[lastMove.to.ff][lastMove.to.ss]) == (Piece::king | Piece::white)) {
            updateAllThreats(currentBoardState);
            return;
        }

    }
    else {
        kingBox = Piece::bkLoc;
        if (board[lastMove.to.ff][lastMove.to.ss] == (Piece::king | Piece::black)) {
            updateAllThreats( currentBoardState);
            return;
        }
    }

    if (!currentBoardState->whiteTurn) {
        if (board[lastMove.to.ff][lastMove.to.ss] == (Piece::white | Piece::knight)) {
            int xDiff = abs(lastMove.to.ff - kingBox.ff);
            int yDiff = abs(lastMove.to.ss - kingBox.ss);
            int sumOfLeftAndRight = xDiff + yDiff;
            //that means there's a square thats 2-1 between them.
            if (sumOfLeftAndRight == 3 && xDiff != 0 && yDiff != 0) {
                blackThreatened.amountAttacked++;
                blackThreatened.attackedByKnight = true;
                blackThreatened.attackedFrom = mp( lastMove.to.ff, lastMove.to.ss );
            }
        } //IF THE LAST mOVEInfo WAS A Piece::PAWN
        else if (board[lastMove.to.ff][lastMove.to.ss] == (Piece::white | Piece::pawn)) {
            if (lastMove.to.ff == kingBox.ff + 1 && lastMove.to.ss == kingBox.ss + 1) {
                //if it's in one of the diagonal rows
                blackThreatened.amountAttacked++;
                blackThreatened.threatenedInfo |= KingThreatenedInfo::downRightThreatened;
                blackThreatened.attackedInfo |= KingThreatenedInfo::downRightThreatened;
                blackThreatened.attackedFrom = mp( lastMove.to.ff, lastMove.to.ss );
            }
            else if (lastMove.to.ff == kingBox.ff - 1 && lastMove.to.ss == kingBox.ss + 1) {
                blackThreatened.amountAttacked++;
                blackThreatened.threatenedInfo |= KingThreatenedInfo::downLeftThreatened;
                blackThreatened.attackedInfo |= KingThreatenedInfo::downLeftThreatened;
                blackThreatened.attackedFrom = mp( lastMove.to.ff, lastMove.to.ss );
            }
        }
    }
    else {
        if (board[lastMove.to.ff][lastMove.to.ss] == (Piece::black | Piece::knight)) {
            int xDiff = abs(lastMove.to.ff - kingBox.ff);
            int yDiff = abs(lastMove.to.ss - kingBox.ss);
            int sumOfLeftAndRight = xDiff + yDiff;
            //that means there's a square thats 2-1 between them.
            if (sumOfLeftAndRight == 3 && xDiff != 0 && yDiff != 0) {
                whiteThreatened.amountAttacked++;
                whiteThreatened.attackedByKnight = true;
                whiteThreatened.attackedFrom = mp( lastMove.to.ff, lastMove.to.ss );
            }
        }
        else if (board[lastMove.to.ff][lastMove.to.ss] == (Piece::black | Piece::pawn)) {
            if (lastMove.to.ff == kingBox.ff + 1 && lastMove.to.ss == kingBox.ss - 1) {
                //if it's in one of the diagonal rows
                whiteThreatened.amountAttacked++;
                whiteThreatened.threatenedInfo |= KingThreatenedInfo::upRightThreatened;
                whiteThreatened.attackedInfo |= KingThreatenedInfo::upRightThreatened;
                whiteThreatened.attackedFrom = mp( lastMove.to.ff, lastMove.to.ss );
            }
            else if (lastMove.to.ff == kingBox.ff - 1 && lastMove.to.ss == kingBox.ss - 1) {
                whiteThreatened.amountAttacked++;
                whiteThreatened.threatenedInfo |= KingThreatenedInfo::upLeftThreatened;
                whiteThreatened.attackedInfo |= KingThreatenedInfo::upLeftThreatened;
                whiteThreatened.attackedFrom = mp( lastMove.to.ff, lastMove.to.ss );
            }
        }
    }

    //if we castled just update for the Piece::rook. there's a better way to do this but have you seen the rest of my code?
    if (lastMove.kingSideCastle || lastMove.queenSideCastle) {
        updateStraightDownThreats( currentBoardState);
        updateStraightUpThreats( currentBoardState);
        updateStraightLeftThreats( currentBoardState);
        updateStraightRightThreats( currentBoardState);
    }
    if (inSameRow(kingBox, lastMove.to)) {                                  //std::cout << "Last moveInfo was from same row as Piece::king. " << std::endl;
        if (kingBox.ff < lastMove.to.ff)     updateStraightRightThreats( currentBoardState);
        else                               updateStraightLeftThreats( currentBoardState);
    }

    else if (inSameCol(kingBox, lastMove.to)) {
        if (kingBox.ss < lastMove.to.ss)     updateStraightDownThreats( currentBoardState);
        else                               updateStraightUpThreats( currentBoardState);
    }

    else if (inSameDiagonal(kingBox, lastMove.to)) {
        if (kingBox.ff < lastMove.to.ff)     updateUpRightThreats( currentBoardState);
        else                               updateDownLeftThreats( currentBoardState);
    }

    else if (inSameReverseDiagonal(kingBox, lastMove.to)) {
        if (kingBox.ff < lastMove.to.ff)     updateDownRightThreats( currentBoardState);
        else                               updateUpLeftThreats( currentBoardState);
    }

    //now we look at where FROM the new piece moved.
    if (inSameRow(lastMove.from, kingBox) && !inSameRow(lastMove.from, lastMove.to)) {
        if (lastMove.from.ff > kingBox.ff)   updateStraightRightThreats( currentBoardState);
        else                               updateStraightLeftThreats( currentBoardState);
    }

    else if (inSameCol(lastMove.from, kingBox) && !inSameCol(lastMove.from, lastMove.to)) {
        if (lastMove.from.ss > kingBox.ss)   updateStraightDownThreats( currentBoardState);
        else                               updateStraightUpThreats( currentBoardState);
    }

    else if (inSameDiagonal(lastMove.from, kingBox) && !inSameDiagonal(lastMove.from, lastMove.to)) {
        if (lastMove.from.ff > kingBox.ff)   updateUpRightThreats( currentBoardState);
        else                               updateDownLeftThreats( currentBoardState);
    }

    else if (inSameReverseDiagonal(lastMove.from, kingBox) && !inSameReverseDiagonal(lastMove.from, lastMove.to)) {
        if (lastMove.from.ff > kingBox.ff)   updateDownRightThreats( currentBoardState);
        else                               updateUpLeftThreats( currentBoardState);
    }
}
void Board::updateAllThreats( BoardState * currentBoardState) {
    updateStraightDownThreats( currentBoardState);
    updateStraightUpThreats( currentBoardState);
    updateStraightLeftThreats( currentBoardState);
    updateStraightRightThreats( currentBoardState);
    updateUpLeftThreats( currentBoardState);
    updateUpRightThreats( currentBoardState);
    updateDownLeftThreats( currentBoardState);
    updateDownRightThreats( currentBoardState);
}

void Board::updateStraightUpThreats(BoardState * currentBoardState) {
    int defense = 0;
    uint8_t** board = currentBoardState->getBoard();
    pii kingBox;
    bool foundAThreat = false;
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        //up
        for (int ss = kingBox.ss - 1; ss >= 0; --ss) {
            if (board[kingBox.ff][ss] == 0)  continue;
            else if ((board[kingBox.ff][ss] & Piece::white) == Piece::white || ((board[kingBox.ff][ss] & Piece::rook) != Piece::rook && (board[kingBox.ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            else {
                if ((board[kingBox.ff][ss] & Piece::rook) == Piece::rook || (board[kingBox.ff][ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.straightUpBox = { kingBox.ff, ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::straightUpThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightUpThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { kingBox.ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightUpThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            whiteThreatened.threatenedInfo &= ~(1UL << 2);
        foundAThreat = false;
    }
    else {
        kingBox = Piece::bkLoc;
        //up
        for (int ss = kingBox.ss - 1; ss >= 0; --ss) {                                  //We are checking only straight up(North) threats
            if (board[kingBox.ff][ss] == 0)  continue;
            else if ((board[kingBox.ff][ss] & Piece::black) == Piece::black || ((board[kingBox.ff][ss] & Piece::rook) != Piece::rook && (board[kingBox.ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[kingBox.ff][ss] & Piece::rook) == Piece::rook || (board[kingBox.ff][ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.straightUpBox = { kingBox.ff, ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::straightUpThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightUpThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { kingBox.ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightUpThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 2);
    }
}
void Board::updateStraightLeftThreats( BoardState * currentBoardState) {
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    bool foundAThreat = false;
    pii kingBox;
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        for (int ff = kingBox.ff - 1; ff >= 0; --ff) {
            if (board[ff][kingBox.ss] == 0) continue;
            else if ((board[ff][kingBox.ss] & Piece::white) == Piece::white || ((board[ff][kingBox.ss] & Piece::rook) != Piece::rook && (board[ff][kingBox.ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) {
                    break;
                }
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][kingBox.ss] & Piece::rook) == Piece::rook || (board[ff][kingBox.ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.straightLeftBox = { ff, kingBox.ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::straightLeftThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightLeftThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { ff, kingBox.ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightLeftThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat) {
            whiteThreatened.threatenedInfo &= ~(1UL << 0);
        }
        foundAThreat = false;

    }
    else {
        kingBox = Piece::bkLoc;
        for (int ff = kingBox.ff - 1; ff >= 0; --ff) {
            if (board[ff][kingBox.ss] == 0) {
                continue;
            }
            else if ((board[ff][kingBox.ss] & Piece::black) == Piece::black || ((board[ff][kingBox.ss] & Piece::rook) != Piece::rook && (board[ff][kingBox.ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2)  break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][kingBox.ss] & Piece::rook) == Piece::rook || (board[ff][kingBox.ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.straightLeftBox = { ff, kingBox.ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::straightLeftThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightLeftThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { ff, kingBox.ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightLeftThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        //to the left
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 0);
    }
}

void Board::updateStraightDownThreats( BoardState * currentBoardState) {
    bool foundAThreat = false;
    pii kingBox;
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    //down.
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        for (int ss = kingBox.ss + 1; ss < 8; ++ss) {
            if (board[kingBox.ff][ss] == 0) {
                continue;
            }
            else if ((board[kingBox.ff][ss] & Piece::white) == Piece::white || ((board[kingBox.ff][ss] & Piece::rook) != Piece::rook && (board[kingBox.ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[kingBox.ff][ss] & Piece::rook) == Piece::rook || (board[kingBox.ff][ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.straightDownBox = { kingBox.ff, ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::straightDownThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightDownThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { kingBox.ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightDownThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            whiteThreatened.threatenedInfo &= ~(1UL << 6);
        foundAThreat = false;
    }
    else {
        kingBox = Piece::bkLoc;
        for (int ss = kingBox.ss + 1; ss < 8; ++ss) {
            if (board[kingBox.ff][ss] == 0) {
                continue;
            }
            else if ((board[kingBox.ff][ss] & Piece::black) == Piece::black || ((board[kingBox.ff][ss] & Piece::rook) != Piece::rook && (board[kingBox.ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2)  break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[kingBox.ff][ss] & Piece::rook) == Piece::rook || (board[kingBox.ff][ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.straightDownBox = { kingBox.ff, ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::straightDownThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightDownThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { kingBox.ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {

                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightDownThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 6);
        foundAThreat = false;
    }
}
void Board::updateStraightRightThreats( BoardState * currentBoardState) {
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    pii kingBox;
    bool foundAThreat = false;
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        defense = 0;
        for (int ff = kingBox.ff + 1; ff < 8; ++ff) {
            if (board[ff][kingBox.ss] == 0)  continue;
            else if ((board[ff][kingBox.ss] & Piece::white) == Piece::white || ((board[ff][kingBox.ss] & Piece::rook) != Piece::rook && (board[ff][kingBox.ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::black piece.
            else {
                if ((board[ff][kingBox.ss] & Piece::rook) == Piece::rook || (board[ff][kingBox.ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.straightRightBox = { ff, kingBox.ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::straightRightThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightRightThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { ff, kingBox.ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::straightRightThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            //std::cout << "Piece::white threatened before setting it to 0: " << int(whiteThreatened.threatenedInfo) << std::endl;
            whiteThreatened.threatenedInfo &= ~(1UL << 4);
        //std::cout << "Piece::white threatened after setting it to 0: " << int(whiteThreatened.threatenedInfo) << std::endl;
        foundAThreat = false;
    }
    else {
        kingBox = Piece::bkLoc;
        //Piece::black time!!!
        defense = 0;

        for (int ff = kingBox.ff + 1; ff < 8; ++ff) {
            if (board[ff][kingBox.ss] == 0)  continue;
            else if ((board[ff][kingBox.ss] & Piece::black) == Piece::black || ((board[ff][kingBox.ss] & Piece::rook) != Piece::rook && (board[ff][kingBox.ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][kingBox.ss] & Piece::rook) == Piece::rook || (board[ff][kingBox.ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.straightRightBox = { ff, kingBox.ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::straightRightThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightRightThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { ff, kingBox.ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::straightRightThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 4);
    }
}


void Board::updateUpLeftThreats( BoardState * currentBoardState) {
    //std::cout << "Checking for up lefts" << std::endl;
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    bool foundAThreat = false;
    pii kingBox;
    int ff, ss;
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (--ff >= 0 && --ss >= 0) {
            if (board[ff][ss] == 0)  continue;
            else if ((board[ff][ss] & Piece::white) == Piece::white || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.upLeftBox = { ff, ss };

                    if (defense == 0) {

                        whiteThreatened.attackedInfo |= KingThreatenedInfo::upLeftThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::upLeftThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::upLeftThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }

        if (!foundAThreat)
            whiteThreatened.threatenedInfo &= ~(1UL << 1);
        foundAThreat = false;
    }
    else {
        kingBox = Piece::bkLoc;
        //up to the left
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (--ff >= 0 && --ss >= 0) {
            if (board[ff][ss] == 0) continue;
            else if ((board[ff][ss] & Piece::black) == Piece::black || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2)  break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.upLeftBox = { ff, ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::upLeftThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::upLeftThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::upLeftThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat) {
            blackThreatened.threatenedInfo &= ~(1UL << 1);
        }
    }
}


void Board::updateUpRightThreats(BoardState * currentBoardState) {
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    pii kingBox;
    int ff, ss;
    bool foundAThreat = false;
    if (currentBoardState->whiteTurn) {
        kingBox = Piece::wkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (++ff < 8 && --ss >= 0) {
            if (board[ff][ss] == 0)  continue;
            else if ((board[ff][ss] & Piece::white) == Piece::white || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::black
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.upRightBox = { ff, ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::upRightThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::upRightThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::upRightThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            whiteThreatened.threatenedInfo &= ~(1UL << 3);
        foundAThreat = false;
    }
    else {
        kingBox = Piece::bkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (++ff < 8 && --ss >= 0) {
            if (board[ff][ss] == 0)  continue;
            else if ((board[ff][ss] & Piece::black) == Piece::black || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2)  break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.upRightBox = { ff, ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::upRightThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::upRightThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::upRightThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 3);
    }
}

void Board::updateDownLeftThreats(BoardState * currentBoardState) {
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    pii kingBox;
    bool foundAThreat = false;
    int ff, ss;
    if (!currentBoardState->whiteTurn) {
        //down to the left
        kingBox = Piece::bkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (--ff >= 0 && ++ss < 8) {
            if (board[ff][ss] == 0) continue;
            else if ((board[ff][ss] & Piece::black) == Piece::black || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.downLeftBox = { ff, ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::downLeftThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::downLeftThreatened;

                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {

                        blackThreatened.threatenedInfo |= KingThreatenedInfo::downLeftThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 7);
        foundAThreat = false;
    }
    else {
        //down to the left
        kingBox = Piece::wkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (--ff >= 0 && ++ss < 8) {
            if (board[ff][ss] == 0) continue;
            else if ((board[ff][ss] & Piece::white) == Piece::white || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.downLeftBox = { ff, ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::downLeftThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::downLeftThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::downLeftThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }
        }
        if (!foundAThreat)
            whiteThreatened.threatenedInfo &= ~(1UL << 7);
    }
}

void Board::updateDownRightThreats( BoardState * currentBoardState) {
    uint8_t** board = currentBoardState->getBoard();
    int defense = 0;
    pii kingBox;
    bool foundAThreat = false;
    int ff, ss;
    if (currentBoardState->whiteTurn) {
        //down to the left
        kingBox = Piece::wkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (++ff < 8 && ++ss < 8) {
            if (board[ff][ss] == 0) continue;
            else if ((board[ff][ss] & Piece::white) == Piece::white || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            //it's a Piece::white piece.
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    whiteThreatened.downRightBox = { ff, ss };
                    if (defense == 0) {
                        whiteThreatened.attackedInfo |= KingThreatenedInfo::downRightThreatened;
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::downRightThreatened;
                        whiteThreatened.amountAttacked++;
                        whiteThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        whiteThreatened.threatenedInfo |= KingThreatenedInfo::downRightThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }

        }
        if (!foundAThreat)
            whiteThreatened.threatenedInfo &= ~(1UL << 5);
        foundAThreat = false;
    }

    else {
        kingBox = Piece::bkLoc;
        ff = kingBox.ff;
        ss = kingBox.ss;
        while (++ff < 8 && ++ss < 8) {
            if (board[ff][ss] == 0) continue;
            else if ((board[ff][ss] & Piece::black) == Piece::black || ((board[ff][ss] & Piece::bishop) != Piece::bishop && (board[ff][ss] & Piece::queen) != Piece::queen)) {
                defense++;
                if (defense == 2) break;
            }
            else {
                if ((board[ff][ss] & Piece::bishop) == Piece::bishop || (board[ff][ss] & Piece::queen) == Piece::queen) {
                    blackThreatened.downRightBox = { ff, ss };
                    if (defense == 0) {
                        blackThreatened.attackedInfo |= KingThreatenedInfo::downRightThreatened;
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::downRightThreatened;
                        blackThreatened.amountAttacked++;
                        blackThreatened.attackedFrom = { ff, ss };
                        foundAThreat = true;
                        break;
                    }
                    else {
                        blackThreatened.threatenedInfo |= KingThreatenedInfo::downRightThreatened;
                        foundAThreat = true;
                        break;
                    }
                }
            }

        }
        if (!foundAThreat)
            blackThreatened.threatenedInfo &= ~(1UL << 5);
    }
}



void Board::initializeKingsThreatened(BoardState * currentBoardState) {
    whiteThreatened.threatenedInfo = 0;
    blackThreatened.threatenedInfo = 0;
    currentBoardState->whiteTurn = true;
    updateStraightUpThreats( currentBoardState);                //handle the color of the player's piece
    updateStraightDownThreats( currentBoardState);
    updateStraightLeftThreats( currentBoardState);
    updateStraightRightThreats( currentBoardState);
    updateDownLeftThreats( currentBoardState);
    updateUpLeftThreats( currentBoardState);
    updateDownRightThreats( currentBoardState);
    updateUpRightThreats( currentBoardState);

    currentBoardState->whiteTurn = false;
    updateStraightUpThreats( currentBoardState);
    updateStraightDownThreats( currentBoardState);
    updateStraightLeftThreats(currentBoardState);
    updateStraightRightThreats( currentBoardState);
    updateDownLeftThreats( currentBoardState);
    updateUpLeftThreats( currentBoardState);
    updateDownRightThreats( currentBoardState);
    updateUpRightThreats( currentBoardState);

    currentBoardState->whiteTurn = true;
}

bool Board::kingAttacked(BoardState* currentBoardState)
{
    if (currentBoardState->whiteTurn)
        return (whiteThreatened.amountAttacked);
    else
        return (blackThreatened.amountAttacked);
}

void Board::togglePromotionOptions() { waitingForPromotionChoice ^= 1; }

bool Board::squareAttacked(pii p, BoardState * currentBoardState)
{
    int ff = p.ff, ss = p.ss; // ff is col(file), ss is row(rank)
    uint8_t** board = currentBoardState->getBoard();

    // pawnAttacks
    if (currentBoardState->whiteTurn) // Piece::white is always at bottom
    {
        if (ss - 1 >= 0)
        {
            if (ff - 1 >= 0)
            {
                if (board[ff - 1][ss - 1] == (Piece::black | Piece::pawn))
                    return true;
            }
            if (ff + 1 < 8)
            {
                if (board[ff + 1][ss - 1] == (Piece::black | Piece::pawn))
                    return true;
            }
        }
    }
    else
    {
        if (ss + 1 < 8)
        {
            if (ff - 1 >= 0)
            {
                if ((board[ff - 1][ss + 1]) == (Piece::white | Piece::pawn))
                    return true;
            }
            if (ff + 1 < 8)
            {
                if (board[ff + 1][ss + 1] == (Piece::white | Piece::pawn))
                    return true;
            }
        }
    }

    uint8_t enemyColor = Piece::white, myColor = Piece::black;
    if (currentBoardState->whiteTurn)
        swap(myColor, enemyColor);

    // going right (rookAttacks & queenAttacks)
    if (ff + 1 < 8 && board[ff + 1][ss] == (enemyColor | Piece::king))
        return true;

    for (int currX = ff + 1; currX < 8; ++currX)
    {
        if (board[currX][ss] == 0)
            continue;
        if (board[currX][ss] == (enemyColor | Piece::queen) || board[currX][ss] == (enemyColor | Piece::rook))
            return true;
        else if (board[currX][ss] != (myColor | Piece::king)) // check for indirect threat
            break;
    }

    //going left (rookAttacks & queenAttacks)
    if (ff - 1 >= 0 && board[ff - 1][ss] == (enemyColor | Piece::king))
        return true;

    for (int currX = ff - 1; currX >= 0; --currX)
    {
        if (board[currX][ss] == 0)
            continue;
        if (board[currX][ss] == (enemyColor | Piece::queen) || board[currX][ss] == (enemyColor | Piece::rook))
            return true;
        else if (board[currX][ss] != (myColor | Piece::king)) // check for indirect threat
            break;
    }

    //going up (rookAttacks & queenAttacks)
    if (ss - 1 >= 0 && board[ff][ss - 1] == (enemyColor | Piece::king))
        return true;

    for (int currY = ss - 1; currY >= 0; --currY)
    {
        if (board[ff][currY] == 0)
            continue;
        if (board[ff][currY] == (enemyColor | Piece::queen) || board[ff][currY] == (enemyColor | Piece::rook))
            return true;
        else if (board[ff][currY] != (myColor | Piece::king)) // check for indirect threat
            break;
    }

    //going down (rookAttacks & queenAttacks)
    if (ss + 1 < 8 && board[ff][ss + 1] == (enemyColor | Piece::king))
        return true;

    for (int currY = ss + 1; currY < 8; ++currY)
    {
        if (board[ff][currY] == 0)
            continue;
        if (board[ff][currY] == (enemyColor | Piece::queen) || board[ff][currY] == (enemyColor | Piece::rook))
            return true;
        else if (board[ff][currY] != (myColor | Piece::king)) // check for indirect threat
            break;
    }

    //going down-right (bishopAttacks & queenAttacks)
    int inc = 1; // increment
    if (ff + inc < 8 && ss + inc < 8 && board[ff + inc][ss + inc] == (enemyColor | Piece::king))
        return true;

    while (ff + inc < 8 && ss + inc < 8)
    {
        if (board[ff + inc][ss + inc] == 0)
            ;
        else if (board[ff + inc][ss + inc] == (enemyColor | Piece::queen) || board[ff + inc][ss + inc] == (enemyColor | Piece::bishop))
            return true;
        else if (board[ff + inc][ss + inc] != (myColor | Piece::king)) // check for indirect threat
            break;
        ++inc;
    }

    //going down-left (bishopAttacks & queenAttacks)
    inc = 1;
    if (ff - inc >= 0 && ss + inc < 8 && board[ff - inc][ss + inc] == (enemyColor | Piece::king))
        return true;

    while (ff - inc >= 0 && ss + inc < 8)
    {
        if (board[ff - inc][ss + inc] == 0)
            ;
        else if (board[ff - inc][ss + inc] == (enemyColor | Piece::queen) || board[ff - inc][ss + inc] == (enemyColor | Piece::bishop))
            return true;
        else if (board[ff - inc][ss + inc] != (myColor | Piece::king)) // check for indirect threat
            break;
        ++inc;
    }

    //going up-left (bishopAttacks & queenAttacks)
    inc = 1;
    if (ff - inc >= 0 && ss - inc >= 0 && board[ff - inc][ss - inc] == (enemyColor | Piece::king))
        return true;

    while (ff - inc >= 0 && ss - inc >= 0)
    {
        if (board[ff - inc][ss - inc] == 0)
            ;
        else if (board[ff - inc][ss - inc] == (enemyColor | Piece::queen) || board[ff - inc][ss - inc] == (enemyColor | Piece::bishop))
            return true;
        else if (board[ff - inc][ss - inc] != (myColor | Piece::king)) // check for indirect threat
            break;
        ++inc;
    }

    //going up-right (bishopAttacks & queenAttacks)
    inc = 1;
    if (ff + inc < 8 && ss - inc >= 0 && board[ff + inc][ss - inc] == (enemyColor | Piece::king))
        return true;

    while (ff + inc < 8 && ss - inc >= 0)
    {
        if (board[ff + inc][ss - inc] == 0)
            ;
        else if (board[ff + inc][ss - inc] == (enemyColor | Piece::queen) || board[ff + inc][ss - inc] == (enemyColor | Piece::bishop))
            return true;
        else if (board[ff + inc][ss - inc] != (myColor | Piece::king)) // check for indirect threat
            break;
        ++inc;
    }

    // knightAttacks
    int dx[] = {2, 2, -2, -2, 1, 1, -1, -1}, dy[] = {1, -1, 1, -1, 2, -2, 2, -2};
    for (int i = 0; i < 8; i++)
    {
        int nx = ff + dx[i], ny = ss + dy[i];
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8 && board[nx][ny] == (enemyColor | Piece::knight))
            return true;
    }

    return false;
}

bool Board::kingInCheck(BoardState *currentBoardState)
{
    pii KingLoc;
    if (currentBoardState->whiteTurn)
        KingLoc = Piece::wkLoc;
    else
        KingLoc = Piece::bkLoc;
    return squareAttacked(KingLoc, currentBoardState);
}

int Board::isGameOver(BoardState* currentBoardState)
{
    if (legalMoves.empty())
    {
        whiteWinner = (currentBoardState->whiteTurn) ^ 1;
        return 1;
    }
    else
        return 0;
}

void Board::initializePieceLocations(BoardState* currentBoardState)
{
    for (int x = 0; x < 8; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            uint8_t currentPiece = (currentBoardState->getBoard())[x][y];
            if (currentPiece != 0)
            {
                if (currentPiece == (Piece::white | Piece::king))
                    Piece::wkLoc = mp(x, y);
                else if (currentPiece == (Piece::white | Piece::queen))
                    Piece::wqLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::white | Piece::rook))
                    Piece::wrLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::white | Piece::bishop))
                    Piece::wbLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::white | Piece::knight))
                    Piece::wnLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::white | Piece::pawn))
                    Piece::wpLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::black | Piece::king))
                    Piece::bkLoc = mp(x, y);
                else if (currentPiece == (Piece::black | Piece::queen))
                    Piece::bqLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::black | Piece::rook))
                    Piece::brLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::black | Piece::bishop))
                    Piece::bbLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::black | Piece::knight))
                    Piece::bnLoc.insert(mp(x, y));
                else if (currentPiece == (Piece::black | Piece::pawn))
                    Piece::bpLoc.insert(mp(x, y));
            }
        }
    }
}

void Board::switchTurns(BoardState* currentBoardState) { (currentBoardState->whiteTurn) ^= 1; }

void Board::nextTurn(BoardState* currentBoardState)
{
    legalMoves = calculateLegalMoves(currentBoardState);
    if (isGameOver(currentBoardState))
        gameOver = true;
    updateHighlightKingBox(currentBoardState);
}

void Board::tryPickingPromotionPiece(int x, int y, BoardState* currentBoardState)
{
    moveInfo M = promotionMove;
    if (y == 3 || y == 4)
    {
        switch (x / 2)
        {
        case 0:
            M.promotedPieceType = Piece::queen;
            makeMove(M, currentBoardState);
            break;
        case 1:
            M.promotedPieceType = Piece::rook;
            makeMove(M, currentBoardState);
            break;
        case 2:
            M.promotedPieceType = Piece::bishop;
            makeMove(M, currentBoardState);
            break;
        case 3:
            M.promotedPieceType = Piece::knight;
            makeMove(M, currentBoardState);
            break;
        }
        waitingForPromotionChoice = false;
        switchTurns(currentBoardState);
        nextTurn(currentBoardState);
    }
}


void Board::renderBox(pii box, SDL_Color color) {

    SDL_Rect highlightRect;
    highlightRect.w = 100, highlightRect.h = 100;
    SDL_Color drawColor = color;
    SDL_SetRenderDrawColor(Window::rend, color.r, color.g, color.b, color.a);

    highlightRect.x = box.ff * 100, highlightRect.y = box.ss * 100;
    SDL_RenderFillRect(Window::rend, &highlightRect);
}

void Board::renderPieces(BoardState * currentBoardState)
{
    display_piece(Window::rend, Piece::wk, Piece::wkLoc, Piece::king);
    display_piece(Window::rend, Piece::bk, Piece::bkLoc, Piece::king);

    for (auto each : Piece::wbLoc)
        display_piece(Window::rend, Piece::wb, each, Piece::bishop);

    for (auto each : Piece::wqLoc)
        display_piece(Window::rend, Piece::wq, each, Piece::queen);

    for (auto each : Piece::wpLoc)
        display_piece(Window::rend, Piece::wp, each, Piece::pawn);

    for (auto each : Piece::wrLoc)
        display_piece(Window::rend, Piece::wr, each, Piece::rook);

    for (auto each : Piece::wnLoc)
        display_piece(Window::rend, Piece::wn, each, Piece::knight);

    for (auto each : Piece::bbLoc)
        display_piece(Window::rend, Piece::bb, each, Piece::bishop);

    for (auto each : Piece::bqLoc)
        display_piece(Window::rend, Piece::bq, each, Piece::queen);

    for (auto each : Piece::bpLoc)
        display_piece(Window::rend, Piece::bp, each, Piece::pawn);

    for (auto each : Piece::brLoc)
        display_piece(Window::rend, Piece::br, each, Piece::rook);

    for (auto each : Piece::bnLoc)
        display_piece(Window::rend, Piece::bn, each, Piece::knight);
}

// void Board::renderAttackedSquares(BoardState *currentBoardState) {
//     for (int x = 0; x < 8; x++) {
//         for (int y = 0; y < 8; y++) {
//             if (squareAttacked(mp(x, y), currentBoardState)) {
//                 renderBox(mp(x, y), DANGER_COLOR);
//             }
//         }
//     }
// }

void Board::renderHighlightMoves()
{
    SDL_Surface* spot_img = IMG_Load("resources/spot.png");
    SDL_Texture* spot_tex = SDL_CreateTextureFromSurface(Window::rend, spot_img);
    SDL_FreeSurface(spot_img);

    SDL_Rect src, dest;
    SDL_QueryTexture(spot_tex, NULL, NULL, &src.w, &src.h);
    src.x = 0, src.y = 0, dest.w = dest.h = 100;

    for (int i = 0; i < highlightBoxes.size(); i++)
    {
        dest.x = 100 * highlightBoxes[i].first, dest.y = 100 * highlightBoxes[i].second, 
        SDL_RenderCopy(Window::rend, spot_tex, &src, &dest);
    }

    SDL_DestroyTexture(spot_tex);
}

void Board::renderPreviousMove()
{
    renderBox(Move::prevMove.from, LAST_MOVE_COLOR);
    renderBox(Move::prevMove.to, LAST_MOVE_COLOR);
}
