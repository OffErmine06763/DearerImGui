#pragma once
#include "Renderable.h"

class UIApp
{
public:

	template <RenderableType S>
	static void Init()
	{
		if (Instance == nullptr)
		{
			InitializeUI();
			UIApp::HRLoadUIDLL();

			Instance = std::unique_ptr<UIApp>(new UIApp());
			// create state only after initialization
			Instance->m_NewStateRequested = true;
			Instance->m_NextState = std::make_unique<S>();
		}
	}
	static void Run();
	static void Stop()
	{
		if (Instance != nullptr)
		{
			Instance.reset();
			UIApp::HRUnloadUIDLL();
			DeinitializeUI();
		}
	}

	template <RenderableType S, typename... Args>
	static void RequestNewState(Args&&... args)
	{
		Instance->m_NextState = std::make_unique<S>(std::forward<Args>(args)...);
		Instance->m_NewStateRequested = true;
	}
	static inline void RequestClose() { Instance->m_IsOpen = false; }

	static void Render(bool* done)
	{
		Instance->_Render();
		if (!Instance->m_IsOpen)
			*done = true;
	}

private:
	static int InitializeUI();
	static void DeinitializeUI();


public:


private:
	UIApp() = default;
	~UIApp() = default;

	void _Render()
	{
		if (m_NewStateRequested)
		{
			m_NewStateRequested = false;
			delete m_Page.release();
			m_Page.swap(m_NextState);
			m_Page->Init();
			m_Page->OnUIReloaded();
		}

		ProcessGlobalShortcuts();
		m_Page->Render();
	}
	void ProcessGlobalShortcuts() {}


private:
	static std::unique_ptr<UIApp> Instance;

	std::unique_ptr<BaseRenderable> m_Page = nullptr, m_NextState = nullptr;
	bool m_IsOpen = true, m_NewStateRequested = false;


public:
	friend struct std::default_delete<UIApp>;



public:
	static void HRLoadUIDLL()
	{
		// TODO: fix the hardcoded path, also giveup on visual studio since it doesn't support recompiling a single project while the others are running
#ifdef UI_HOT_RELOAD
		// fs::path p = fs::current_path() / "..\\x64\\Debug Dynamic\\UI.dll";
		fs::path p = "build/debug/UI.dll";
		bool res = CopyFileA(p.string().c_str(), "UI_temp.dll", FALSE);
		if (res == 0)
			std::cout << GetLastError();
		Loader::Load("UI_temp.dll");

		auto fn = Loader::GetFunction<std::function<void(void*)>>("UI_temp.dll", "OnLoad").value();
		fn(ImGui::GetCurrentContext());
#endif
	}
	static void HRUnloadUIDLL()
	{
#ifdef UI_HOT_RELOAD
		Loader::Unload("UI_temp.dll");
#endif
	}
	static void HRReloadUIDLL()
	{
#ifdef UI_HOT_RELOAD
		HRUnloadUIDLL();
		HRLoadUIDLL();
		if (Instance != nullptr && Instance->m_Page != nullptr)
			Instance->m_Page->OnUIReloaded();
#endif
	}
};