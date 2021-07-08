#pragma once

#include <utility> // pair
#include <stdint.h> // uint8_t
using namespace std;

struct KingThreatenedInfo {
	uint8_t threatenedInfo;
	uint8_t attackedInfo;

	pair<int, int> straightLeftBox, upLeftBox, straightUpBox, upRightBox;
	pair<int, int> straightRightBox, downRightBox, straightDownBox, downLeftBox;
	pair<int, int> attackedFrom;

	int amountAttacked;
	bool attackedByKnight;

	const static uint8_t straightLeftThreatened = (1 << 0);
	const static uint8_t upLeftThreatened = (1 << 1);
	const static uint8_t straightUpThreatened = (1 << 2);
	const static uint8_t upRightThreatened = (1 << 3);
	const static uint8_t straightRightThreatened = (1 << 4);
	const static uint8_t downRightThreatened = (1 << 5);
	const static uint8_t straightDownThreatened = (1 << 6);
	const static uint8_t downLeftThreatened = (1 << 7);
};
