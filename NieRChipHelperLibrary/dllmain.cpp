#include "pch.h"

#include <Windows.h>
#include "imgui.h"

#include "Hook.h"
#include "Nier.h"
#include "fonts.h"

void osd(ImDrawList* drawList);

BOOL loadCustomDearImguiFonts(HMODULE hModule) {
	ImGuiIO& io = ImGui::GetIO();
	Nier::osdFont = io.Fonts->AddFontFromMemoryCompressedBase85TTF(roboto_bold_compressed_data_base85, Nier::osdFontSize);
	io.Fonts->Build();

	return TRUE;
}
 
void customImguiDrawAlways() {
	auto bgDrawlist = ImGui::GetBackgroundDrawList();

	if (Nier::isOSDActive()) {
		osd(bgDrawlist);
	}
}

// On screen display for chips count
void osd(ImDrawList* drawlist) {
	if (Nier::isOSDActive() && Nier::osdFont != nullptr)
	{
		// Chips count
		ImGui::PushFont(Nier::osdFont);
		char text[128];
		sprintf_s(text, 128, "Chips: %d/%d", Nier::dChipsCount, Nier::dMaxChipCount);
		ImVec2 textBasePos = ImVec2(5, 20);
		ImVec2 textSize = ImGui::CalcTextSize(text);
		float padding = 2.0f;
		drawlist->AddRectFilled(
			ImVec2(textBasePos.x-padding, textBasePos.y-padding),
			ImVec2(textBasePos.x+textSize.x+padding, textBasePos.y+textSize.y+padding),
			IM_COL32(140,40,40,255),
			3.0f
		);
		drawlist->AddText(NULL, 0, textBasePos, IM_COL32(200, 200, 200, 255), text);
		ImGui::PopFont();
	}
}

