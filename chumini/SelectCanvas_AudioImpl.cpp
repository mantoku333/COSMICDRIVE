
    // ===== プレビュー再生 =====
    void SelectCanvas::PlayPreview() {
        // 前の曲を止める
        previewPlayer.Stop();
        previewResource = nullptr;

        if (songs.empty()) return;
        
        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        const auto& info = songs[idx];

        if (info.musicPath.empty()) return;

        // パスをShift-JISに変換
        std::string sjisPath = Utf8ToShiftJis(info.musicPath);

        // 新しいリソースを作成
        previewResource = sf::new_ref<sf::sound::SoundResource>();
        
        // ロード失敗したら再生しない
        if (FAILED(previewResource->LoadSound(sjisPath, true))) { // true = loop
             sf::debug::Debug::Log("Preview Load Failed: " + sjisPath);
             return;
        }

        previewPlayer.SetResource(previewResource);
        previewPlayer.SetVolume(1.0f); // 必要に応じて調整
        previewPlayer.Play();
    }

    void SelectCanvas::End() {
        previewPlayer.Stop();
        previewResource = nullptr;
        sf::ui::Canvas::End();
    }
