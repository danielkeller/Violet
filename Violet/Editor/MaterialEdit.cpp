#include "stdafx.h"
#include "MaterialEdit.hpp"
#include "UI/Layout.hpp"
#include "UI/PixelDraw.hpp"
#include "Window.hpp"

MaterialEdit::MaterialEdit()
	: mat(), instances(1), vao({ "assets/blank" }, UnitBox)
	, matName(WIDTH)
{
	cam = ShaderProgram{ "assets/simple" }.MakeUBO("Common", "PreviewCamera");

	//TODO: not so many Eigen::
	InstData object{ Object::invalid,
		Eigen::Affine3f{ Eigen::Translation3f{ 0, 1, 0 } }.matrix() };
	instances.Assign(0, object);
}

void MaterialEdit::Edit(Material newMat)
{
	mat = newMat;

	static VertexData sample{ "assets/capsule.obj" };
	vao = { mat.shader, sample };
	vao.BindInstanceData(mat.shader, instances);
}

bool MaterialEdit::Draw(Persist& persist)
{
	UI::ModalBoxRAII modalBox(UI::Layout::Dir::Right);
	if (modalBox.closed)
		return true;

	UI::LayoutStack& l = UI::CurLayout();

	//render the preview
	Viewport view(l.PutSpace(300));
	cam["camera"] = view.OrthoMat();
	UI::DrawSpecial([=]()
	{
		mat.use();
		cam.Bind();

		view.GlViewport();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		vao.Draw();
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		UI::SetViewport();
	});
	l.PutSpace(20);

	l.PushNext(UI::Layout::Dir::Down);
	
	if (matName.Draw(mat.name))
		mat.Save(persist);

	l.PutSpace(UI::LINEH);


	l.Pop();

	return false;
}