void customImguiDrawMenu() {
	ImGui::Text("%d/%d chips in inventory", Nier::dChipsCount, Nier::dMaxChipCount);

	ImGui::Text("[%c] Chips count OSD:", Nier::isOSDActive() ? 'X' : ' ');
	ImGui::SameLine();
	if (ImGui::SmallButton("Toggle##t1"))
	{
		Nier::toggleOSD();
	}

	ImGui::Text("[%c] Auto-delete new useless chips:", Nier::isAutoDeleteActive() ? 'X' : ' ');
	ImGui::SameLine();
	if (ImGui::SmallButton("Toggle##t2"))
	{
		Nier::toggleAutoDelete();
	}

	ImGui::Separator();

	if (ImGui::Button("Clear \"New Status\" from chips", { ImGui::GetWindowContentRegionWidth(), 18.0f }))
	{
		Nier::removeNewStatusFromChips();
	}

	ImGui::Combo("Shown status in table", &Nier::curShownStatusIndex, "All\0Empty\0Trash\0New\0\0");

	const int NUM_COLUMNS = 5;
	ImGuiTableFlags flags =
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingFixedFit |
		ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti;
	if (ImGui::BeginTable("chipsTable", NUM_COLUMNS, flags))
	{
		ImGui::TableSetupColumn("Name",		ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthStretch	);
		ImGui::TableSetupColumn("Level",	ImGuiTableColumnFlags_DefaultSort												);
		ImGui::TableSetupColumn("Weight",	ImGuiTableColumnFlags_PreferSortAscending										);
		ImGui::TableSetupColumn("Status",	ImGuiTableColumnFlags_NoSort			  | ImGuiTableColumnFlags_WidthStretch	);
		ImGui::TableSetupColumn("Action",	ImGuiTableColumnFlags_NoSort													);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		// Sort our data if sort specs have been changed!
		if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
			if (sorts_specs->SpecsDirty || Nier::isChipsListDirty)
			{
				std::sort(Nier::chipsList.begin(), Nier::chipsList.begin() + Nier::dChipsCount, [sorts_specs](const Nier::ChipWrapper a, const Nier::ChipWrapper b) {
					if (a.item == nullptr || b.item == nullptr) return false;

					for (int n = 0; n < sorts_specs->SpecsCount; n++)
					{
						const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
						int delta = 0;

						switch (sort_spec->ColumnIndex)
						{
						case 0: // Name
							delta = a.type.name.compare(b.type.name);
							break;
						case 1: // Level
							delta = (a.item->level - b.item->level);
							break;
						case 2: // Weight
							delta = (a.item->weight - b.item->weight);
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
				Nier::isChipsListDirty = FALSE;
			}
		}

		// render table
		int row = 0;
		for (const Nier::ChipWrapper c : Nier::chipsList)
		{
			if (c.item == nullptr || c.item->baseId == -1 || c.item->alwaysZero != 0) continue;
			if ((Nier::curShownStatusIndex == 1 && c.status != Chip::Status_None)    ||
				(Nier::curShownStatusIndex == 2 && !(c.status & Chip::Status_Trash)) ||
				(Nier::curShownStatusIndex == 3 && !(c.status & Chip::Status_New))) continue;

			ImGui::PushID(row);
			ImGui::TableNextRow();

			const Chip::Level* cl = &c.level;
			const Chip::Type* ct = &c.type;

			// Set color based on chip category
			ImU32 row_bg_color = NULL;
			switch (ct->category) {
			case Chip::Category::ATTACK:
				row_bg_color = ImGui::GetColorU32(ImVec4(0.529f, 0.494f, 0.400f, 0.7f));
				break;
			case Chip::Category::DEFENSE:
				row_bg_color = ImGui::GetColorU32(ImVec4(0.718f, 0.600f, 0.494f, 0.7f));
				break;
			case Chip::Category::HACKING:
				row_bg_color = ImGui::GetColorU32(ImVec4(0.906f, 0.882f, 0.780f, 0.7f));
				break;
			case Chip::Category::SUPPORT:
				row_bg_color = ImGui::GetColorU32(ImVec4(0.890f, 0.851f, 0.655f, 0.7f));
				break;
			case Chip::Category::SYSTEM:
				row_bg_color = ImGui::GetColorU32(ImVec4(0.741f, 0.686f, 0.545f, 0.7f));
				break;
			}
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, row_bg_color);

			for (int column = 0; column < NUM_COLUMNS; column++)
			{
				ImGui::TableSetColumnIndex(column);

				char buf[64];
				switch (column) {
				case 0:
					sprintf_s(buf, "%s%s", ct->name.c_str(), c.item->weight == cl->diamondRank ? " *" : "");
					ImGui::TextUnformatted(buf);
					break;
				case 1:
					sprintf_s(buf, "%d", c.item->level);
					ImGui::TextUnformatted(buf);
					break;
				case 2:
					sprintf_s(buf, "%d", c.item->weight);
					ImGui::TextUnformatted(buf);
					break;
				case 3: // Set color based on chip status
				{
					std::string status = "";

					if (c.status & Chip::Status_New)
					{
						status += "[NEW] ";
						for (int i = column; i < NUM_COLUMNS; i++) {
							ImGui::TableSetBgColor(
								ImGuiTableBgTarget_CellBg,
								ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.45f)),
								i
							);
						}
					}
					if (c.status & Chip::Status_Trash)
					{
						status += "[TRASH] ";
						for (int i = column; i < NUM_COLUMNS; i++) {
							ImGui::TableSetBgColor(
								ImGuiTableBgTarget_CellBg,
								ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.45f)),
								i
							);
						}
					}

					ImGui::TextUnformatted(status.c_str());
				}
					
					break;
				case 4:
					if (ImGui::Button("Delete")) {
						c.item->clear();
						Nier::updateChipsCount((PVOID)Nier::pChips);
						Nier::isChipsListDirty = TRUE;
					};
					break;
				}
			}
			ImGui::PopID();
			row++;
		}

		ImGui::EndTable();
	}
}

// main logic is in this separate function so to be able to use smart pointers
void mainFunction(HMODULE hModule) {
	std::unique_ptr<Hook> hook(new Hook(hModule));
	hook->toggleConsole();

	std::unique_ptr<Nier> nier(new Nier());

	std::cout << "[*] Waiting for world to be loaded..." << std::endl;
	while (Nier::isWorldLoaded == NULL || !*Nier::isWorldLoaded) {
		Sleep(100);
	}

	Nier::updateChipsCount((PVOID)Nier::pChips);
	Nier::updateChipsListAndCount();
	Nier::removeNewStatusFromChips();
	hook->initialize();
	std::cout << "[*] Ready!" << std::endl;

	while (true)
	{
		// If number of chips changed, update the local chips array copy
		// if totChips == 0, gamesave has not been loaded yet.
		if (Nier::pChips && Nier::pChips->totChips != 0 && Nier::dChipsCount != Nier::pChips->totChips) {
			/*
			Edge case (because of a BUG?):
			When fusing chips, the first time you fuse chips, the in-game chips counter is decreased by 2 instead of only 1.
			After the first time, and until the next the chips count is updated, each fuse the counter it's decreased by 1 as expected.
			Therefore if in a menu, first update internal chip count. Otherwise an infinite loop will occur.
			*/
			if (Nier::isInAMenu)
			{
				Nier::updateChipsCount((PVOID)Nier::pChips);
				Nier::isChipsListDirty = TRUE;
			}
			
			Nier::updateChipsListAndCount();
		}

		if (GetAsyncKeyState(VK_F3) & 1) {
			break;
		}

		Sleep(5);
	}
}

DWORD WINAPI mainThread(HMODULE hModule)
{
	mainFunction(hModule);
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
