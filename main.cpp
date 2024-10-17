#include <soloud_engine.hpp>
#include <soloud_wav.hpp>
#include <soloud_bus.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

int main()
{
    auto file_stream = std::ifstream{"/Users/cem/Desktop/sounds/player_fall.wav", std::ios::ate | std::ios::binary};
    auto file_size = file_stream.tellg();
    file_stream.seekg(0, std::ios::beg);
    auto file_data = std::make_unique<uint8_t[]>(file_size);
    file_stream.read(reinterpret_cast<char*>(file_data.get()), file_size);

    auto file_data_span = std::span<const std::byte>(reinterpret_cast<const std::byte*>(file_data.get()), size_t(file_size));

    auto engine = SoLoud::Engine{};

    auto wav = SoLoud::Wav{file_data_span};
    wav.should_loop=true;

    engine.play(wav);

    while(engine.getActiveVoiceCount()>0)
    {
        std::this_thread::sleep_for(0.1s);
    }

    return 0;
}