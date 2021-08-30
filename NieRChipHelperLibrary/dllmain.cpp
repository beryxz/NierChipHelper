#include "pch.h"

#include <Windows.h>
#include "imgui.h"

#include "Hook.h"
#include "Nier.h"

bool g_bActivated = FALSE;

void menu();
void osd(ImDrawList* drawList);
 
void customImguiDraw() {
	auto bgDrawlist = ImGui::GetBackgroundDrawList();
	
	menu();

	if (Nier::isOSDActive()) {
		osd(bgDrawlist);
	}
}

// On screen display for chips count
void osd(ImDrawList* drawlist) {
	char text[128];
	sprintf_s(text, 128, "Total chips");

	drawlist->AddText(ImVec2(10.0f, 10.0f), (ImU32)IM_COL32_BLACK, text);
}

void menu() {
	ImGui::Text("%d/200 chips in inventory", Nier::dChipsCount);
	if (Nier::pChips != nullptr)
		ImGui::Text("in-game chip counter: %d", Nier::pChips->totChips);

	ImGui::Text("[%c] Auto-delete new useless chips:", Nier::isAutoDeleteActive() ? 'X' : ' ');
	ImGui::SameLine();
	if (ImGui::Button("Toggle"))
	{
		Nier::toggleAutoDelete();
	}

	ImGuiTableFlags flags =
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingFixedFit |
		ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti;
	if (ImGui::BeginTable("chipsTable", 4, flags))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_DefaultSort);
		ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_PreferSortAscending);
		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		// Sort our data if sort specs have been changed!
		__try {
			if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
				if (sorts_specs->SpecsDirty)
				{
					std::sort(Nier::chipsList.begin(), Nier::chipsList.begin() + Nier::dChipsCount, [sorts_specs](const Chip* a, const Chip* b) {
						if (a == nullptr || b == nullptr) return false;

						for (int n = 0; n < sorts_specs->SpecsCount; n++)
						{
							const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
							int delta = 0;

							switch (sort_spec->ColumnIndex)
							{
							case 0: // Name
								delta = (Nier::chipsTypeTable.at(a->type).name.compare(Nier::chipsTypeTable.at(b->type).name));
								break;
							case 1: // Level
								delta = (a->level - b->level);
								break;
							case 2: // Weight
								delta = (a->weight - b->weight);
								break;
							}

							if (delta > 0)
								return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
							if (delta < 0)
								return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
						}

						return false; // default case if all columns are equal
						});

					sorts_specs->SpecsDirty = false;
				}
			}
		}
		__except (filterException(GetExceptionCode(), GetExceptionInformation())) {
			std::cout << "[!] Error: Table sorting function" << std::endl;
		}


		__try {
			int row = 0;
			for (Chip* c : Nier::chipsList)
			{
				if (c != nullptr && c->baseId != -1 && c->alwaysZero == 0)
				{
					ImGui::PushID(row);
					ImGui::TableNextRow();

					const Nier::ChipLevel* cl = &Nier::chipsLevelsTable.at(c->level);
					const Nier::ChipType* ct = &Nier::chipsTypeTable.at(c->type);

					// Set color based on chip category
					ImU32 row_bg_color = NULL;
					switch (ct->category) {
					case Nier::CHIP_ATTACK:
						row_bg_color = ImGui::GetColorU32(ImVec4(0.529f, 0.494f, 0.400f, 0.7f));
						break;
					case Nier::CHIP_DEFENSE:
						row_bg_color = ImGui::GetColorU32(ImVec4(0.718f, 0.600f, 0.494f, 0.7f));
						break;
					case Nier::CHIP_HACKING:
						row_bg_color = ImGui::GetColorU32(ImVec4(0.906f, 0.882f, 0.780f, 0.7f));
						break;
					case Nier::CHIP_SUPPORT:
						row_bg_color = ImGui::GetColorU32(ImVec4(0.890f, 0.851f, 0.655f, 0.7f));
						break;
					case Nier::CHIP_SYSTEM:
						row_bg_color = ImGui::GetColorU32(ImVec4(0.741f, 0.686f, 0.545f, 0.7f));
						break;
					}
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, row_bg_color);

					// Set color based on chip usefulness
					if (c->weight > cl->maxWorthRank) {
						ImGui::TableSetBgColor(
							ImGuiTableBgTarget_RowBg1,
							ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.45f)));
					}

					for (int column = 0; column < 4; column++)
					{
						ImGui::TableSetColumnIndex(column);

						char buf[64];

						switch (column) {
						case 0:
							sprintf_s(buf, "%s%s", ct->name.c_str(), c->weight == cl->diamondRank ? " *" : "");
							ImGui::TextUnformatted(buf);
							break;
						case 1:
							sprintf_s(buf, "%d", c->level);
							ImGui::TextUnformatted(buf);
							break;
						case 2:
							sprintf_s(buf, "%d", c->weight);
							ImGui::TextUnformatted(buf);
							break;
						case 3:
							if (ImGui::Button("Delete")) {
								std::cout << "Deleting chip test\n";
								c->clear();
							};
							break;
						}
					}
					ImGui::PopID();
					row++;
				}
			}
		}
		__except (filterException(GetExceptionCode(), GetExceptionInformation())) {
			std::cout << "[!] Error: Table drawer" << std::endl;
		}
		ImGui::EndTable();
	}
}

DWORD WINAPI mainThread(HMODULE hModule)
{
	Hook* hook = new Hook();
	hook->toggleConsole();
	hook->initialize();
	
	std::cout << "[*] Main thread started" << std::endl;

	Nier::moduleBaseAddress = (uintptr_t)GetModuleHandle(L"NieRAutomata.exe");
	std::cout << "[*] NieRAutomata.exe base: " << std::hex << Nier::moduleBaseAddress << std::endl;
	Nier::pChips = (Chips*)(Nier::moduleBaseAddress + 0xF5D0C0);

	Nier::updateChipsListAndCount();

	// Main loop
	while (true)
	{
		// If number of chips changed, update the local chips array copy
		// if totChips == 0, gamesave has not been loaded yet.
		if (Nier::pChips && Nier::pChips->totChips != 0 && Nier::dChipsCount != Nier::pChips->totChips) {
			Nier::updateChipsListAndCount();
		}

		if (GetAsyncKeyState(VK_F3) & 1) {
			break;
		}

		Sleep(5);
	}

	Nier::clearForExit();
	delete hook;
	FreeLibraryAndExitThread(hModule, 0);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)mainThread, hModule, 0, nullptr));
		break;
    }
    case DLL_PROCESS_DETACH:
		break;
    }
    return TRUE;
}
