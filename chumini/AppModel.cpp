#include "AppModel.h"
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include "Live2D/Framework/src/Rendering/D3D11/CubismRenderer_D3D11.hpp"
#include "Texture.h"
#include <Windows.h> // For OutputDebugStringA

using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::Rendering;

namespace {
    std::vector<csmByte> LoadFileAsBytes(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<csmByte> buffer(size);
        if (file.read((char*)buffer.data(), size)) return buffer;
        return {};
    }
}

// Include for ACubismMotion
#include <Motion/CubismMotion.hpp>
#include <CubismDefaultParameterId.hpp>
#include <Id/CubismIdManager.hpp>

// Helper for loading file (re-use existing or ensure visibility)
// Since the previous LoadFileAsBytes was in anonymous namespace, we can assume it's available or move it if needed.
// Checking previous view, it was in same file.

AppModel::AppModel() : _modelSetting(nullptr), _myRenderer(nullptr), _motionManager(nullptr), _pose(nullptr), _eyeBlink(nullptr), _breath(nullptr), _physics(nullptr) {
}

AppModel::~AppModel() {
    if (_modelSetting) {
        delete _modelSetting;
        _modelSetting = nullptr;
    }
    
    // Managers cleanup
    if (_motionManager) {
        delete _motionManager;
        _motionManager = nullptr;
    }
    if (_pose) {
        CubismPose::Delete(_pose); 
        _pose = nullptr;
    }
    if (_eyeBlink) {
        CubismEyeBlink::Delete(_eyeBlink);
        _eyeBlink = nullptr;
    }
    if (_breath) {
        CubismBreath::Delete(_breath);
        _breath = nullptr;
    }
    if (_physics) {
        CubismPhysics::Delete(_physics);
        _physics = nullptr;
    }

    // _myRenderer's destructor is protected, and it's managed by the framework/model usually?
    // Or we should just leave it for now as previously decided to avoid compilation errors.
    // if (_myRenderer) { delete _myRenderer; }

    for (auto* tex : _loadedTextures) {
        delete tex;
    }
    _loadedTextures.clear();
}

// MATCH HEADER SIGNATURE: const std::string& dir, const std::string& fileName
void AppModel::LoadAssets(ID3D11Device* device, const std::string& dir, const std::string& fileName) {
    _modelHomeDir = dir;
    std::string model3JsonPath = _modelHomeDir + "/" + fileName;

    auto jsonBytes = LoadFileAsBytes(model3JsonPath);
    if (jsonBytes.empty()) {
        OutputDebugStringA(("Failed to load model3.json: " + model3JsonPath + "\n").c_str());
        return;
    }

    _modelSetting = new CubismModelSettingJson(jsonBytes.data(), static_cast<csmSizeInt>(jsonBytes.size()));

    // Load Moc3
    std::string moc3Path = _modelSetting->GetModelFileName();
    std::string fullMoc3Path = _modelHomeDir + "/" + moc3Path;
    
    auto moc3Bytes = LoadFileAsBytes(fullMoc3Path);
    if (moc3Bytes.empty()) {
         OutputDebugStringA(("Failed to load moc3: " + fullMoc3Path + "\n").c_str());
         return;
    }

    LoadModel(moc3Bytes.data(), static_cast<csmSizeInt>(moc3Bytes.size()));

    // Create Renderer
    CubismRenderer* renderer = CubismRenderer::Create();
    renderer->Initialize(GetModel());
    _myRenderer = static_cast<CubismRenderer_D3D11*>(renderer); // Cast to D3D11 specific renderer

    // Setup Textures
    SetupTextures(device);

    // Initialize Motion Manager
    _motionManager = new CubismMotionManager();
    // _motionManager->SetUserData(this); // REMOVED: Not a member or needed for basic playback

    // Initialize Pose (Fixes "Four Hands" issue)
    const csmChar* poseFileName = _modelSetting->GetPoseFileName();
    if (poseFileName != nullptr) {
        std::string posePath = _modelHomeDir + "/" + poseFileName;
        auto poseBytes = LoadFileAsBytes(posePath);
        if (!poseBytes.empty()) {
            _pose = CubismPose::Create(poseBytes.data(), static_cast<csmSizeInt>(poseBytes.size()));
        }
    }

    // Initialize EyeBlink
    _eyeBlink = CubismEyeBlink::Create(_modelSetting);

    // Initialize Breath
    _breath = CubismBreath::Create();
    
    // Default setup for breath
    csmVector<CubismBreath::BreathParameterData> breathParam;
    CubismIdManager* idManager = CubismFramework::GetIdManager();
    
    breathParam.PushBack(CubismBreath::BreathParameterData(idManager->GetId(DefaultParameterId::ParamAngleX),     0.0f, 15.0f, 6.5345f, 0.5f));
    breathParam.PushBack(CubismBreath::BreathParameterData(idManager->GetId(DefaultParameterId::ParamAngleY),     0.0f, 8.0f, 3.5345f, 0.5f));
    breathParam.PushBack(CubismBreath::BreathParameterData(idManager->GetId(DefaultParameterId::ParamAngleZ),     0.0f, 10.0f, 5.5345f, 0.5f));
    breathParam.PushBack(CubismBreath::BreathParameterData(idManager->GetId(DefaultParameterId::ParamBodyAngleX), 0.0f, 4.0f, 15.5345f, 0.5f));
    breathParam.PushBack(CubismBreath::BreathParameterData(idManager->GetId(DefaultParameterId::ParamBreath),     0.5f, 0.5f, 3.2345f, 0.5f));

    _breath->SetParameters(breathParam);

    // Initialize Physics
    const csmChar* physicsFileName = _modelSetting->GetPhysicsFileName();
    if (physicsFileName != nullptr) {
        std::string physicsPath = _modelHomeDir + "/" + physicsFileName;
        auto physicsBytes = LoadFileAsBytes(physicsPath);
        if (!physicsBytes.empty()) {
            _physics = CubismPhysics::Create(physicsBytes.data(), static_cast<csmSizeInt>(physicsBytes.size()));
        }
    }
    
    GetModel()->SaveParameters(); // Save initial state
}

