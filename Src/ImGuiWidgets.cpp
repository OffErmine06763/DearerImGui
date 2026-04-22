#include "ImGuiWidgets.hpp"

/*

// [SECTION] Forward Declarations of ImGui Internals Copies
// [SECTION] Custom Widgets
// [SECTION] Modified ImGui Widgets
// [SECTION] Utility Functions
// [SECTION] ImGui Internal Copies

*/

//-------------------------------------------------------------------------
// [SECTION] Forward Declarations of ImGui Internals
//-------------------------------------------------------------------------

static void BoxSelectPreStartDrag(ImGuiID id, ImGuiSelectionUserData clicked_item);
static void BoxSelectDeactivateDrag(ImGuiBoxSelectState* bs);
static void DebugLogMultiSelectRequests(const char* function, const ImGuiMultiSelectIO* io);
static ImRect CalcScopeRect(ImGuiMultiSelectTempData* ms, ImGuiWindow* window);


//-------------------------------------------------------------------------
// [SECTION] Custom Widgets
//-------------------------------------------------------------------------
// - DrawLegendScale
// - DrawStackedProgressBar

void DrawLegendScale(ImVec2 size, float min, float max)
{
	ImGui::Text("%.2f", min);
	ImGui::SameLine();

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 p0 = ImGui::GetCursorScreenPos();
	ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);

	const int steps = 100; // more = smoother

	for (int i = 0; i < steps; ++i)
	{
		float t0 = (float)i / steps;
		float t1 = (float)(i + 1) / steps;

		// Long-way hue interpolation: 2/3 -> 0
		float h0 = (2.0f / 3.0f) * (1.0f - t0);
		float h1 = (2.0f / 3.0f) * (1.0f - t1);

		ImU32 c0 = ImColor::HSV(h0, 1.0f, 1.0f);
		ImU32 c1 = ImColor::HSV(h1, 1.0f, 1.0f);

		float x0 = Lerp(p0.x, p1.x, t0);
		float x1 = Lerp(p0.x, p1.x, t1);

		draw_list->AddRectFilledMultiColor(
			ImVec2(x0, p0.y),
			ImVec2(x1, p1.y),
			c0, c1, c1, c0
		);
	}

	// Advance layout cursor
	ImGui::Dummy(size);
	ImGui::SameLine();
	ImGui::Text("%.2f", max);
}

int DrawStackedProgressBar(const std::vector<float>& values, const ImVec2& size)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return -1;

	float sum = 0.0f;
	for (float v : values)
		sum += ImMax(v, 0.0f);

	if (sum <= 0.0f)
		return -1;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 bar_size = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight());

	ImRect bb(pos, ImVec2(pos.x + bar_size.x, pos.y + bar_size.y));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return -1;

	// Background
	ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, ImGui::GetStyle().FrameRounding);

	ImDrawList* draw = ImGui::GetWindowDrawList();

	std::vector<ImU32> colors =
	{
		IM_COL32(255, 80, 80, 255),
		IM_COL32(80, 200, 80, 255),
		IM_COL32(80, 120, 255, 255)
	};

	int hovering = -1;

	float x = bb.Min.x;
	for (int i = 0; i < values.size(); i++)
	{
		float v = ImMax(values[i], 0.0f);
		if (v == 0.0f)
			continue;

		float w = (v / sum) * bar_size.x;
		if (w <= 0.0f)
			continue;

		ImU32 col = colors[i % colors.size()];

		draw->AddRectFilled(
			ImVec2(x, bb.Min.y), ImVec2(x + w, bb.Max.y),
			col, ImGui::GetStyle().FrameRounding,
			(x == bb.Min.x) ? ImDrawFlags_RoundCornersLeft : 0);

		if (ImGui::IsMouseHoveringRect(ImVec2(x, bb.Min.y), ImVec2(x + w, bb.Max.y)))
		{
			hovering = i;
		}

		x += w;
	}

	return hovering;
}


//-------------------------------------------------------------------------
// [SECTION] Modified ImGui Widgets
//-------------------------------------------------------------------------
// - MyEndMultiSelect

