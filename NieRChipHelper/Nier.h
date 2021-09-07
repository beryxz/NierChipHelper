#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>
#include <mutex>

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

	/*
		Struct to track a single position of the in-game chip array, between changes.
		isEmpty and isNew are used to set "Status_New"
		baseId is used when chips are fused to check if a chip was overwritten with a new one. If so, sets "Status_New"
	*/
	struct ChipsListIndex {
		BOOL isEmpty;
		BOOL isNew;
		int32_t baseId;
	};
	/*
		Wrapper around the Reclass "ChipItem" structure.
		This wrapper adds useful metadata to improve its use.
	*/
	struct ChipWrapper {
		ChipItem* item;
		const Chip::Type* type;
		const Chip::Level* level;
		DWORD status;
		ChipsListIndex* chipsListIndex;
	};

	// The game internal logic support up to 300 chips.
	// But in the latest game versions the actual max has been capped to 200.
	// Therefore when reading to memory there are 300 indexes to check,
	// but only 200 of them can be used without chaning some internal functions.
	static const DWORD dMaxChipCount = 200;
	static const DWORD dMaxStorableChipCount = 300;

	static Chips* pChips; // pointer to chips counters and inventory location
	static DWORD dChipsCount;

	// ==== Status checks ====
	static BOOL isInAMenu();
	static BOOL isWorldLoaded();

	static uintptr_t moduleBaseAddress;

	// ==== Used for rendering Chips Table ====
	/*
		Chips in memory are saved in an array that isn't sorted.
		New chips are added to the first position that's empty.
		Deleted chips are just set to -1, nothing gets moved around.
		Therefore, this array is used to keep track of empty spots, to be able to mark new chips between updates
	*/
	static std::array<ChipsListIndex, dMaxStorableChipCount> chipsListIndexes;
	static BOOL isChipsListDirty;
	static int curShownStatusIndex; // Index of the filter type applied to the chips table
	static std::array<ChipWrapper, dMaxStorableChipCount> chipsList; // Local copy of pointers to chips, used for sorting the list
	/*
		A mutex is required since the chipsList array could be read/modified by both: the Dear Imgui render function (called by the DirectX thread);
		and in the main loop inside mainFunction.
	*/
	static std::mutex mtxChipsList;

	// ==== In-game function pointers ====
	static void (*updateChipsCount)(void* pChipsBaseAddr);

	static const std::unordered_map<int, Chip::Level> chipsLevelsTable;
	static const std::unordered_map<int, Chip::Type> chipsTypeTable;

	static void updateChipsListAndCount();
	static void removeNewStatusFromChip(ChipWrapper* chip);
	static void removeNewStatusFromChips();
	static void resetChipsList();

	static void toggleAutoDelete();
	static BOOL isAutoDeleteActive();

	// ==== OSD (On screen display) ====
	static ImFont* osdFont;
	static const float osdFontSize;
	static BOOL isOSDActive();
	static void toggleOSD();

	static void clearForExit();

private:

	// ==== AutoDelete hook ====
	static BOOL bAutoDelete;
	static Mem::hook_t* autoDeleteHook;

	// ==== On-Screen Display ====
	static BOOL bOSD;

	// ==== Status variables ====
	static DWORD* pdIsInAMenu;
	static DWORD* pdIsWorldLoaded;

};
