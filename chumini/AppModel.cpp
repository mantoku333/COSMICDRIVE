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

void AppModel::LoadAssets(ID3D11Device* device, const std::string& dir, const std::string& fileName)
{
    _modelHomeDir = dir;

    // 1. .model3.json (設定ファイル) の読み込み
    std::string filePath = dir + fileName;
    csmSizeType size = 0;
    csmByte* buffer = CreateBuffer(filePath.c_str(), &size);
    if (!buffer) {
        return;
    }

    CubismModelSettingJson* modelSetting = new CubismModelSettingJson(buffer, size);
    free(buffer);

    // 2. .moc3 (モデル本体) の読み込み
    std::string moc3Path = dir + modelSetting->GetModelFileName();
    buffer = CreateBuffer(moc3Path.c_str(), &size);

    LoadModel(buffer, size);
    free(buffer);

    // 3. レンダラー作成と初期化 【ここを修正】
    _renderer = static_cast<CubismRenderer_D3D11*>(CubismRenderer_D3D11::Create());

    // ★修正1: Initializeには Device ではなく GetModel() を渡します
    _renderer->Initialize(GetModel());

    // ★修正2: BindModel は存在しないので削除しました（Initializeで紐づけ済みです）

    // 4. テクスチャ読み込み
    int textureCount = modelSetting->GetTextureCount();
    for (int i = 0; i < textureCount; i++)
    {
        std::string texturePath = dir + modelSetting->GetTextureFileName(i);

        sf::Texture* texture = new sf::Texture();
        if (texture->LoadTextureFromFile(texturePath))
        {
            _loadedTextures.push_back(texture);

            // テクスチャをバインド
            _renderer->BindTexture(i, texture->srv);
        }
        else
        {
            delete texture;
        }
    }

    // 5. サイズ保存
    GetModel()->SaveParameters();
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

    // 行列計算
    Csm::CubismMatrix44 viewMatrix = matrix;
    projection.MultiplyByMatrix(&viewMatrix);

    _renderer->SetMvpMatrix(&projection);


    // モデル描画
    _renderer->DrawModel();


}

Csm::csmInt32 AppModel::GetTextureDirectoryIndex(const Csm::csmString& path)
{
    return -1;
}