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

	enum Highlights
	{
		//Ordered by priority
		Focused = 0,
		Selected,
		Hovered,
	};
    
	TypedTex<std::uint32_t> WindowResize(Eigen::Vector2i size);

private:
    FBO<std::uint32_t> fbo;
    Render& r;
    Window& w;
    Object pickedObj;
};

#endif
