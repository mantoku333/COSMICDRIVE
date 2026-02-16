#include "AppModel.h"
#include <fstream>
#include <vector>
#include "JobSystem.h"
#include <string>
#include <map>
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

AppModel::AppModel() : _modelSetting(nullptr), _myRenderer(nullptr), _motionManager(nullptr), _glitchManager(nullptr), _pose(nullptr), _eyeBlink(nullptr), _breath(nullptr), _physics(nullptr) {
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
    if (_glitchManager) {
        delete _glitchManager;
        _glitchManager = nullptr;
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

    // Unbind all texture slots before releasing texture objects.
    // Renderer ownership is managed by CubismUserModel (DeleteRenderer).
    if (_myRenderer) {
        for (int i = 0; i < (int)_loadedTextures.size(); i++) {
            _myRenderer->BindTexture(i, nullptr);
        }
        DeleteRenderer();
        _myRenderer = nullptr;
    }

    for (auto* tex : _loadedTextures) {
        delete tex;
    }
    _loadedTextures.clear();
}

// MATCH HEADER SIGNATURE: const std::string& dir, const std::string& fileName
// MATCH HEADER SIGNATURE: const std::string& dir, const std::string& fileName
void AppModel::LoadAssets(ID3D11Device* device, const std::string& dir, const std::string& fileName) {
    if (!device) {
        OutputDebugStringA("AppModel::LoadAssets - [ERROR] Device is NULL!\n");
        return;
    }

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

    // Create Renderer through CubismUserModel ownership
    CreateRenderer();
    _myRenderer = GetRenderer<CubismRenderer_D3D11>();
    if (!_myRenderer) {
        OutputDebugStringA("AppModel::LoadAssets - [ERROR] Failed to create CubismRenderer_D3D11!\n");
        return;
    }

    // Fix for "Transparent Eyes" (Double Alpha Multiply)
    // If the model looks ghostly/transparent, enable this. 
    // Most modern exports might be premultiplied.
    // _myRenderer->IsPremultipliedAlpha(true); // Reverted as per user request

    // Setup Textures
    SetupTextures(device);

    // Initialize Motion Manager
    _motionManager = new CubismMotionManager();
    _glitchManager = new CubismMotionManager(); // Init glitch manager
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

// -------------------------------------------------------------------------
void AppModel::Update() {
    std::lock_guard<std::mutex> dock(_mainMutex);

    // Check Async Load
    std::shared_ptr<AsyncMotionResult> resultToCheck = nullptr;
    {
        std::lock_guard<std::mutex> lock(_motionMutex);
        if (_pendingAsyncResult && _pendingAsyncResult->ready.load(std::memory_order_acquire)) {
            resultToCheck = _pendingAsyncResult;
            _pendingAsyncResult = nullptr; // Clear the pending slot
        }
    }

    if (resultToCheck) {
        if (!resultToCheck->data.empty()) {
            // Cache it!
            std::string key = resultToCheck->group + "_" + std::to_string(resultToCheck->no);
            _motionDataCache[key] = resultToCheck->data;

            CubismMotion* motion = CubismMotion::Create(resultToCheck->data.data(), static_cast<csmSizeInt>(resultToCheck->data.size()));

            // Force Loop to FALSE to rely on Manual Restart
            motion->SetLoop(false);

            // Priority 3. autoDelete = true.
            _glitchManager->StartMotionPriority(motion, true, 3);

            // Store state for manual loop in Update
            _isGlitchLooping = true;
            _glitchGroup = resultToCheck->group;
            _glitchNo = resultToCheck->no;

            OutputDebugStringA(("AppModel: Started Glitch Motion (Loaded & Cached)\n"));
        }
    }

    // 1/60 fixed or use actual delta
    const csmFloat32 deltaTimeSeconds = 1.0f / 60.0f; 

    _model->LoadParameters();

    if (_motionManager) {
        _motionManager->UpdateMotion(_model, deltaTimeSeconds);
    }
    if (_glitchManager) {
        _glitchManager->UpdateMotion(_model, deltaTimeSeconds);
        
        // Manual loop check
        bool finished = _glitchManager->IsFinished();
        if (_isGlitchLooping && finished) {
             // Restart
             // OutputDebugStringA("AppModel: Glitch Finished! Restarting...\n");
             StartGlitchMotionNoLock(_glitchGroup.c_str(), _glitchNo);
        }
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
    
    // 2.5 Apply Look At (Target)
    // Direct set, Physics will pick up the change for hair movement
    if (_model) {
        Live2D::Cubism::Framework::CubismIdManager* idManager = Live2D::Cubism::Framework::CubismFramework::GetIdManager();
        
        // Face Angle
        int idxX = _model->GetParameterIndex(idManager->GetId(DefaultParameterId::ParamAngleX));
        int idxY = _model->GetParameterIndex(idManager->GetId(DefaultParameterId::ParamAngleY));
        int idxZ = _model->GetParameterIndex(idManager->GetId(DefaultParameterId::ParamAngleZ));
        
        if (idxX >= 0) _model->AddParameterValue(idxX, _targetX * 30.0f); // Add to motion/idle
        if (idxY >= 0) _model->AddParameterValue(idxY, _targetY * 30.0f); 
        if (idxZ >= 0) _model->AddParameterValue(idxZ, _targetX * _targetY * -20.0f); // Little tilt
        
        // Eye Ball
        int idxEyeX = _model->GetParameterIndex(idManager->GetId(DefaultParameterId::ParamEyeBallX));
        int idxEyeY = _model->GetParameterIndex(idManager->GetId(DefaultParameterId::ParamEyeBallY));
        
        if (idxEyeX >= 0) _model->AddParameterValue(idxEyeX, _targetX);
        if (idxEyeY >= 0) _model->AddParameterValue(idxEyeY, _targetY);
        
        // Body (slight turn)
        int idxBodyX = _model->GetParameterIndex(idManager->GetId(DefaultParameterId::ParamBodyAngleX));
        if (idxBodyX >= 0) _model->AddParameterValue(idxBodyX, _targetX * 10.0f);
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

void AppModel::SetDragging(float x, float y) {
    _targetX = x;
    _targetY = y;
}

void AppModel::StartMotion(const char* group, int no, int priority) {
    std::lock_guard<std::mutex> lock(_mainMutex);
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

// -------------------------------------------------------------------------
void AppModel::StartGlitchMotion(const char* group, int no) {
    std::lock_guard<std::mutex> lock(_mainMutex);
    StartGlitchMotionNoLock(group, no);
}

void AppModel::StartGlitchMotionNoLock(const char* group, int no) {
    if (!_modelSetting || !_glitchManager) return;

    // Check Cache first
    std::string key = std::string(group) + "_" + std::to_string(no);
    if (_motionDataCache.count(key)) {
        // Use Cached Data (Synchronous creation, very fast)
        // Copy data because Create might not accept const pointer or might modify buffer
        std::vector<unsigned char> data = _motionDataCache[key];
        
        if (!data.empty()) {
            CubismMotion* motion = CubismMotion::Create(data.data(), static_cast<csmSizeInt>(data.size()));
            motion->SetLoop(false);
            _glitchManager->StartMotionPriority(motion, true, 3);

            _isGlitchLooping = true;
            _glitchGroup = group;
            _glitchNo = no;
            // OutputDebugStringA(("AppModel: Glitch Started from Cache\n"));
        }
        return;
    }

    // Not cached, load async
    
    // Resolve filename synchronously here to avoid race/data issues later
    std::string fileName = _modelSetting->GetMotionFileName(group, no);
    // Fallback
    if (fileName.empty()) {
        fileName = std::string(group) + ".motion3.json";
    }
    std::string path = _modelHomeDir + "/" + fileName;

    // Create a shared result object that both the job and the instance share
    auto result = std::make_shared<AsyncMotionResult>();
    result->group = group;
    result->no = no;

    {
        std::lock_guard<std::mutex> lock(_motionMutex);
        _pendingAsyncResult = result;
    }

    sf::jobsystem::JobSystem::Instance()->addJob([path, result]() {
        // Capture 'path' (value) and 'result' (shared_ptr)
        // Do NOT capture 'this'
        
        auto motionBytes = LoadFileAsBytes(path);

        if (!motionBytes.empty()) {
            result->data = std::move(motionBytes);
            result->ready.store(true, std::memory_order_release); // Mark as ready
        }
    }); 
}

// MATCH HEADER SIGNATURE: const ... & matrix
void AppModel::Draw(ID3D11Device* device, ID3D11DeviceContext* context, const Csm::CubismMatrix44& matrix) {
    if (!_myRenderer) return;

    Csm::CubismMatrix44 projection;
    // projection.Scale(1.0f, 1.0f); // Default
    
    // SDK API takes a non-const pointer for MultiplyByMatrix.
    // We cast here because the source matrix is not expected to be modified.
    projection.MultiplyByMatrix(const_cast<Csm::CubismMatrix44*>(&matrix));

    _myRenderer->SetMvpMatrix(&projection);

    OutputDebugStringA("AppModel::Draw - Calling _myRenderer->DrawModel()\n");
    _myRenderer->DrawModel();
    OutputDebugStringA("AppModel::Draw - Finished _myRenderer->DrawModel()\n");
}
