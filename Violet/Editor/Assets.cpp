#include "stdafx.h"
#include "Assets.hpp"

#include "UI/Layout.hpp"
#include "UI/Elements.hpp"
#include "UI/Text.hpp"
#include "File/Filesystem.hpp"

#include "Rendering/VertexData.hpp"
#include "File/Wavefront.hpp"
#include "File/Persist.hpp"

using namespace Asset_detail;

template<typename Key>
Asset<Key>::Asset()
	: thumb("assets/cube.png")
{}

template<typename Key>
Asset<Key>::Asset(std::string name, Key key)
	: thumb("assets/cube.png"), name(name), key(key)
{}

template<typename Key>
bool Assets<Key>::Draw(Key& cur, std::function<void(Asset<Key>&)> edit)
{
	UI::LayoutStack& l = UI::CurLayout();
	l.PushNext(UI::Layout::Dir::Down);
	l.EnsureWidth(WIDTH);
	UI::PushZ(3);

	UI::DrawBox(l.Current());
	UI::DrawShadow(l.Current());

	int height = l.Current().maxFill;

	Vector2i origin = l.Current().Box().corner(UI::AlignedBox2i::TopLeft);
	Vector2i size{ THM_SIZE, THM_SIZE };

	auto it = assets.begin();
	buttons.resize(assets.size());
	if (edit) editButtons.resize(assets.size());
	auto button = buttons.begin();
	auto editButton = editButtons.begin();

	bool ret = false;

	UI::PushZ();
	for (int y = THM_SPACE; y < height; y += THM_SIZE + THM_SPACE)
	{
		for (int x = THM_SPACE; x < WIDTH; x += THM_SIZE + THM_SPACE, ++it, ++button)
		{
			if (it == assets.end())
				goto done;
			Vector2i init = origin + Vector2i{ x, - THM_SIZE - y };
			UI::AlignedBox2i box{ init, init + size };
			UI::AlignedBox2i textBox{ init, init + Vector2i{ THM_SIZE, UI::LINEH } };

			UI::DrawQuad(it->thumb, box);
			if (cur == it->key)
			{
				UI::PushZ();
				UI::DrawHlBox(box);
				UI::PopZ();
			}

			UI::PushZ();
			std::string::size_type strbegin = std::max(0, int(it->name.size()) - THM_CHARS);
			UI::DrawBox(textBox, button->GetColor(), button->GetColor());
			UI::DrawText(it->name.substr(strbegin, it->name.size()), textBox);

			if (edit)
			{
				UI::PushZ();
				UI::AlignedBox2i editBox{ textBox.max() - Vector2i{ UI::LINEH, UI::LINEH }, textBox.max() };
				static Tex editIcon{ "assets/edit.png" };
				UI::DrawQuad(editIcon, editBox);
				UI::DrawBox(editBox, editButton->GetColor(), editButton->GetColor());
				if (editButton->Behavior(editBox))
					edit(*it);
				++editButton;
				UI::PopZ();
			}

			if (button->Behavior(box))
			{
				cur = it->key;
				ret = true;
			}

			UI::PopZ();
		}
	}
done:
	UI::PopZ();

	l.Pop();

	UI::PopZ();
	return ret;
}

ObjAssets::ObjAssets()
{
	for (auto path : Browse("assets"))
		if (IsWavefront(path))
		{
			a.assets.emplace_back(path, path);
			a.assets.back().thumb = Thumb(path);
		}
}

bool ObjAssets::Draw(VertexData& cur)
{
	std::string curName = cur.Name();
	bool ret = a.Draw(curName, {});
	if (ret) cur = curName;
	return ret;
}

MaterialAssets::MaterialAssets(Persist& persist)
	: editorOn(false)
{
	for (auto mat : persist.GetAll<Material>())
	{
		a.assets.emplace_back(std::get<1>(mat), std::get<0>(mat));
		a.assets.back().thumb = Thumb(
			Material(std::get<0>(mat), std::get<1>(mat), std::get<2>(mat),
				std::get<3>(mat), std::get<4>(mat)));
	}
}

bool MaterialAssets::Draw(Material& cur, Persist& persist)
{
	if (editorOn && edit.Draw(persist))
	{
		//TODO: replace every object using the material
		editorOn = false;
	}

	Material::Id curName = cur.id;
	bool ret = a.Draw(curName, [&](Asset<Material::Id>& mat){
		editorOn = true;
		edit.Edit(Material(curName, persist));
	});
	if (ret) cur = Material(curName, persist);
	return ret;
}