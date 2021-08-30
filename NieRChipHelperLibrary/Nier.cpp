#include "pch.h"
#include "Nier.h"

extern "C" LPVOID AUTODELETE();
extern "C" LPVOID AUTODELETE_SKIP;
LPVOID AUTODELETE_SKIP = (LPVOID)((uintptr_t)GetModuleHandle(L"NieRAutomata.exe") + 0x7CB47E);
extern "C" LPVOID AUTODELETE_JUMPBACK;
LPVOID AUTODELETE_JUMPBACK = (LPVOID)((uintptr_t)GetModuleHandle(L"NieRAutomata.exe") + 0x7CB388);

Chips* Nier::pChips = nullptr; // pointer to chips counters and inventory location
DWORD Nier::dChipsCount = 0;
std::array<Chip*, 300> Nier::chipsList{}; // Local copy of pointers to chips, used for sorting the list

uintptr_t Nier::moduleBaseAddress;

BOOL Nier::bAutoDelete = FALSE;
Mem::hook_t* Nier::autoDeleteHook;

BOOL Nier::bOSD = TRUE;

const std::unordered_map<int, Nier::ChipLevel> Nier::chipsLevelsTable = {
	{ 0, { 0,	 6,	 4 } },
	{ 1, { 1,	 7,	 5 } },
	{ 2, { 2,	 8,	 6 } },
	{ 3, { 3,	 9,	 7 } },
	{ 4, { 4,	11,	 9 } },
	{ 5, { 5,	13,	11 } },
	{ 6, { 6,	16,	14 } },
	{ 7, { 7,	19,	17 } }
};

