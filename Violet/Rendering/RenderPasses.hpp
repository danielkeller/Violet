#ifndef RENDER_PASSES_HPP
#define RENDER_PASSES_HPP

#include "FBO.hpp"
#include "Shader.hpp"
#include "Material.hpp"
#include "VAO.hpp"
#include "Object.hpp"

class Render;
class Window;

class RenderPasses
{
public:
	RenderPasses(Window& w, Render& r);

	void WindowResize(Eigen::Vector2i size);
	void Draw();

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
	Object Pick(Vector2f posView) const;

private:
	Render& r;

	FBO fbo;

	ShaderProgram screenShader;
	Material screenMat;
	VAO screenQuad;
};

static const char* shaderOutputs[] = { "color", "picker" };

#endif