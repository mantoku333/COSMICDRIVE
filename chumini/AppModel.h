#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <CubismModelSettingJson.hpp> 
#include "Live2D/Framework/src/Rendering/D3D11/CubismRenderer_D3D11.hpp" 
#include <Motion/CubismMotionManager.hpp>
#include <Effect/CubismPose.hpp>
#include <vector>
#include <string>
#include "D3D.h"

namespace sf { class Texture; }

class AppModel : public Live2D::Cubism::Framework::CubismUserModel
{
public:
    AppModel();
    virtual ~AppModel();

    void LoadAssets(ID3D11Device* device, const std::string& dir, const std::string& fileName);
    void Update(); 
    void Draw(ID3D11Device* device, ID3D11DeviceContext* context, const Live2D::Cubism::Framework::CubismMatrix44& matrix);

    void StartMotion(const char* group, int no, int priority); // Animation trigger

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11* GetMyRenderer() const { return _myRenderer; }

private:
    void SetupTextures(ID3D11Device* device);

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11* _myRenderer;
    Live2D::Cubism::Framework::CubismModelSettingJson* _modelSetting; 
    std::string _modelHomeDir;
    std::vector<sf::Texture*> _loadedTextures; 

    // Managers
    Live2D::Cubism::Framework::CubismMotionManager* _motionManager;
    Live2D::Cubism::Framework::CubismPose* _pose;
};
