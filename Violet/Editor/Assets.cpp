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
Assets<Key>::Assets() : slide(200ms) {}

Asset::Asset()
	: thumb("assets/cube.png")
{}

Asset::Asset(std::string name)
	: thumb("assets/cube.png"), name(name)
{}

template<typename Key>
bool Assets<Key>::Draw(Key& cur, std::function<void(const Key&, UI::AlignedBox2i, bool)> edit)
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
	if (edit)
    {
        editButtons.resize(assets.size());
        deleteButtons.resize(assets.size());
    }
    
	auto button = buttons.begin();
	auto editButton = editButtons.begin();
    auto deleteButton = deleteButtons.begin();

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

			UI::DrawQuad(it->second.thumb, box);
			if (cur == it->first)
			{
				UI::PushZ();
				UI::DrawHlBox(box);
				UI::PopZ();
			}

			UI::PushZ();
			const auto& name = it->second.name;
			std::string::size_type strbegin = std::max(0, int(name.size()) - THM_CHARS);
			UI::DrawBox(textBox, button->GetColor(), button->GetColor());
			UI::DrawText(name.substr(strbegin, name.size()), textBox);

			if (edit)
			{
				UI::PushZ();
                UI::AlignedBox2i deleteBox{ textBox.min(), textBox.min() + Vector2i{ UI::LINEH, UI::LINEH }};
                static Tex delIcon{ "assets/delete.png" };
                UI::DrawQuad(delIcon, deleteBox);
                UI::DrawBox(deleteBox, deleteButton->GetColor(), deleteButton->GetColor());
                if (deleteButton->Behavior(deleteBox))
                    edit(it->first, box, true);
                ++deleteButton;
                
				UI::AlignedBox2i editBox{ textBox.max() - Vector2i{ UI::LINEH, UI::LINEH }, textBox.max() };
				static Tex editIcon{ "assets/edit.png" };
				UI::DrawQuad(editIcon, editBox);
				UI::DrawBox(editBox, editButton->GetColor(), editButton->GetColor());
				if (editButton->Behavior(editBox))
					edit(it->first, box, false);
				++editButton;
				UI::PopZ();
			}

			if (button->Behavior(box))
			{
				cur = it->first;
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

template Assets<std::string>::Assets();

bool ObjAssets::Draw(VertexData& cur)
{
    for (auto path : Browse("assets"))
    {
        auto realPath = ends_with(path, ".cache") ? path.substr(0, path.size() - 6) : path;
        if (!a.assets.count(realPath) && IsWavefront(realPath))
        {
            auto it = a.assets.emplace(realPath, realPath).first;
            it->second.thumb = Thumb(realPath);
        }
    }
    
	std::string curName = cur.Name();
	bool ret = a.Draw(curName, {});
	if (curName != cur.Name()) cur = curName;
	return ret;
}

MaterialAssets::MaterialAssets(Persist& persist)
	: editorOn(false)
{}

bool MaterialAssets::Draw(Material& cur, Persist& persist)
{
    //TODO: ListAll?
    for (auto matTup : persist.GetAll<Material>())
    {
        if (!a.assets.count(std::get<0>(matTup)))
        {
            Material mat{ std::get<0>(matTup), persist };
            auto it = a.assets.emplace(mat.Key(), mat.Name()).first;
            it->second.thumb = Thumb(mat);
        }
    }
    
	if (editorOn && edit.Draw(persist))
	{
		//update the thumbnail
		const Material& edited = edit.Current();
		auto& editedAsset = a.assets[edited.GetId()];
		editedAsset.thumb = Thumb(edited);
		editedAsset.name = edited.Name();

		editorOn = false;
	}

	Material::Id curName = cur.GetId();
	bool ret = a.Draw(curName, [&](const Material::Id& mat, UI::AlignedBox2i box, bool del){
        if (del)
            persist.Delete<Material>(mat);
        else
        {
            editorOn = true;
            edit.Edit(Material{ mat, persist }, box);
        }
	});
	if (curName != cur.GetId()) cur = Material{ curName, persist };
	return ret;
}