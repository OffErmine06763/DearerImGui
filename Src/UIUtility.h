#pragma once
#include "DearerImGuiUtility.hpp"

#ifdef UI_HOT_RELOAD
#ifdef BUILD_UI
#define UI_API EXPORT
#else
#define UI_API IMPORT
#endif
#else
#define UI_API
#endif