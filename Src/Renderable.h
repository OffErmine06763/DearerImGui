#pragma once
#include "Utility.hpp"

#ifdef HOT_RELOAD
#define BIND_RENDER_FUNCTION(type, fn) m_Render = Loader::GetFunction<consumer<type*>>("UI_temp.dll", #fn).value();
#else
#define BIND_RENDER_FUNCTION(type, fn) m_Render = fn;
#endif

class BaseRenderable
{
public:
	virtual ~BaseRenderable() = default;

	virtual void Render() = 0;
	virtual void Init() {}
	virtual void OnUIReloaded() = 0;
};


template <typename Context>
class Renderable : public BaseRenderable
{
public:
	virtual ~Renderable() override = default;

	void Render() override
	{
		m_Render(&context);
		HandleUIEvents();
	}

protected:

	virtual void HandleUIEvents() {}

protected:
	consumer<Context*> m_Render;

	Context context;
};

template <typename S, std::enable_if_t<std::is_base_of_v<BaseRenderable, S>, bool> = true>
concept RenderableType = true;