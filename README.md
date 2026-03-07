# NEWchumini

DirectX 11 / C++20 で動くリズムゲームプロジェクトです。  
Visual Studio 2022 のソリューション `chumini.sln` からビルドできます。

## 開発環境
- OS: Windows 10 / 11
- IDE: Visual Studio 2022
- Toolset: MSVC v143
- Windows SDK: 10.0 系
- 言語規格: C++20

## 依存ライブラリ
このリポジトリ内に以下を同梱しています（`chumini/` 配下）。
- ImGui (`imgui/`)
- Live2D Cubism SDK (`Live2D/`)
- Effekseer (`Effekseer/`)
- DirectXTK ヘッダ群 (`directxtk/include`)

`chumini.vcxproj` は相対パスで参照する設定になっています。

## ビルド手順
1. ルートの `chumini.sln` を Visual Studio 2022 で開く
2. 構成を選択（推奨: `Debug | x64`）
3. `chumini` プロジェクトを `Rebuild`

## 実行時に必要なフォルダ
実行時は `chumini/` 直下の以下フォルダが必要です。
- `Assets/`
- `Songs/`
- `Save/`
- `Live2D/`
- `Effekseer/`
- `imgui/`

シェーダはビルド時に `Assets/Shader/*.cso` として出力されます。

## ディレクトリ構成（主要部分）
```text
NEWchumini/
├─ chumini.sln
├─ README.md
└─ chumini/
   ├─ chumini.vcxproj
   ├─ src/
   │  ├─ app/       # ゲーム側ロジック
   │  ├─ sf/        # フレームワーク側
   │  └─ shader/    # HLSL
   ├─ Assets/
   ├─ Songs/
   ├─ Live2D/
   ├─ Effekseer/
   └─ imgui/
```

## よくあるハマりどころ
- `imgui/imgui.h` が見つからない:  
  `imgui/` フォルダが `chumini/` 配下にあるか確認
- Live2D のヘッダが見つからない:  
  `Live2D/Framework/src` と `Live2D/Core/include` の存在を確認
- ソース整理後に大量エラーが出る:  
  まず `Clean` → `Rebuild` を実行

## 提出・共有時の注意
GitHub には以下を含めてください。
- `*.sln`
- `*.vcxproj`
- `*.vcxproj.filters`
- （使っていれば）`*.props`, `*.targets`

以下は含めないでください。
- `.vs/`
- `Debug/`, `Release/`, `x64/`
- `*.user`, `*.suo`
