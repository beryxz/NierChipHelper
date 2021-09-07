#include "pch.h"
#include "Nier.h"

extern "C" LPVOID AUTODELETE();
extern "C" LPVOID AUTODELETE_SKIP;
LPVOID AUTODELETE_SKIP = (LPVOID)((uintptr_t)GetModuleHandle(L"NieRAutomata.exe") + 0x7CB47E);
extern "C" LPVOID AUTODELETE_JUMPBACK;
LPVOID AUTODELETE_JUMPBACK = (LPVOID)((uintptr_t)GetModuleHandle(L"NieRAutomata.exe") + 0x7CB388);

Chips* Nier::pChips = nullptr; // pointer to chips counters and inventory location
DWORD Nier::dChipsCount = 0; // This counter is auto updated to reflect the in-game one. It shouldn't be modified.

BOOL Nier::isChipsListDirty = FALSE;
int Nier::curShownStatusIndex = 0; // Index of the filter type applied to the chips table
/*
Chips in memory are saved in an array that isn't sorted.
New chips are added to the first position that's empty.
Deleted chips are just set to -1, nothing gets moved around.
Therefore, this array is used to keep track of empty spots, to be able to mark new chips between updates
*/
std::array<Nier::ChipsListIndex, Nier::dMaxStorableChipCount> Nier::chipsListIndexes;
std::array<Nier::ChipWrapper, Nier::dMaxStorableChipCount> Nier::chipsList{}; // Local copy of pointers to chips, used for sorting the list
/*
A mutex is required since the chipsList array could be read/modified by both: the Dear Imgui render function (called by the DirectX thread);
and in the main loop inside mainFunction.
*/
std::mutex Nier::mtxChipsList;

uintptr_t Nier::moduleBaseAddress;
void (*Nier::updateChipsCount)(void* pChipsBaseAddr);
DWORD* Nier::pdIsWorldLoaded;
DWORD* Nier::pdIsInAMenu;

BOOL Nier::bAutoDelete = FALSE;
Mem::hook_t* Nier::autoDeleteHook;

BOOL Nier::bOSD = TRUE;
ImFont* Nier::osdFont = nullptr;
const float Nier::osdFontSize = 18;

const std::unordered_map<int, Chip::Level> Nier::chipsLevelsTable = {
	{ -1, { -1,	 0,	 0 } },
	{  0, {  0,	 6,	 4 } },
	{  1, {  1,	 7,	 5 } },
	{  2, {  2,	 8,	 6 } },
	{  3, {  3,	 9,	 7 } },
	{  4, {  4,	11,	 9 } },
	{  5, {  5,	13,	11 } },
	{  6, {  6,	16,	14 } },
	{  7, {  7,	19,	17 } },
	{  8, {  8,	23,	21 } }
};

