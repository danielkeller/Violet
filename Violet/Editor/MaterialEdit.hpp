#ifndef MATERIAL_EDIT_HPP
#define MATERIAL_EDIT_HPP

#include "Rendering/Material.hpp"
#include "Rendering/Render.hpp"
#include "UI/Elements.hpp"

class Persist;

class MaterialEdit
{
public:
	MaterialEdit();
	//returns true to dismiss
	bool Draw(Persist&);
	void Edit(Material mat);

private:
	static const int WIDTH = 400;

	Material mat;

	UBO cam;
	BufferObject<InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> instances;
	VAO vao;

	UI::LineEdit matName;
};

#endif