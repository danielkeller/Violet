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

AssetsBar::ObjAsset::ObjAsset(const std::string& path)
    : thumb(ObjThumb(path))
{}

AssetsBar::MatAsset::MatAsset(const Material& mat)
    : thumb(MatThumb(mat)), name(mat.Name())
    , editButton("assets/edit.png"), deleteButton("assets/delete.png")
{}

AssetsBar::AssetsBar()
    : enter(200ms), change(200ms), matEditorOn(false)
{}

void AssetsBar::Close()
{
    enter.Close();
}

bool AssetsBar::DrawObj(VertexData& cur)
{
    for (auto path : Browse("assets"))
    {
        auto realPath = ends_with(path, ".cache") ? path.substr(0, path.size() - 6) : path;
        if (!objAssets.count(realPath) && IsWavefront(realPath))
            objAssets.emplace(realPath, realPath);
    }
    
    UI::LayoutStack& l = UI::CurLayout();
    bool ret = enter.Draw(WIDTH);
    l.PushNext(UI::Layout::Dir::Down);
    l.EnsureWidth(WIDTH);
    UI::PushZ(3);
    
    UI::DrawBox(l.Current());
    UI::DrawShadow(l.Current());
    
    {
        auto grid = l.StartGrid(THM_SIZE, UI::Layout::Dir::Up);
        
        for (auto& p : objAssets)
        {
            const auto& name = p.first;
            auto& asset = p.second;
            
            auto box = l.Current();
            auto textBox = l.PutSpace(UI::LINEH);
            
            UI::DrawQuad(asset.thumb, box);
            if (cur == name)
            {
                UI::PushZ();
                UI::DrawHlBox(box);
                UI::PopZ();
            }
            
            UI::PushZ();
            std::string::size_type strbegin = std::max(0, int(name.size()) - THM_CHARS);
            UI::Color buttonColor = asset.button.GetColor();
            UI::DrawBox(textBox, buttonColor, buttonColor);
            UI::DrawText(name.substr(strbegin, name.size()), textBox);
            
            if (asset.button.Behavior(box))
            {
                cur = name;
                Close();
            }
            
            UI::PopZ();
            grid.Next(UI::Layout::Dir::Up);
        }
    }
    
    l.Pop();
    UI::PopZ();
    
    return ret;
}

bool AssetsBar::DrawMat(Material& cur, Persist& persist)
{
    //TODO: ListAll?
    for (auto matTup : persist.GetAll<Material>())
    {
        if (!matAssets.count(std::get<0>(matTup)))
        {
            Material mat{ std::get<0>(matTup), persist };
            matAssets.emplace(mat.Key(), mat);
        }
    }
    
	if (matEditorOn && matEdit.Draw(persist))
	{
		//update the thumbnail
		const Material& edited = matEdit.Current();
		auto& editedAsset = matAssets.at(edited.GetId());
		editedAsset.thumb = MatThumb(edited);
		editedAsset.name = edited.Name();

		matEditorOn = false;
	}

    UI::LayoutStack& l = UI::CurLayout();
    bool ret = enter.Draw(WIDTH);
    l.PushNext(UI::Layout::Dir::Down);
    l.EnsureWidth(WIDTH);
    UI::PushZ(3);
    
    UI::DrawBox(l.Current());
    UI::DrawShadow(l.Current());
    
    {
        auto grid = l.StartGrid(THM_SIZE, UI::Layout::Dir::Up);
        
        for (auto& p : matAssets)
        {
            const auto& id = p.first;
            auto& asset = p.second;
            
            auto box = l.Current();
            
            UI::DrawQuad(asset.thumb, box);
            if (cur.Key() == id)
            {
                UI::PushZ();
                UI::DrawHlBox(box);
                UI::PopZ();
            }
            
            UI::PushZ();
            
            l.PushNext(UI::Layout::Dir::Right);
            
            if (asset.editButton.Draw())
            {
                matEditorOn = true;
                matEdit.Edit(Material{ id, persist }, box);
            }
            
            //FIXME: Unicode!
            std::string::size_type strbegin = std::max(0, int(asset.name.size()) - THM_CHARS);
            UI::DrawText(asset.name.substr(strbegin, asset.name.size()),
                         l.PutSpace(THM_SIZE - 2*UI::LINEH));
            
            if (asset.deleteButton.Draw())
                persist.Delete<Material>(id);
            
            //bottom background
            UI::Color buttonColor = asset.button.GetColor();
            UI::DrawBox(l.Current(), buttonColor, buttonColor);
            
            l.Pop();
            UI::PopZ();
            
            //behind the other buttons, so this code has to be after (sigh)
            if (asset.button.Behavior(box))
            {
                cur = { id, persist };
                Close();
            }
            
            grid.Next(UI::Layout::Dir::Up);
        }
    }
    l.Pop();
    UI::PopZ();
    
    return ret;
}