const std::unordered_map<int, Chip::Type> Nier::chipsTypeTable = {
	{ 0x01, { 0x01, "Weapon Attack Up",		Chip::Category::ATTACK		} },
	{ 0x02, { 0x02, "Down-Attack Up",		Chip::Category::ATTACK		} },
	{ 0x03, { 0x03, "Critical Up",			Chip::Category::ATTACK		} },
	{ 0x04, { 0x04, "Ranged Attack Up",		Chip::Category::ATTACK		} },
	{ 0x05, { 0x05, "Fast Cooldown",		Chip::Category::SUPPORT		} },
	{ 0x06, { 0x06, "Melee Defence Up",		Chip::Category::DEFENSE		} },
	{ 0x07, { 0x07, "Ranged Defence Up",	Chip::Category::DEFENSE		} },
	{ 0x08, { 0x08, "Anti Chain Damage",	Chip::Category::DEFENSE		} },
	{ 0x09, { 0x09, "Max HP Up",			Chip::Category::DEFENSE		} },
	{ 0x0A, { 0x0A, "Offensive Heal",		Chip::Category::DEFENSE		} },
	{ 0x0B, { 0x0B, "Deadly Heal",			Chip::Category::DEFENSE		} },
	{ 0x0C, { 0x0C, "Auto-Heal",			Chip::Category::DEFENSE		} },
	{ 0x0D, { 0x0D, "Evade Range Up",		Chip::Category::SUPPORT		} },
	{ 0x0E, { 0x0E, "Moving Speed Up",		Chip::Category::SUPPORT		} },
	{ 0x0F, { 0x0F, "Drop Rate Up",			Chip::Category::SUPPORT		} },
	{ 0x10, { 0x10, "EXP Gain Up",			Chip::Category::SUPPORT		} },
	{ 0x11, { 0x11, "Shock Wave",			Chip::Category::ATTACK		} },
	{ 0x12, { 0x12, "Last Stand",			Chip::Category::ATTACK		} },
	{ 0x13, { 0x13, "Damage Absorb",		Chip::Category::DEFENSE		} },
	{ 0x14, { 0x14, "Vengeance",			Chip::Category::SUPPORT		} },
	{ 0x15, { 0x15, "Reset",				Chip::Category::DEFENSE		} },
	{ 0x16, { 0x16, "Overclock",			Chip::Category::SUPPORT		} },
	{ 0x17, { 0x17, "Resilience",			Chip::Category::DEFENSE		} },
	{ 0x18, { 0x18, "Counter",				Chip::Category::ATTACK		} },
	{ 0x19, { 0x19, "Taunt Up",				Chip::Category::SUPPORT		} },
	{ 0x1A, { 0x1A, "Charge Attack",		Chip::Category::ATTACK		} },
	{ 0x1B, { 0x1B, "Auto-Use Item",		Chip::Category::SUPPORT		} },
	{ 0x1D, { 0x1D, "Hijack Boost",			Chip::Category::HACKING		} },
	{ 0x1E, { 0x1E, "Stun",					Chip::Category::HACKING		} },
	{ 0x1F, { 0x1F, "Combust",				Chip::Category::HACKING		} },
	{ 0x22, { 0x22, "Heal Drops Up",		Chip::Category::HACKING		} },
	{ 0x23, { 0x23, "Item Scan",			Chip::Category::SUPPORT		} },
	{ 0x26, { 0x26, "Death Rattle",			Chip::Category::HACKING		} },
	{ 0x27, { 0x27, "HUD: HP Gauge",		Chip::Category::SYSTEM		} },
	{ 0x28, { 0x28, "HUD: Sound Waves",		Chip::Category::SYSTEM		} },
	{ 0x29, { 0x29, "HUD: Enemy Data",		Chip::Category::SYSTEM		} },
	{ 0x2A, { 0x2A, "OS Chip",				Chip::Category::SYSTEM		} },
	{ 0x2C, { 0x2C, "Evasive System",		Chip::Category::SUPPORT		} },
	{ 0x2D, { 0x2D, "Continuous Combo",		Chip::Category::ATTACK		} },
	{ 0x2E, { 0x2E, "Bullet Detonation",	Chip::Category::SUPPORT		} },
	{ 0x2F, { 0x2F, "Auto-Collect Items",	Chip::Category::SUPPORT		} },
	{ 0x30, { 0x30, "HUD: Skill Gauge",		Chip::Category::SYSTEM		} },
	{ 0x31, { 0x31, "HUD: Text Log",		Chip::Category::SYSTEM		} },
	{ 0x32, { 0x32, "HUD: Mini-map",		Chip::Category::SYSTEM		} },
	{ 0x33, { 0x33, "HUD: EXP Gauge",		Chip::Category::SYSTEM		} },
	{ 0x34, { 0x34, "HUD: Save Points",		Chip::Category::SYSTEM		} },
	{ 0x35, { 0x35, "HUD: Damage Values",	Chip::Category::SYSTEM		} },
	{ 0x36, { 0x36, "HUD: Objectives",		Chip::Category::SYSTEM		} },
	{ 0x37, { 0x37, "HUD: Control",			Chip::Category::SYSTEM		} },
	{ 0x3A, { 0x3A, "HUD: Fishing Spots",	Chip::Category::SYSTEM		} },
	{ 0x3B, { 0x3B, "Auto-Attack",			Chip::Category::SUPPORT		} },
	{ 0x3C, { 0x3C, "Auto-Fire",			Chip::Category::SUPPORT		} },
	{ 0x3D, { 0x3D, "Auto-Evade",			Chip::Category::SUPPORT		} },
	{ 0x3E, { 0x3E, "Auto-Program",			Chip::Category::SUPPORT		} },
	{ 0x3F, { 0x3F, "Auto-Weapon Switch",	Chip::Category::SUPPORT		} },
};

