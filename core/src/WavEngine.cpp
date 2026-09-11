#include "WavEngine.hpp"

#include <cmath>
#include <fstream>

void WavEngine::writeLe16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void WavEngine::writeLe32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

std::vector<int16_t> WavEngine::synthesizePcm(const std::vector<NoteEvent>& sequence,
                                              uint32_t sampleRate) {
    std::vector<int16_t> pcm;
    constexpr double twoPi = 6.28318530717958647692;
    constexpr int16_t maxAmp = 18000;

    for (const auto& event : sequence) {
        size_t noteSamples =
            static_cast<size_t>((static_cast<uint64_t>(event.duration_ms) * sampleRate) / 1000);
        if (noteSamples == 0) {
            continue;
        }

        if (event.is_rest || event.frequency == 0) {
            pcm.insert(pcm.end(), noteSamples, 0);
            continue;
        }

        double freq = static_cast<double>(event.frequency);
        size_t attackSamples = (sampleRate * 8) / 1000;   // 8ms attack
        size_t releaseSamples = (sampleRate * 18) / 1000; // 18ms release

        if (attackSamples + releaseSamples > noteSamples) {
            attackSamples = noteSamples / 4;
            releaseSamples = noteSamples / 4;
        }

        double phase = 0.0;
        double phaseStep = (twoPi * freq) / static_cast<double>(sampleRate);

        for (size_t i = 0; i < noteSamples; ++i) {
            double envelope = 1.0;
            if (i < attackSamples) {
                envelope = static_cast<double>(i) / static_cast<double>(attackSamples);
            } else if (i >= noteSamples - releaseSamples) {
                envelope =
                    static_cast<double>(noteSamples - 1 - i) / static_cast<double>(releaseSamples);
            }

            // Warm generative tone: 80% fundamental sine + 20% second harmonic
            double val = 0.80 * std::sin(phase) + 0.20 * std::sin(2.0 * phase);
            int16_t sample = static_cast<int16_t>(val * maxAmp * envelope);
            pcm.push_back(sample);

            phase += phaseStep;
            if (phase >= twoPi) {
                phase -= twoPi;
            }
        }
    }

    return pcm;
}

std::vector<uint8_t> WavEngine::generateWavData(const std::vector<NoteEvent>& sequence,
                                                uint32_t sampleRate) {
    auto pcm = synthesizePcm(sequence, sampleRate);
    uint32_t dataBytes = static_cast<uint32_t>(pcm.size() * sizeof(int16_t));
    uint32_t riffChunkSize = 36 + dataBytes;

    std::vector<uint8_t> wav;
    wav.reserve(44 + dataBytes);

    // RIFF header
    wav.push_back('R');
    wav.push_back('I');
    wav.push_back('F');
    wav.push_back('F');
    writeLe32(wav, riffChunkSize);
    wav.push_back('W');
    wav.push_back('A');
    wav.push_back('V');
    wav.push_back('E');

    // fmt subchunk
    wav.push_back('f');
    wav.push_back('m');
    wav.push_back('t');
    wav.push_back(' ');
    writeLe32(wav, 16);             // Subchunk1Size = 16 for PCM
    writeLe16(wav, 1);              // AudioFormat = 1 (PCM)
    writeLe16(wav, 1);              // NumChannels = 1 (Mono)
    writeLe32(wav, sampleRate);     // SampleRate
    writeLe32(wav, sampleRate * 2); // ByteRate = SampleRate * NumChannels * BitsPerSample/8
    writeLe16(wav, 2);              // BlockAlign = NumChannels * BitsPerSample/8
    writeLe16(wav, 16);             // BitsPerSample = 16

    // data subchunk
    wav.push_back('d');
    wav.push_back('a');
    wav.push_back('t');
    wav.push_back('a');
    writeLe32(wav, dataBytes);

    // Sample data
    const uint8_t* rawBytes = reinterpret_cast<const uint8_t*>(pcm.data());
    wav.insert(wav.end(), rawBytes, rawBytes + dataBytes);

    return wav;
}

bool WavEngine::exportToFile(const std::string& filepath, const std::vector<NoteEvent>& sequence,
                             uint32_t sampleRate) {
    auto data = generateWavData(sequence, sampleRate);
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    return file.good();
}
