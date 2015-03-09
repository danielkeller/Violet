#include "stdafx.h"

#include "Picker.hpp"
#include "Render.hpp"
#include "Window.hpp"

Picker::Picker(Render& r, Window& w)
	: shader("assets/picker")
	, r(r), w(w), pickedObj(Object::none)
{
	r.PassDefaults(PickerPass, shader, {});
}

TypedTex<std::uint32_t> Picker::WindowResize(Eigen::Vector2i size)
{
	auto tex = TypedTex<std::uint32_t>{ size };
	fbo.AttachTex(tex);
	fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, size });
	fbo.CheckStatus();

	return tex;
}

void Picker::Pick()
{
    auto bound = fbo.Bind(GL_FRAMEBUFFER);
    fbo.PreDraw({Object::none.Id(),0,0,0});
    r.DrawPass(PickerPass);
    
    auto objId = fbo.ReadPixel(w.MousePosView());
    
    pickedObj = Object(objId);
}

Object Picker::Picked() const
{
    return pickedObj;
}
