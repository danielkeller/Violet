#ifndef RENDER_PASSES_HPP
#define RENDER_PASSES_HPP

#include "FBO.hpp"
#include "Shader.hpp"
#include "Material.hpp"
#include "VAO.hpp"
#include "Object.hpp"
#include "Viewport.hpp"

class Render;
class Window;
class Events;

class RenderPasses
{
public:
	RenderPasses(Window& w, Render& r);

	void Draw(const Matrix4f& camera, Events e);

	enum Passes
	{
		MainPass,
		PickerPass,
		NumPasses,
	};

	enum Highlights
	{
		//Ordered by priority
		Focused = 0,
		Selected,
		Hovered,
	};

	//set the highlighted object
	void Highlight(Object o, Highlights type);
	Object Pick(Vector2f posPixel) const;

private:
	void WindowResize(Eigen::Vector2i size);

	Render& r;
	Window& w;

	FBO fbo;

	ShaderProgram screenShader;
	Material screenMat;
	VAO screenQuad;
	Viewport view;
};

static const char* shaderOutputs[] = { "color", "picker" };

#endif