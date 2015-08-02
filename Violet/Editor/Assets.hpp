#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "Rendering/Texture.hpp"
#include "Rendering/Material.hpp"
#include "UI/Elements.hpp"
#include "MaterialEdit.hpp"

class Material;
class VertexData;

class AssetsBar
{
public:
    AssetsBar();
    
    //true if selection changed
    bool DrawObj(VertexData& cur);
    bool DrawMat(Material& cur, Persist&);
    
    void Close();
    
private:
    UI::SlideInOut enter, change;
    
    struct ObjAsset
    {
        ObjAsset(const std::string& path);
        Tex thumb;
        UI::Button button;
    };
    std::unordered_map<std::string, ObjAsset> objAssets;
    
    struct MatAsset
    {
        MatAsset(const Material& mat);
        Tex thumb;
        std::string name;
        UI::Button button;
        UI::IconButton editButton, deleteButton;
    };
    std::unordered_map<Material::Id, MatAsset> matAssets;
    
    bool matEditorOn;
    MaterialEdit matEdit;
};

namespace Asset_detail
{
	static const int THM_SIZE = 100;
	static const int THM_CHARS = 13;
	static const int ROWS = 4;
    static const int WIDTH = THM_SIZE*ROWS + UI::GRID_SPACING*(ROWS + 1);
    
    Tex ObjThumb(const std::string& path);
    Tex MatThumb(const Material& mat);
}

#endif