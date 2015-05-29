#ifndef RENDER_PASSES_HPP
#define RENDER_PASSES_HPP

#include "FBO.hpp"
#include "VAO.hpp"
#include "Core/Object.hpp"
#include "Viewport.hpp"
#include "Mobile.hpp"
#include "Material.hpp"
#include "Containers/l_unordered_map.hpp"

class Render;
class Window;
struct Events;

class RenderPasses
{
public:
	RenderPasses(Position& p, Window& w, Render& r);

	void Draw(Events e, float alpha);

	using Custom = std::function<void(float alpha)>;
	void CreateCustom(Object obj, Custom draw);

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
	void Camera(Object camera);

private:
	void WindowResize(Eigen::Vector2i size);

	Render& r;
	Window& w;
	Mobile mobile;
	Object camera;

	FBO fbo;

	//UBO shared with all shaders
	ShaderProgram simpleShader;
	UBO commonUBO;

	l_unordered_map<Object, Custom> customs;

	Material screenMat;
	VAO screenQuad;
	Viewport view;
};

static const char* shaderOutputs[] = { "color", "picker" };

#endif