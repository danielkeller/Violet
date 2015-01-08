#include "stdafx.h"

#include "Picker.hpp"
#include "Render.hpp"
#include "Window.hpp"

Picker::Picker(Render& r, Window& w)
    : shader("assets/picker"), tex(TexDim{512, 512}), fbo(tex)
    , hlShader("assets/picker_hl"), hlMat(hlShader.MakeUBO("Material", "pickerMat"))
    , r(r), w(w), pickedObj(Object::none)
{
    fbo.AttachDepth(RenderBuffer{GL_DEPTH_COMPONENT, {512, 512}});
    fbo.CheckStatus();
    
    Object pickerObj;
    hlMat["selected"] = Object::none.Id();
    hlMat.Sync();
    r.Create(pickerObj, {hlShader, {}}, {{{hlMat, {tex}}, {}}}, UnitBox, Matrix4f::Identity());
}

void Picker::Pick()
{
    auto bound = fbo.Bind(GL_FRAMEBUFFER);
    fbo.PreDraw({Object::none.Id(),0,0,0});
    r.DrawPass(PickerPass);
    
    auto objId = fbo.ReadPixel(w.mousePosPct());
    hlMat["selected"] = objId;
    hlMat.Sync();
    
    pickedObj = Object(objId);
}

Object Picker::Picked() const
{
    return pickedObj;
}