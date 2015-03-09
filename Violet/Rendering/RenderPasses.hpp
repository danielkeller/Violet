#ifndef RENDER_PASSES_HPP
#define RENDER_PASSES_HPP

#include "FBO.hpp"
#include "Shader.hpp"
#include "Material.hpp"
#include "VAO.hpp"
#include "Picker.hpp"

class Render;
class Window;

enum Passes
{
	PickerPass,
	NumPasses,
	AllPasses
};

class RenderPasses
{
public:
	RenderPasses(Window& w, Render& r);

	void WindowResize(Eigen::Vector2i size);
	void Draw();

	//set the highlighted object
	void Highlight(Object o, Picker::Highlights type);
	Picker& Picker() { return pickerPass; }

private:
	Render& r;

	FBO<> mainPass;
	::Picker pickerPass;

	ShaderProgram screenShader;
	Material screenMat;
	VAO screenQuad;

	Eigen::Vector2i GetDim() { return{}; }
};

#endif