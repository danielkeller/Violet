#include "stdafx.h"
#include "Assets.hpp"

#include "UI/Layout.hpp"
#include "UI/Elements.hpp"
#include "UI/Text.hpp"
#include "File/Filesystem.hpp"

#include "File/Wavefront.hpp"

Asset::Asset()
	: thumb("assets/cube.png")
{}

Asset::Asset(std::string name)
	: thumb("assets/cube.png"), name(name)
{}

bool Assets::Draw(std::string& cur)
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
	auto button = buttons.begin();

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

			UI::DrawBox(box, button->GetColor(), 0);
			UI::DrawQuad(it->thumb, box);
			if (cur == it->name)
				UI::DrawHlBox(box);
			UI::DrawText(it->name.substr(it->name.size() - THM_CHARS, it->name.size()), textBox);

			if (button->Draw(box))
			{
				cur = it->name;
				ret = true;
			}
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
			a.assets.emplace_back(path);
			a.assets.back().thumb = Thumb(path);
		}
}

bool ObjAssets::Draw(VertexData& cur)
{
	std::string curName = cur.Name();
	bool ret = a.Draw(curName);
	cur = curName;
	return ret;
}