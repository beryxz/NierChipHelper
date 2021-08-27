#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>

#include "reclass.h"

class Nier
{
public:

	static Chips* pChips; // pointer to chips counters and inventory location
	static DWORD dChipsCount;
	static std::array<Chip*, 300> chipsList; // Local copy of pointers to chips, used for sorting the list

	enum ChipCategory {
		CHIP_SYSTEM,  // #877E66 - 0.906 0.882 0.780
		CHIP_ATTACK,  // #B7997E - 0.718 0.600 0.494
		CHIP_DEFENSE, // #BDAF8B - 0.741 0.686 0.545
		CHIP_SUPPORT, // #E3D9A7 - 0.890 0.851 0.655
		CHIP_HACKING  // #E7E1C7 - 0.529 0.494 0.400
	};

	struct ChipType {
		int32_t type;
		std::string name;
		ChipCategory category;
	};

	struct ChipLevel {
		int32_t level;
		int32_t maxWorthRank;
		int32_t diamondRank;
	};

	static void updateChipsListAndCount();

private:

};

//TODO: Remove from global scope
// <chipLevel, maxWorthRank>
extern std::unordered_map<int, Nier::ChipLevel> chipsLevelsTable;

//TODO: Remove from global scope
// <chipType, chipName>
extern std::unordered_map<int, Nier::ChipType> chipsTypeTable;