/// Fixes a bug with the ImGui::EndMultiSelect, that occurs when selecting and deselecting ranges, causing a erroneous start of range selection
/// This is based on v1.92.7
ImGuiMultiSelectIO* MyEndMultiSelect()
{
	using namespace ImGui;

	ImGuiContext& g = *GImGui;
	ImGuiMultiSelectTempData* ms = g.CurrentMultiSelect;
	ImGuiMultiSelectState* storage = ms->Storage;
	ImGuiWindow* window = g.CurrentWindow;
	IM_ASSERT_USER_ERROR(ms->FocusScopeId == g.CurrentFocusScopeId, "EndMultiSelect() FocusScope mismatch!");
	IM_ASSERT(g.CurrentMultiSelect != NULL && storage->Window == g.CurrentWindow);
	IM_ASSERT(g.MultiSelectTempDataStacked > 0 && &g.MultiSelectTempData[g.MultiSelectTempDataStacked - 1] == g.CurrentMultiSelect);

	ImRect scope_rect = CalcScopeRect(ms, window);
	if (ms->IsFocused)
	{
		// We currently don't allow user code to modify RangeSrcItem by writing to BeginIO's version, but that would be an easy change here.
		if (ms->IO.RangeSrcReset || (ms->RangeSrcPassedBy == false && ms->IO.RangeSrcItem != ImGuiSelectionUserData_Invalid)) // Can't read storage->RangeSrcItem here -> we want the state at begining of the scope (see tests for easy failure)
		{
			IMGUI_DEBUG_LOG_SELECTION("[selection] EndMultiSelect: Reset RangeSrcItem.\n"); // Will set be to NavId.
			storage->RangeSrcItem = ImGuiSelectionUserData_Invalid;
		}
		if (ms->NavIdPassedBy == false && storage->NavIdItem != ImGuiSelectionUserData_Invalid)
		{
			IMGUI_DEBUG_LOG_SELECTION("[selection] EndMultiSelect: Reset NavIdItem.\n");
			storage->NavIdItem = ImGuiSelectionUserData_Invalid;
			storage->NavIdSelected = -1;
		}

		if ((ms->Flags & (ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_BoxSelect2d)) && GetBoxSelectState(ms->BoxSelectId))
			EndBoxSelect(scope_rect, ms->Flags);
	}

	if (ms->IsEndIO == false)
		ms->IO.Requests.resize(0);

	// Clear selection when clicking void?
	// We specifically test for IsMouseDragPastThreshold(0) == false to allow box-selection!
	// The InnerRect test is necessary for non-child/decorated windows.
	bool scope_hovered = IsWindowHovered() && window->InnerRect.Contains(g.IO.MousePos);
	if (scope_hovered && (ms->Flags & ImGuiMultiSelectFlags_ScopeRect))
		scope_hovered &= scope_rect.Contains(g.IO.MousePos);
	if (scope_hovered && g.HoveredId == 0 && g.ActiveId == 0)
	{
		if (ms->Flags & (ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_BoxSelect2d))
		{
			if (!g.BoxSelectState.IsActive && !g.BoxSelectState.IsStarting && g.IO.MouseClickedCount[0] == 1)
			{
				BoxSelectPreStartDrag(ms->BoxSelectId, ImGuiSelectionUserData_Invalid);
				FocusWindow(window, ImGuiFocusRequestFlags_UnlessBelowModal);
				SetHoveredID(ms->BoxSelectId);
				if (ms->Flags & ImGuiMultiSelectFlags_ScopeRect)
					SetNavID(0, ImGuiNavLayer_Main, ms->FocusScopeId, ImRect(g.IO.MousePos, g.IO.MousePos)); // Automatically switch FocusScope for initial click from void to box-select.
			}
		}

		if (ms->Flags & ImGuiMultiSelectFlags_ClearOnClickVoid)
		{
			if (IsMouseReleased(0) && IsMouseDragPastThreshold(0) == false && g.IO.KeyMods == ImGuiMod_None)
			{
				MultiSelectAddSetAll(ms, false);
				ms->Storage->RangeSrcItem = ms->Storage->NavIdItem;
			}
		}
	}

	// Courtesy nav wrapping helper flag
	if (ms->Flags & ImGuiMultiSelectFlags_NavWrapX)
	{
		IM_ASSERT(ms->Flags & ImGuiMultiSelectFlags_ScopeWindow); // Only supported at window scope
		ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), ImGuiNavMoveFlags_WrapX);
	}

	// Unwind
	window->DC.CursorMaxPos = ImMax(ms->BackupCursorMaxPos, window->DC.CursorMaxPos);
	PopFocusScope();

	if (g.DebugLogFlags & ImGuiDebugLogFlags_EventSelection)
		DebugLogMultiSelectRequests("EndMultiSelect", &ms->IO);

	ms->FocusScopeId = 0;
	ms->Flags = ImGuiMultiSelectFlags_None;
	g.CurrentMultiSelect = (--g.MultiSelectTempDataStacked > 0) ? &g.MultiSelectTempData[g.MultiSelectTempDataStacked - 1] : NULL;

	return &ms->IO;
}




