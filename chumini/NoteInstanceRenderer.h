#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

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

        /// <summary>
        /// DirectXリソースを初期化
        /// - インスタンスバッファ（動的、最大2048ノーツ）
        /// - キューブメッシュ（頂点・インデックス）
        /// - シェーダーと入力レイアウト
        /// </summary>
        void Init();

        /// <summary>
        /// インスタンスバッファを更新
        /// </summary>
        /// <param name="instances">描画するノーツのインスタンスデータ</param>
        void UpdateBuffer(const std::vector<NoteInstanceData>& instances);

        /// <summary>
        /// インスタンス描画を実行
        /// 事前にUpdateBufferを呼んでおく必要がある
        /// </summary>
        /// <param name="instanceCount">描画するインスタンス数</param>
        void Draw(size_t instanceCount);

        /// <summary>
        /// バッファ更新と描画を一括実行
        /// </summary>
        /// <param name="instances">描画するノーツのインスタンスデータ</param>
        void UpdateAndDraw(const std::vector<NoteInstanceData>& instances);

        /// <summary>
        /// DirectXリソースを解放
        /// </summary>
        void Release();

    private:
        static constexpr size_t MAX_INSTANCES = 2048;

        ID3D11Buffer* m_instanceBuffer = nullptr;
        ID3D11VertexShader* m_vs = nullptr;
        ID3D11InputLayout* m_layout = nullptr;
        ID3D11Buffer* m_cubeVB = nullptr;
        ID3D11Buffer* m_cubeIB = nullptr;
        int m_cubeIndexCount = 0;
    };

}
