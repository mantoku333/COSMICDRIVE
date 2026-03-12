#pragma once
#include <string>
#include <vector>
#include "SongInfo.h"

namespace app::test {

    /// <summary>
    /// ジャンル情報
    /// ジャンル名と所属楽曲のリストを保持
    /// </summary>
    struct Genre {
        std::string name;                   ///< ジャンル名（フォルダ名）
        std::vector<SongInfo> songs;        ///< このジャンルに属する楽曲リスト
    };

    /// <summary>
    /// 楽曲リスト管理サービス
    /// 
    /// ファイルシステムから楽曲をスキャンし、
    /// ジャンル別に分類して管理する
    /// </summary>
    class SongListService {
    public:
        SongListService() = default;

        /// <summary>
        /// 楽曲データのスキャンとロード
        /// </summary>
        /// <param name="rootPath">楽曲ルートディレクトリ（例: "Songs"）</param>
        void ScanSongs(const std::string& rootPath = "Songs");

        /// <summary>
        /// 全ジャンルリストを取得
        /// </summary>
        const std::vector<Genre>& GetAllGenres() const { return allGenres; }

        /// <summary>
        /// 現在のジャンルを取得
        /// </summary>
        const Genre& GetCurrentGenre() const;

        /// <summary>
        /// 現在のジャンルインデックスを取得
        /// </summary>
        int GetCurrentGenreIndex() const { return currentGenreIndex; }

        /// <summary>
        /// ジャンル数を取得
        /// </summary>
        size_t GetGenreCount() const { return allGenres.size(); }

        /// <summary>
        /// ジャンルを変更
        /// </summary>
        /// <param name="index">新しいジャンルインデックス</param>
        /// <returns>変更後の楽曲リスト</returns>
        const std::vector<SongInfo>& ChangeGenre(int index);

        /// <summary>
        /// 次のジャンルに移動
        /// </summary>
        /// <returns>変更後の楽曲リスト</returns>
        const std::vector<SongInfo>& NextGenre();

        /// <summary>
        /// 前のジャンルに移動
        /// </summary>
        /// <returns>変更後の楽曲リスト</returns>
        const std::vector<SongInfo>& PreviousGenre();

        /// <summary>
        /// 現在のジャンルの楽曲リストを取得
        /// </summary>
        const std::vector<SongInfo>& GetCurrentSongs() const;

    private:
        std::vector<Genre> allGenres;       ///< 全ジャンルリスト
        int currentGenreIndex = 0;          ///< 現在選択中のジャンルインデックス
    };

}
