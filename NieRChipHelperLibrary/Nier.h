#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>

#include "imgui.h"

#include "Mem.h"
#include "reclass.h"


namespace Chip {
	enum Status_ {
		Status_None		= 1 << 0,
		Status_Trash	= 1 << 1,
		Status_New		= 1 << 2
	};

	enum class Category {
		SYSTEM,		// #877E66 - 0.906 0.882 0.780
		ATTACK,		// #B7997E - 0.718 0.600 0.494
		DEFENSE,	// #BDAF8B - 0.741 0.686 0.545
		SUPPORT,	// #E3D9A7 - 0.890 0.851 0.655
		HACKING		// #E7E1C7 - 0.529 0.494 0.400
	};

	struct Type {
		int32_t type;
		std::string name;
		Chip::Category category;
	};

	struct Level {
		int32_t level;
		int32_t maxWorthRank;
		int32_t diamondRank;
	};
}


class Nier
{
public:

	Nier();
	~Nier();

	struct ChipWrapper {
		ChipItem* item;
		Chip::Type type;
		DWORD status;
		Chip::Level level;
	};

	// The game internal logic support up to 300 chips.
	// But in the latest game versions the actual max has been capped to 200.
	// Therefore when reading to memory there are 300 indexes to check,
	// but only 200 of them can be used without chaning some internal functions.
	static const DWORD dMaxChipCount = 200;
	static const DWORD dMaxStorableChipCount = 300;

	static Chips* pChips; // pointer to chips counters and inventory location
	static DWORD dChipsCount;

	// Status variables
	static DWORD* isWorldLoaded;
	static DWORD* isInAMenu;

	static uintptr_t moduleBaseAddress;

	// Used for rendering Chips Table
	static BOOL isChipsListDirty;
	static int curShownStatusIndex;
	static std::array<ChipWrapper, dMaxStorableChipCount> chipsList; // Local copy of pointers to chips, used for sorting the list

	// In-game function pointers
	static void (*updateChipsCount)(void* pChipsBaseAddr);

	static const std::unordered_map<int, Chip::Level> chipsLevelsTable;
	static const std::unordered_map<int, Chip::Type> chipsTypeTable;

	static void updateChipsListAndCount();
	static void removeNewStatusFromChips();

	static void toggleAutoDelete();
	static BOOL isAutoDeleteActive();

	// OSD (On screen display)
	static ImFont* osdFont;
	static const float osdFontSize;
	static BOOL isOSDActive();
	static void toggleOSD();

	static void clearForExit();

private:

	// ==== AutoDelete hook ====
	static BOOL bAutoDelete;
	static Mem::hook_t* autoDeleteHook;

	// ==== Used for rendering chip tables ====
	/*
		Struct to track a single position of the in-game chip array, between changes.
		isEmpty and isNew are used to set "Status_New"
		baseId is used when chips are fused to check if a chip was overwritten with a new one. If so, sets "Status_New"
	*/
	struct ChipsListIndex { BOOL isEmpty; BOOL isNew; int32_t baseId; };
	/*
		Chips in memory are saved in an array that isn't sorted.
		New chips are added to the first position that's empty.
		Deleted chips are just set to -1, nothing gets moved around.
		Therefore, this array is used to keep track of empty spots, to be able to mark new chips between updates
	*/
	static std::array<ChipsListIndex, dMaxStorableChipCount> chipsListIndex;

	// ==== On-Screen Display ====
	static BOOL bOSD;

};
