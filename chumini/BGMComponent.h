#pragma once
#include "Component.h"
#include "SafePtr.h"
#include "SoundPlayer.h"
#include "SoundResource.h"
#include <string>

namespace app::test {

    class BGMComponent : public sf::Component {
    public:
        void Begin() override;
       // void Update(const sf::command::ICommand&) override {}

        // API
        void SetPath(const std::string& p);
        void Play();
        void Stop();

        void SetVolume(float v);
        void SetLoop(bool v) { loop = v; }

    private:
        void ensurePlayer();

    private:
        sf::SafePtr<sf::sound::SoundPlayer> player; // “à•”‚Å—˜—p
        sf::sound::SoundResource resource;
        std::string path;
        bool loaded = false;
        bool loop = true;
        float volume = 1.0f;
    };

}