//-------------------------------------------------------------------------
// [SECTION] Utility Functions
//-------------------------------------------------------------------------


float Lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}


//-------------------------------------------------------------------------
// [SECTION] ImGui Internal Copies
//-------------------------------------------------------------------------
// - BoxSelectPreStartDrag
// - BoxSelectDeactivateDrag
// - DebugLogMultiSelectRequests
// - CalcScopeRect

static void BoxSelectPreStartDrag(ImGuiID id, ImGuiSelectionUserData clicked_item)
{
	ImGuiContext& g = *GImGui;
	ImGuiBoxSelectState* bs = &g.BoxSelectState;
	bs->ID = id;
	bs->IsStarting = true; // Consider starting box-select.
	bs->IsStartedFromVoid = (clicked_item == ImGuiSelectionUserData_Invalid);
	bs->IsStartedSetNavIdOnce = bs->IsStartedFromVoid;
	bs->KeyMods = g.IO.KeyMods;
	bs->StartPosRel = bs->EndPosRel = ImGui::WindowPosAbsToRel(g.CurrentWindow, g.IO.MousePos);
	bs->ScrollAccum = ImVec2(0.0f, 0.0f);
}
static void BoxSelectDeactivateDrag(ImGuiBoxSelectState* bs)
{
	ImGuiContext& g = *GImGui;
	bs->IsActive = bs->IsStarting = false;
	if (g.ActiveId == bs->ID)
	{
		IMGUI_DEBUG_LOG_SELECTION("[selection] BeginBoxSelect() 0X%08X: Deactivate\n", bs->ID);
		ImGui::ClearActiveID();
	}
	bs->ID = 0;
}

static void DebugLogMultiSelectRequests(const char* function, const ImGuiMultiSelectIO* io)
{
	ImGuiContext& g = *GImGui;
	IM_UNUSED(function);
	for (const ImGuiSelectionRequest& req : io->Requests)
	{
		if (req.Type == ImGuiSelectionRequestType_SetAll)    IMGUI_DEBUG_LOG_SELECTION("[selection] %s: Request: SetAll %d (= %s)\n", function, req.Selected, req.Selected ? "SelectAll" : "Clear");
		if (req.Type == ImGuiSelectionRequestType_SetRange)  IMGUI_DEBUG_LOG_SELECTION("[selection] %s: Request: SetRange %" IM_PRId64 "..%" IM_PRId64 " (0x%" IM_PRIX64 "..0x%" IM_PRIX64 ") = %d (dir %d)\n", function, req.RangeFirstItem, req.RangeLastItem, req.RangeFirstItem, req.RangeLastItem, req.Selected, req.RangeDirection);
	}
}

static ImRect CalcScopeRect(ImGuiMultiSelectTempData* ms, ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	if (ms->Flags & ImGuiMultiSelectFlags_ScopeRect)
	{
		// Warning: this depends on CursorMaxPos so it means to be called by EndMultiSelect() only
		return ImRect(ms->ScopeRectMin, ImMax(window->DC.CursorMaxPos, ms->ScopeRectMin));
	}
	else
	{
		// When a table, pull HostClipRect, which allows us to predict ClipRect before first row/layout is performed. (#7970)
		ImRect scope_rect = window->InnerClipRect;
		if (g.CurrentTable != NULL)
			scope_rect = g.CurrentTable->HostClipRect;

		// Add inner table decoration (#7821) // FIXME: Why not baking in InnerClipRect?
		scope_rect.Min = ImMin(scope_rect.Min + ImVec2(window->DecoInnerSizeX1, window->DecoInnerSizeY1), scope_rect.Max);
		return scope_rect;
	}
}