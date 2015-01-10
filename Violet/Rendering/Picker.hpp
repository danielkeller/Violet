#ifndef PICKER_HPP
#define PICKER_HPP

#include "Shader.hpp"
#include "Texture.hpp"
#include "FBO.hpp"
#include "Object.hpp"

class Render;
class Window;

class Picker
{
public:
    Picker(Render&, Window&);
    const ShaderProgram shader;
    void Pick();
    Object Picked() const;
    //set the highlighted object
    void Highlight(Object o);
    
private:
    TypedTex<std::uint32_t> tex;
    FBO<std::uint32_t> fbo;
    const ShaderProgram hlShader;
    UBO hlMat;
    Render& r;
    Window& w;
    Object pickedObj;
};

#endif
