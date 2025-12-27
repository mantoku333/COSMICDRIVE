#include "AppModel.h" // インクルード名も変更
#include <fstream>
#include <vector>

#include <CubismModelSettingJson.hpp>
#include <Motion/CubismMotion.hpp>
#include <Physics/CubismPhysics.hpp>
#include <CubismDefaultParameterId.hpp>

using namespace std;
using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::DefaultParameterId;

namespace {
    csmByte* CreateBuffer(const char* path, csmSizeType* outSize)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return nullptr;

        file.seekg(0, std::ios::end);
        *outSize = static_cast<csmSizeType>(file.tellg());
        file.seekg(0, std::ios::beg);

        csmByte* buffer = static_cast<csmByte*>(malloc(*outSize));
        file.read(reinterpret_cast<char*>(buffer), *outSize);
        return buffer;
    }
}

// コンストラクタ
AppModel::AppModel() : _renderer(nullptr)
{
}

// デストラクタ
AppModel::~AppModel()
{
    if (_renderer) {
        CubismRenderer::Delete(_renderer);
        _renderer = nullptr;
    }

    for (auto texture : _loadedTextures) {
        delete texture;
    }
    _loadedTextures.clear();
}

void AppModel::LoadAssets(const std::string& dir, const std::string& fileName)
{
    _modelHomeDir = dir;
    std::string filePath = dir + fileName;

    csmSizeType size = 0;
    csmByte* buffer = CreateBuffer(filePath.c_str(), &size);
    if (!buffer) {
        return;
    }

    CubismModelSettingJson* modelSetting = new CubismModelSettingJson(buffer, size);
    free(buffer);

    std::string moc3Path = dir + modelSetting->GetModelFileName();
    buffer = CreateBuffer(moc3Path.c_str(), &size);

    LoadModel(buffer, size);
    free(buffer);

    // ★修正箇所: static_cast を追加
    _renderer = static_cast<CubismRenderer_D3D11*>(CubismRenderer_D3D11::Create());

    _renderer->Initialize(GetModel());

    // テクスチャ読み込み
    int textureCount = modelSetting->GetTextureCount();
    for (int i = 0; i < textureCount; i++)
    {
        std::string texturePath = dir + modelSetting->GetTextureFileName(i);

        sf::Texture* texture = new sf::Texture();
        if (texture->LoadTextureFromFile(texturePath))
        {
            _loadedTextures.push_back(texture);
            _renderer->BindTexture(i, texture->srv);
        }
        else
        {
            delete texture;
        }
    }

    delete modelSetting;
}

void AppModel::Update()
{
    if (GetModel() != nullptr)
    {
        GetModel()->Update();
    }
}

void AppModel::Draw(ID3D11Device* device, ID3D11DeviceContext* context, const Csm::CubismMatrix44& matrix)
{
    if (GetModel() == nullptr) return;

    Csm::CubismMatrix44 projection;
    Csm::CubismMatrix44 viewMatrix = matrix;
    projection.MultiplyByMatrix(&viewMatrix);

    _renderer->SetMvpMatrix(&projection);
    _renderer->DrawModel();
}

Csm::csmInt32 AppModel::GetTextureDirectoryIndex(const Csm::csmString& path)
{
    return -1;
}