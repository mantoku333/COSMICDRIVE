#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>

namespace app::test {

    /// <summary>
    /// インスタンシング用のノーツデータ
    /// 各ノーツのワールド行列と色を保持
    /// </summary>
    struct NoteInstanceData {
        DirectX::XMFLOAT4X4 world;  ///< ワールド変換行列
        DirectX::XMFLOAT4   color;  ///< 色
    };

    /// <summary>
    /// ノーツのインスタンシング描画を担当するクラス
    /// DirectX11リソースの管理とバッチ描画を行う
    /// </summary>
    class NoteInstanceRenderer {
    public:
        NoteInstanceRenderer();
        ~NoteInstanceRenderer();
        
        // コピー禁止 (unique_ptrのため)
        NoteInstanceRenderer(const NoteInstanceRenderer&) = delete;
        NoteInstanceRenderer& operator=(const NoteInstanceRenderer&) = delete;
        NoteInstanceRenderer(NoteInstanceRenderer&&) noexcept;
        NoteInstanceRenderer& operator=(NoteInstanceRenderer&&) noexcept;

        /// <summary>
        /// DirectXリソースを初期化
        /// </summary>
        void Init();

        /// <summary>
        /// インスタンスバッファを更新
        /// </summary>
        void UpdateBuffer(const std::vector<NoteInstanceData>& instances);

        /// <summary>
        /// インスタンス描画を実行
        /// </summary>
        void Draw(size_t instanceCount);

        /// <summary>
        /// バッファ更新と描画を一括実行
        /// </summary>
        void UpdateAndDraw(const std::vector<NoteInstanceData>& instances);

        /// <summary>
        /// リソース解放
        /// </summary>
        void Release();

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

}
