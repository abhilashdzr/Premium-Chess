#include "Piece.h"
#include "Window.h"

SDL_Texture *Piece::wk, *Piece::wq, *Piece::wr, *Piece::wb, *Piece::wn, *Piece::wp;
SDL_Texture *Piece::bk, *Piece::bq, *Piece::br, *Piece::bb, *Piece::bn, *Piece::bp;

const uint8_t Piece::king, Piece::queen, Piece::rook, Piece::bishop, Piece::knight, Piece::pawn;
const uint8_t Piece::white, Piece::black;

pii Piece::wkLoc, Piece::bkLoc;
set<pii> Piece::wqLoc, Piece::wrLoc, Piece::wbLoc, Piece::wnLoc, Piece::wpLoc;
set<pii> Piece::bqLoc, Piece::brLoc, Piece::bbLoc, Piece::bnLoc, Piece::bpLoc;

void Piece::init()
{
    loadPieces();
    wkLoc = mp(-1, -1);
    bkLoc = mp(-1, -1);
}

void Piece::clear()
{
    wqLoc.clear();
    wrLoc.clear();
    wbLoc.clear();
    wnLoc.clear();
    wpLoc.clear();
    bqLoc.clear();
    brLoc.clear();
    bbLoc.clear();
    bnLoc.clear();
    bpLoc.clear();
}

void Piece::loadPieces()
{
    SDL_Surface* wk_img = IMG_Load("resources/wk.png");
    wk = SDL_CreateTextureFromSurface(Window::rend, wk_img);
    SDL_FreeSurface(wk_img);

    SDL_Surface* wq_img = IMG_Load("resources/wq.png");
    wq = SDL_CreateTextureFromSurface(Window::rend, wq_img);
    SDL_FreeSurface(wq_img);

    SDL_Surface* wr_img = IMG_Load("resources/wr.png");
    wr = SDL_CreateTextureFromSurface(Window::rend, wr_img);
    SDL_FreeSurface(wr_img);

    SDL_Surface* wb_img = IMG_Load("resources/wb.png");
    wb = SDL_CreateTextureFromSurface(Window::rend, wb_img);
    SDL_FreeSurface(wb_img);

    SDL_Surface* wn_img = IMG_Load("resources/wn.png");
    wn = SDL_CreateTextureFromSurface(Window::rend, wn_img);
    SDL_FreeSurface(wn_img);

    SDL_Surface* wp_img = IMG_Load("resources/wp.png");
    wp = SDL_CreateTextureFromSurface(Window::rend, wp_img);
    SDL_FreeSurface(wp_img);

    SDL_Surface* bk_img = IMG_Load("resources/bk.png");
    bk = SDL_CreateTextureFromSurface(Window::rend, bk_img);
    SDL_FreeSurface(bk_img);

    SDL_Surface* bq_img = IMG_Load("resources/bq.png");
    bq = SDL_CreateTextureFromSurface(Window::rend, bq_img);
    SDL_FreeSurface(bq_img);

    SDL_Surface* br_img = IMG_Load("resources/br.png");
    br = SDL_CreateTextureFromSurface(Window::rend, br_img);
    SDL_FreeSurface(br_img);

    SDL_Surface* bb_img = IMG_Load("resources/bb.png");
    bb = SDL_CreateTextureFromSurface(Window::rend, bb_img);
    SDL_FreeSurface(bb_img);

    SDL_Surface* bn_img = IMG_Load("resources/bn.png");
    bn = SDL_CreateTextureFromSurface(Window::rend, bn_img);
    SDL_FreeSurface(bn_img);

    SDL_Surface* bp_img = IMG_Load("resources/bp.png");
    bp = SDL_CreateTextureFromSurface(Window::rend, bp_img);
    SDL_FreeSurface(bp_img);
}

void Piece::destroyPieces()
{
    SDL_DestroyTexture(wk);
    SDL_DestroyTexture(wq);
    SDL_DestroyTexture(wr);
    SDL_DestroyTexture(wb);
    SDL_DestroyTexture(wn);
    SDL_DestroyTexture(wp);
    SDL_DestroyTexture(bk);
    SDL_DestroyTexture(bq);
    SDL_DestroyTexture(br);
    SDL_DestroyTexture(bb);
    SDL_DestroyTexture(bn);
    SDL_DestroyTexture(bp);
}

void Piece::removePiece(uint8_t piece, pii Loc)
{
    if (piece & white)
    {
        if (piece & king)
            wkLoc = mp(-1, -1);
        else if (piece & queen)
            wqLoc.erase(Loc);
        else if (piece & rook)
            wrLoc.erase(Loc);
        else if (piece & bishop)
            wbLoc.erase(Loc);
        else if (piece & knight)
            wnLoc.erase(Loc);
        else if (piece & pawn)
            wpLoc.erase(Loc);
    }
    else
    {
        if (piece & king)
            bkLoc = mp(-1, -1);
        else if (piece & queen)
            bqLoc.erase(Loc);
        else if (piece & rook)
            brLoc.erase(Loc);
        else if (piece & bishop)
            bbLoc.erase(Loc);
        else if (piece & knight)
            bnLoc.erase(Loc);
        else if (piece & pawn)
            bpLoc.erase(Loc);
    }
}

void Piece::addPiece(uint8_t piece, pii Loc)
{
    if (piece & white)
    {
        if (piece & king)
            wkLoc = Loc;
        else if (piece & queen)
            wqLoc.insert(Loc);
        else if (piece & rook)
            wrLoc.insert(Loc);
        else if (piece & bishop)
            wbLoc.insert(Loc);
        else if (piece & knight)
            wnLoc.insert(Loc);
        else if (piece & pawn)
            wpLoc.insert(Loc);
    }
    else
    {
        if (piece & king)
            bkLoc = Loc;
        else if (piece & queen)
            bqLoc.insert(Loc);
        else if (piece & rook)
            brLoc.insert(Loc);
        else if (piece & bishop)
            bbLoc.insert(Loc);
        else if (piece & knight)
            bnLoc.insert(Loc);
        else if (piece & pawn)
            bpLoc.insert(Loc);
    }
}

void Piece::updatePiece(uint8_t piece, pii oldLoc, pii newLoc)
{
    Piece::removePiece(piece, oldLoc);
    Piece::addPiece(piece, newLoc);
}