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
	: thumb("assets/cube.png"), slide(200ms)
{}

template<typename Key>
Asset<Key>::Asset(std::string name, Key key)
	: thumb("assets/cube.png"), name(name), key(key)
{}

template<typename Key>
bool Assets<Key>::Draw(Key& cur, std::function<void(Asset<Key>&, UI::AlignedBox2i)> edit)
{
	UI::LayoutStack& l = UI::CurLayout();
	bool ret = slide.Draw(WIDTH);
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
					edit(*it, box);
				++editButton;
				UI::PopZ();
			}

			if (button->Behavior(box))
			{
				cur = it->key;
				slide.Close();
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
	//TODO: ListAll?
	for (auto matTup : persist.GetAll<Material>())
	{
		Material mat{ std::get<0>(matTup), persist };
		a.assets.emplace_back(mat.Name(), mat.Key());
		a.assets.back().thumb = Thumb(mat);
	}
}

bool MaterialAssets::Draw(Material& cur, Persist& persist)
{
	if (editorOn && edit.Draw(persist))
	{
		//update the thumbnail
		Material& edited = edit.Current();
		auto editedAsset = std::find(a.assets.begin(), a.assets.end(), edited.GetId());
		editedAsset->thumb = Thumb(edited);
		editedAsset->name = edited.Name();

		editorOn = false;
	}

	Material::Id curName = cur.GetId();
	bool ret = a.Draw(curName, [&](Asset<Material::Id>& mat, UI::AlignedBox2i box){
		editorOn = true;
		edit.Edit(Material{ curName, persist }, box);
	});
	if (ret) cur = Material{ curName, persist };
	return ret;
}