Nier::Nier()
{
	std::cout << "[#] Initializing Nier Class" << std::endl;

	Nier::moduleBaseAddress = (uintptr_t)GetModuleHandle(L"NieRAutomata.exe");
	std::cout << "[+] Module base: " << std::hex << Nier::moduleBaseAddress << std::endl;

	Nier::pChips = (Chips*)(Nier::moduleBaseAddress + 0xF5D0C0);
	Nier::updateChipsCount = (void (*)(void*))(PVOID)(Nier::moduleBaseAddress + 0x7D5020);

	Nier::pdIsWorldLoaded = (DWORD*)(Nier::moduleBaseAddress + 0x1268910);
	Nier::pdIsInAMenu = (DWORD*)(Nier::moduleBaseAddress + 0x1414240);

	// Set all the indexes as empty
	Nier::chipsListIndexes.fill({TRUE, FALSE, -1});

	Nier::updateChipsListAndCount();
	Nier::removeNewStatusFromChips();
}

Nier::~Nier()
{
	clearForExit();
}

void Nier::updateChipsListAndCount() {
	if (pChips != nullptr)
	{
		dChipsCount = 0;
		chipsList = {};
		ChipItem* c;
		for (int row = 0, i = 0; row < 300; row++)
		{
			c = &pChips->pInventory->chips[row];
			if (c->baseId != -1 && c->alwaysZero == 0)
			{
				// If the chip was just added, set the status "new"
				if (chipsListIndexes[row].isEmpty)
				{
					chipsListIndexes[row].isEmpty = FALSE;
					chipsListIndexes[row].isNew = TRUE;
					chipsListIndexes[row].baseId = c->baseId;
				}
				else
				{
					// When chips are fused, one is overwritten and one is cleared
					// Therefore, if baseId change, set the NEW status
					if (chipsListIndexes[row].baseId != c->baseId) {
						chipsListIndexes[row].baseId = c->baseId;
						chipsListIndexes[row].isNew = TRUE;
					}
				}

				chipsList[i].item = c;
				chipsList[i].type = &Nier::chipsTypeTable.at(c->type);
				chipsList[i].level = &Nier::chipsLevelsTable.at(c->level);
				chipsList[i].status = Chip::Status_None;
				chipsList[i].chipsListIndex = &chipsListIndexes[row];

				// Trash status
				if (c->weight > chipsList[i].level->maxWorthRank)
					chipsList[i].status |= Chip::Status_Trash;
				// New status
				if (chipsListIndexes[row].isNew)
					chipsList[i].status |= Chip::Status_New;

				i++;
				dChipsCount++;
			}
			else
			{
				// If the chip was deleted, clear the status "new"
				if (!chipsListIndexes[row].isEmpty)
				{
					chipsListIndexes[row].isEmpty = TRUE;
					chipsListIndexes[row].isNew = FALSE;
					chipsListIndexes[row].baseId = -1;
				}
			}
		}
	}
}

void Nier::removeNewStatusFromChips() {
	ChipWrapper* c;

	for (int i = 0; i < chipsList.size(); i++) {
		c = &chipsList[i];
		if (c->item == NULL) continue;

		c->chipsListIndex->isNew = FALSE;
		c->status &= ~Chip::Status_New;
	}
}

void Nier::removeNewStatusFromChip(ChipWrapper* chip) {
	if (chip)
	{
		chip->chipsListIndex->isNew = FALSE;
		chip->status &= ~Chip::Status_New;
	}
}

void Nier::resetChipsList() {
	if (Nier::pChips != nullptr)
	{
		Nier::updateChipsCount((PVOID)Nier::pChips);
		Nier::updateChipsListAndCount();
		Nier::removeNewStatusFromChips();
		Nier::isChipsListDirty = TRUE;
	}
}

BOOL Nier::isInAMenu() {
	return *pdIsInAMenu;
}

BOOL Nier::isWorldLoaded() {
	return pdIsWorldLoaded != NULL && *pdIsWorldLoaded;
}

BOOL Nier::isAutoDeleteActive()
{
	return bAutoDelete;
}

void Nier::toggleAutoDelete()
{
	bAutoDelete = !bAutoDelete;

	if (bAutoDelete)
	{
		Nier::autoDeleteHook = new Mem::hook_t;
		Mem::detour64((uintptr_t)(Nier::moduleBaseAddress + 0x7CB382), AUTODELETE, 6, Nier::autoDeleteHook);
	}
	else
	{
		Mem::patch(Nier::autoDeleteHook->pHookedAddr, Nier::autoDeleteHook->pOriginalBytes, Nier::autoDeleteHook->len);
		delete Nier::autoDeleteHook;
	}
}

BOOL Nier::isOSDActive()
{
	return bOSD;
}

void Nier::toggleOSD()
{
	bOSD = !bOSD;
}

void Nier::clearForExit()
{
	if (isAutoDeleteActive()) toggleAutoDelete();
}
