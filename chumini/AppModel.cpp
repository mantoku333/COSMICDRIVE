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

AppModel::AppModel() : _modelSetting(nullptr), _myRenderer(nullptr) {
}

AppModel::~AppModel() {
    if (_modelSetting) {
        delete _modelSetting;
        _modelSetting = nullptr;
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
    // Basic update:
    GetModel()->Update();
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
