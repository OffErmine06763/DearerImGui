#pragma once

#include "DearerImGuiUtility.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>



float Lerp(float a, float b, float t);
void DrawLegendScale(ImVec2 size, float min, float max);
int DrawStackedProgressBar(const std::vector<float>& values, const ImVec2& size);

ImGuiMultiSelectIO* MyEndMultiSelect();