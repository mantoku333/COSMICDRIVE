#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <Rendering/D3D11/CubismRenderer_D3D11.hpp>
#include <vector>
#include "Texture.h" 

using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::Rendering;

// ƒNƒ‰ƒX–¼‚ð•ÏX: LAppModel -> AppModel
class AppModel : public CubismUserModel {
public:
    AppModel();
    virtual ~AppModel();

    void LoadAssets(ID3D11Device* device, const std::string& dir, const std::string& fileName);
    void Update();
    void Draw(ID3D11Device* device, ID3D11DeviceContext* context, const Csm::CubismMatrix44& matrix);

    CubismRenderer_D3D11* GetMyRenderer() const { return _myRenderer; }

protected:
    Csm::csmInt32 GetTextureDirectoryIndex(const Csm::csmString& path);

private:
    CubismRenderer_D3D11* _myRenderer = nullptr;
    std::string _modelHomeDir;
    std::vector<sf::Texture*> _loadedTextures;
};