const std::unordered_map<int, Nier::ChipType> Nier::chipsTypeTable = {
	{ 0x01, { 0x01, "Weapon Attack Up",		Nier::CHIP_ATTACK	} },
	{ 0x02, { 0x02, "Down-Attack Up",		Nier::CHIP_ATTACK	} },
	{ 0x03, { 0x03, "Critical Up",			Nier::CHIP_ATTACK	} },
	{ 0x04, { 0x04, "Ranged Attack Up",		Nier::CHIP_ATTACK	} },
	{ 0x05, { 0x05, "Fast Cooldown",		Nier::CHIP_SUPPORT	} },
	{ 0x06, { 0x06, "Melee Defence Up",		Nier::CHIP_DEFENSE	} },
	{ 0x07, { 0x07, "Ranged Defence Up",	Nier::CHIP_DEFENSE	} },
	{ 0x08, { 0x08, "Anti Chain Damage",	Nier::CHIP_DEFENSE	} },
	{ 0x09, { 0x09, "Max HP Up",			Nier::CHIP_DEFENSE	} },
	{ 0x0A, { 0x0A, "Offensive Heal",		Nier::CHIP_DEFENSE	} },
	{ 0x0B, { 0x0B, "Deadly Heal",			Nier::CHIP_DEFENSE	} },
	{ 0x0C, { 0x0C, "Auto-Heal",			Nier::CHIP_DEFENSE	} },
	{ 0x0D, { 0x0D, "Evade Range Up",		Nier::CHIP_SUPPORT	} },
	{ 0x0E, { 0x0E, "Moving Speed Up",		Nier::CHIP_SUPPORT	} },
	{ 0x0F, { 0x0F, "Drop Rate Up",			Nier::CHIP_SUPPORT	} },
	{ 0x10, { 0x10, "EXP Gain Up",			Nier::CHIP_SUPPORT	} },
	{ 0x11, { 0x11, "Shock Wave",			Nier::CHIP_ATTACK	} },
	{ 0x12, { 0x12, "Last Stand",			Nier::CHIP_ATTACK	} },
	{ 0x13, { 0x13, "Damage Absorb",		Nier::CHIP_DEFENSE	} },
	{ 0x14, { 0x14, "Vengeance",			Nier::CHIP_SUPPORT	} },
	{ 0x15, { 0x15, "Reset",				Nier::CHIP_DEFENSE	} },
	{ 0x16, { 0x16, "Overclock",			Nier::CHIP_SUPPORT	} },
	{ 0x17, { 0x17, "Resilience",			Nier::CHIP_DEFENSE	} },
	{ 0x18, { 0x18, "Counter",				Nier::CHIP_ATTACK	} },
	{ 0x19, { 0x19, "Taunt Up",				Nier::CHIP_SUPPORT	} },
	{ 0x1A, { 0x1A, "Charge Attack",		Nier::CHIP_ATTACK	} },
	{ 0x1B, { 0x1B, "Auto-Use Item",		Nier::CHIP_SUPPORT	} },
	{ 0x1D, { 0x1D, "Hijack Boost",			Nier::CHIP_HACKING	} },
	{ 0x1E, { 0x1E, "Stun",					Nier::CHIP_HACKING	} },
	{ 0x1F, { 0x1F, "Combust",				Nier::CHIP_HACKING	} },
	{ 0x22, { 0x22, "Heal Drops Up",		Nier::CHIP_HACKING	} },
	{ 0x23, { 0x23, "Item Scan",			Nier::CHIP_SUPPORT	} },
	{ 0x26, { 0x26, "Death Rattle",			Nier::CHIP_HACKING	} },
	{ 0x27, { 0x27, "HUD: HP Gauge",		Nier::CHIP_SYSTEM	} },
	{ 0x28, { 0x28, "HUD: Sound Waves",		Nier::CHIP_SYSTEM	} },
	{ 0x29, { 0x29, "HUD: Enemy Data",		Nier::CHIP_SYSTEM	} },
	{ 0x2A, { 0x2A, "OS Chip",				Nier::CHIP_SYSTEM	} },
	{ 0x2C, { 0x2C, "Evasive System",		Nier::CHIP_SUPPORT	} },
	{ 0x2D, { 0x2D, "Continuous Combo",		Nier::CHIP_ATTACK	} },
	{ 0x2E, { 0x2E, "Bullet Detonation",	Nier::CHIP_SUPPORT	} },
	{ 0x2F, { 0x2F, "Auto-Collect Items",	Nier::CHIP_SUPPORT	} },
	{ 0x30, { 0x30, "HUD: Skill Gauge",		Nier::CHIP_SYSTEM	} },
	{ 0x31, { 0x31, "HUD: Text Log",		Nier::CHIP_SYSTEM	} },
	{ 0x32, { 0x32, "HUD: Mini-map",		Nier::CHIP_SYSTEM	} },
	{ 0x33, { 0x33, "HUD: EXP Gauge",		Nier::CHIP_SYSTEM	} },
	{ 0x34, { 0x34, "HUD: Save Points",		Nier::CHIP_SYSTEM	} },
	{ 0x35, { 0x35, "HUD: Damage Values",	Nier::CHIP_SYSTEM	} },
	{ 0x36, { 0x36, "HUD: Objectives",		Nier::CHIP_SYSTEM	} },
	{ 0x37, { 0x37, "HUD: Control",			Nier::CHIP_SYSTEM	} },
	{ 0x3A, { 0x3A, "HUD: Fishing Spots",	Nier::CHIP_SYSTEM	} },
	{ 0x3B, { 0x3B, "Auto-Attack",			Nier::CHIP_SUPPORT	} },
	{ 0x3C, { 0x3C, "Auto-Fire",			Nier::CHIP_SUPPORT	} },
	{ 0x3D, { 0x3D, "Auto-Evade",			Nier::CHIP_SUPPORT	} },
	{ 0x3E, { 0x3E, "Auto-Program",			Nier::CHIP_SUPPORT	} },
	{ 0x3F, { 0x3F, "Auto-Weapon Switch",	Nier::CHIP_SUPPORT	} },
};

void Nier::updateChipsListAndCount() {
	if (pChips != nullptr)
	{
		dChipsCount = 0;
		chipsList = {};
		Chip* c;
		for (int row = 0, i = 0; row < 300; row++)
		{
			c = &pChips->pInventory->chips[row];
			if (c->baseId != -1 && c->alwaysZero == 0)
			{
				chipsList[i++] = c;
				dChipsCount++;
			}
		}
	}
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

BOOL Nier::isAutoDeleteActive()
{
	return bAutoDelete;
}

BOOL Nier::isOSDActive()
{
	return bOSD;
}

void Nier::clearForExit()
{
	if (isAutoDeleteActive()) toggleAutoDelete();
}
