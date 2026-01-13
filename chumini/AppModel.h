#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <CubismModelSettingJson.hpp> 
#include "Live2D/Framework/src/Rendering/D3D11/CubismRenderer_D3D11.hpp" 

#include <Motion/CubismMotionManager.hpp>
#include <Effect/CubismPose.hpp>
#include <Effect/CubismEyeBlink.hpp>
#include <Effect/CubismBreath.hpp>
#include <Physics/CubismPhysics.hpp>

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
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

    void StartMotion(const char* group, int no, int priority); 
    void StartGlitchMotion(const char* group, int no); // ★New Method for Glitch 

    // Manual Override
    void SetParamOverride(const char* id, float value);
    void ClearParamOverrides();

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11* GetMyRenderer() const { return _myRenderer; }
    
    // Look At
    void SetDragging(float x, float y);

private:
    void SetupTextures(ID3D11Device* device); 
    
    float _targetX = 0.0f;
    float _targetY = 0.0f;

    // Glitch Loop State
    bool _isGlitchLooping = false;
    std::string _glitchGroup;
    int _glitchNo = 0;

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11* _myRenderer;
    Live2D::Cubism::Framework::CubismModelSettingJson* _modelSetting; 
    std::string _modelHomeDir;
    std::vector<sf::Texture*> _loadedTextures; 

    // Overrides
    struct OverrideParam { const char* id; float value; };
    std::vector<OverrideParam> _overrides; 

    // Managers
    Live2D::Cubism::Framework::CubismMotionManager* _motionManager;
    Live2D::Cubism::Framework::CubismMotionManager* _glitchManager; // ★Additional Manager for Parallel/Loop Glitch
    Live2D::Cubism::Framework::CubismPose* _pose;
    Live2D::Cubism::Framework::CubismEyeBlink* _eyeBlink;
    Live2D::Cubism::Framework::CubismBreath* _breath;
    Live2D::Cubism::Framework::CubismPhysics* _physics;

    // Async Loading
    std::vector<unsigned char> _pendingMotionData;
    std::atomic<bool> _isMotionDataReady = false;
    std::string _pendingGroup;
    int _pendingNo = 0;
    std::mutex _motionMutex;
    std::mutex _mainMutex;
};
