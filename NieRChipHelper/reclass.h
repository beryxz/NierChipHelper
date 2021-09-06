#pragma once

#include <Windows.h>

// Created with ReClass.NET 1.2 by KN4CK3R

class Player
{
public:
	char pad_0000[2136]; //0x0000
	uint32_t health; //0x0858
	uint32_t MaxHealth; //0x085C
	char pad_0860[4140]; //0x0860
}; //Size: 0x188C
static_assert(sizeof(Player) == 0x188C);

class ChipItem
{
public:
	int32_t baseCode; //0x0000
	int32_t baseId; //0x0004
	int32_t type; //0x0008
	int32_t level; //0x000C
	int32_t weight; //0x0010
	int32_t offsetA; //0x0014
	int32_t offsetB; //0x0018
	int32_t offsetC; //0x001C
	char pad_0020[12]; //0x0020
	int32_t alwaysZero; //0x002C

	void clear();
}; //Size: 0x0030
static_assert(sizeof(ChipItem) == 0x30);

#pragma pack(push,1)
class Chips
{
public:
	class Inventory* pInventory; //0x0000
	int32_t system; //0x0008
	int32_t attack; //0x000C
	int32_t defense; //0x0010
	int32_t support; //0x0014
	int32_t hacking; //0x0018
	int32_t totChips; //0x001C
	int32_t curSlotUsedStorage; //0x0020
}; //Size: 0x0024
static_assert(sizeof(Chips) == 0x24);
#pragma pack(pop)

class Item
{
public:
	char pad_0000[12]; //0x0000
}; //Size: 0x000C
static_assert(sizeof(Item) == 0xC);

class Inventory
{
public:
	class Item items[512]; //0x0000
	char pad_1800[1872]; //0x1800
	class ChipItem chips[300]; //0x1F50
}; //Size: 0x5790
static_assert(sizeof(Inventory) == 0x5790);