void AppModel::SetupTextures(ID3D11Device* device) {
    int textureCount = _modelSetting->GetTextureCount();
    for (int i = 0; i < textureCount; i++) {
        std::string texturePath = _modelSetting->GetTextureFileName(i);
        std::string fullTexPath = _modelHomeDir + "/" + texturePath;
        
        namespace fs = std::filesystem;
        
        // Fix path separators
        for(auto& c : fullTexPath) { if(c == '\\') c = '/'; }

        sf::Texture* texture = new sf::Texture(); // Using sf::Texture
        bool success = texture->LoadTextureFromFile(fullTexPath);
        _loadedTextures.push_back(texture);
        
        if (success) {
            // Bind to renderer
            if (_myRenderer) {
                // Access the shader resource view from the Texture class
                // Based on previous debugging, the member is 'srv'
                ID3D11ShaderResourceView* view = texture->srv; 
                
                if (view) {
                    _myRenderer->BindTexture(i, view);
                    OutputDebugStringA((" -> [OK] Success: " + fullTexPath + "\n").c_str());
                } else {
                    OutputDebugStringA((" -> [NG] No SRV: " + fullTexPath + "\n").c_str());
                }
            }
        } else {
            OutputDebugStringA((" -> [NG] Failed to load: " + fullTexPath + "\n").c_str());
        }
    }
    
    // Debug: Check if Renderer has shaders
    if (_myRenderer->GetShaderManager()) {
         OutputDebugStringA(" -> Renderer ShaderManager exists.\n");
    } else {
         OutputDebugStringA(" -> [ERROR] Renderer ShaderManager is NULL!\n");
    }

    if (GetModel()) {
        Csm::csmFloat32 w = GetModel()->GetCanvasWidth();
        Csm::csmFloat32 h = GetModel()->GetCanvasHeight();
        OutputDebugStringA((" -> Model Canvas Size: " + std::to_string(w) + " x " + std::to_string(h) + "\n").c_str());
    }
}

void AppModel::Update() {
    // 1/60 fixed or use actual delta
    const csmFloat32 deltaTimeSeconds = 1.0f / 60.0f; 

    _model->LoadParameters();

    if (_motionManager) {
        _motionManager->UpdateMotion(_model, deltaTimeSeconds);
    }

    _model->SaveParameters();

    // 1. EyeBlink
    if (_eyeBlink) {
        _eyeBlink->UpdateParameters(_model, deltaTimeSeconds);
    }

    // 2. Breath
    if (_breath) {
        _breath->UpdateParameters(_model, deltaTimeSeconds);
    }

    // 3. Physics
    if (_physics) {
        _physics->Evaluate(_model, deltaTimeSeconds);
    }

    // 4. Pose (Parts switching)
    if (_pose) {
         _pose->UpdateParameters(_model, deltaTimeSeconds);
    }

    // 5. Manual Overrides
    if (!_overrides.empty()) {
        Live2D::Cubism::Framework::CubismIdManager* idManager = Live2D::Cubism::Framework::CubismFramework::GetIdManager();
        for (const auto& o : _overrides) {
            Live2D::Cubism::Framework::CubismIdHandle id = idManager->GetId(o.id);
            int idx = _model->GetParameterIndex(id);
            if (idx >= 0) {
                _model->SetParameterValue(idx, o.value);
            }
        }
    }

    _model->Update();
}

void AppModel::SetParamOverride(const char* id, float value) {
    // Check if exists and update, or add
    for (auto& o : _overrides) {
        if (std::string(o.id) == id) {
             o.value = value;
             return;
        }
    }
    _overrides.push_back({ id, value });
}

void AppModel::ClearParamOverrides() {
    _overrides.clear();
}

void AppModel::StartMotion(const char* group, int no, int priority) {
    if (!_modelSetting || !_motionManager) return;

    std::string fileName = _modelSetting->GetMotionFileName(group, no);
    if (fileName.empty()) return;

    std::string path = _modelHomeDir + "/" + fileName;
    auto motionBytes = LoadFileAsBytes(path);

    if (!motionBytes.empty()) {
        // Create motion (ACubismMotion is base, using CubismMotion)
        CubismMotion* motion = CubismMotion::Create(motionBytes.data(), static_cast<csmSizeInt>(motionBytes.size()));
        _motionManager->StartMotionPriority(motion, false, priority);
    }
}

// MATCH HEADER SIGNATURE: const ... & matrix
void AppModel::Draw(ID3D11Device* device, ID3D11DeviceContext* context, const Csm::CubismMatrix44& matrix) {
    if (!_myRenderer) return;

    Csm::CubismMatrix44 projection;
    // projection.Scale(1.0f, 1.0f); // Default
    
    // SDKの仕様上、MultiplyByMatrixは非constポインタを要求するため、キャストして渡す
    // (通常、行列の乗算で右辺が変更されることはないはず)
    projection.MultiplyByMatrix(const_cast<Csm::CubismMatrix44*>(&matrix));

    _myRenderer->SetMvpMatrix(&projection);
    _myRenderer->DrawModel();
}
