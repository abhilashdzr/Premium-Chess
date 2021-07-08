#include "Move.h"

int Move::fiftyMovesCounter;
list<moveInfo> Move::lastThree;
moveInfo Move::prevMove;

void Move::init()
{
	fiftyMovesCounter = 0;
	prevMove.from = prevMove.to = mp(-1, -1);
}

void Move::setprevMove(moveInfo m)
{
	prevMove = m;
	if (prevMove.pawnAdvanced || prevMove.tookAnotherPiece)
		fiftyMovesCounter = 0;
	else
		fiftyMovesCounter++;
	lastThree.push_back(prevMove);
	if (lastThree.size() > 10)
		lastThree.pop_front();
}

bool Move::isDraw()
{
	if (fiftyMovesCounter == 100) // total 100 moves noPieceTaking & noPawnAdvanced
		return true;
	if (lastThree.size() < 10)
		return false;
	int pos = 0;
	bool takings = false;
	pii arr[10];
	for (auto it = lastThree.begin(); it != lastThree.end(); it++, pos++)
		takings |= (it->tookAnotherPiece), arr[pos] = (it->to);
	if (takings)
		return false;
	bool Rep = (arr[0] == arr[4]) & (arr[4] == arr[8]) & (arr[2] == arr[6]);
	Rep &= (arr[1] == arr[5]) & (arr[3] == arr[7]);
	if (Rep) // last 10 moveInfo.to -> {w1, b1, w2, b2, w1, b1, w2, b2, w1, b1} & noPieceTaking
		return true;
}
