#pragma once
#include <Windows.h>
#include <fstream> // ★追加: ファイル読み込み用
#include <CubismFramework.hpp>
#include "AppAllocator.h"

// ★追加: Live2Dのレンダラーとシェーダー関連
#include <Rendering/D3D11/CubismRenderer_D3D11.hpp>
#include <Rendering/D3D11/CubismShader_D3D11.hpp>

using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::Rendering; // ★追加

// Live2Dの管理クラス
class Live2DManager {
public:
    // シングルトン
    static Live2DManager* GetInstance() {
        static Live2DManager instance;
        return &instance;
    }

    // ★追加1: シェーダー読み込み用関数 (SDKから自動で呼ばれます)
    static csmByte* LoadShaderFromFile(const csmChar* fileName, csmSizeInt* outSize)
    {
        std::ifstream file(fileName, std::ios::binary);
        if (!file.is_open()) {
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        *outSize = static_cast<csmSizeInt>(file.tellg());
        file.seekg(0, std::ios::beg);

        csmByte* buffer = static_cast<csmByte*>(malloc(*outSize));
        file.read(reinterpret_cast<char*>(buffer), *outSize);
        return buffer;
    }

    // ★追加2: シェーダーメモリ解放用関数
    static void ReleaseShaderData(csmByte* buffer)
    {
        free(buffer);
    }

    // 初期化
    void Initialize() {
        // 二重初期化防止
        if (_isInitialized) return;

        // 1. メモリ確保機能のセットアップ
        _allocator = new LAppAllocator();

        // 2. オプション設定
        Csm::CubismFramework::Option option;
        option.LogFunction = Live2DManager::PrintLog;
        option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;

        // 3. Live2Dの開始
        Csm::CubismFramework::StartUp(_allocator, &option);
        Csm::CubismFramework::Initialize();


        _isInitialized = true;
    }

    // 終了処理
    void Release() {
        Csm::CubismFramework::Dispose();
        delete _allocator;
    }

    // ログ出力
    static void PrintLog(const char* message) {
        OutputDebugStringA(message);
    }

private:
    LAppAllocator* _allocator = nullptr;
    bool _isInitialized = false; // ★追加: 初期化フラグ

    Live2DManager() {}
};