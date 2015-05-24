#include "stdafx.h"
#include "MaterialEdit.hpp"
#include "UI/Layout.hpp"
#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "Window.hpp"

MaterialEdit::MaterialEdit()
	: mat(), instances(1), vao({ "assets/blank" }, UnitBox)
	, matName(WIDTH)
{
	cam = ShaderProgram{ "assets/simple" }.MakeUBO("Common");

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

Material MaterialEdit::Current() const
{
	return mat;
}

bool MaterialEdit::Draw(Persist& persist)
{
	UI::ModalBoxRAII modalBox(UI::Layout::Dir::Right);
	if (modalBox.closed)
		return true;

	UI::LayoutStack& l = UI::CurLayout();

	//render the preview
	Viewport view(l.PutSpace(WIDTH));
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
	l.EnsureWidth(WIDTH);
	
	if (matName.Draw(mat.name))
		mat.Save(persist);

	size_t nFloatUnifs = 0;
	for (const auto& unif : mat.ubo.Uniforms())
	{
		l.PutSpace(UI::LINEH);
		UI::DrawText(unif.name, l.PutSpace(UI::LINEH), UI::TextAlign::Left);

		//TODO: arrays
		if (unif.type.scalar == GL_FLOAT)
		{
			size_t curUnif = nFloatUnifs;
			nFloatUnifs += unif.type.rows*unif.type.cols;
			//grow to fit
			floatUnifs.resize(std::max(floatUnifs.size(), nFloatUnifs), UI::FloatEdit(WIDTH/4));
			
			auto map = mat.ubo[unif.name].Map<float>(unif.type.rows, unif.type.cols);
			auto dispMap = unif.type.cols == 1 //display column vectors as rows
				? map.transpose() : map;
			
			for (int row = 0; row < dispMap.rows(); ++row)
			{
				l.PushNext(UI::Layout::Dir::Right);
				for (int col = 0; col < dispMap.cols(); ++col, ++curUnif)
				{
					if (floatUnifs[curUnif].Draw(dispMap(row, col)))
						mat.Save(persist);
					//live editing
					if (floatUnifs[curUnif].editing)
						mat.ubo.Sync();
				}
				l.Pop();
			}
		}
	}

	l.Pop();

	return false;
}