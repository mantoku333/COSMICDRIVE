#pragma once
#include <Windows.h>
#include <CubismFramework.hpp>
#include "AppAllocator.h"

// Live2Dの管理クラス
class Live2DManager {
public:
    // シングルトン（どこからでも呼べるようにする）
    static Live2DManager* GetInstance() {
        static Live2DManager instance;
        return &instance;
    }

    // 初期化（ゲーム起動時に1回呼ぶ）
    void Initialize() {
        // 1. メモリ確保機能のセットアップ
        _allocator = new LAppAllocator();

        // 2. オプション設定（ログ出力レベルなど）
        Csm::CubismFramework::Option option;
        option.LogFunction = Live2DManager::PrintLog;
        option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;

        // 3. Live2Dの開始
        Csm::CubismFramework::StartUp(_allocator, &option);
        Csm::CubismFramework::Initialize();
    }

    // 終了処理（ゲーム終了時に呼ぶ）
    void Release() {
        Csm::CubismFramework::Dispose();
        delete _allocator;
    }

    // ログ出力用の関数（デバッグ用）
    static void PrintLog(const char* message) {
        // Visual Studioの出力ウィンドウに出す
        OutputDebugStringA(message);
    }

private:
    LAppAllocator* _allocator = nullptr; // アロケータの保持

    // コンストラクタを隠す（シングルトン用）
    Live2DManager() {}
};