// Render harness: renders the wavetable oscillator to a 16-bit mono WAV
// so the ARC Audio Bench can measure it (paper section 7, the alias
// test). This is a test tool, not DSP: it links the framework-free DSP
// files directly, like test_core.
//
// Usage:
//   ./render_wavetable [--f0 997] [--sr 48000] [--seconds 3]
//                      [--out file.wav] [--mip|--no-mip]
//                      [--position 0..1] [--morph]
// Defaults: 997 Hz, 48000 Hz, 3 s, wavetable_997hz.wav, mip on,
// position 2/3 (the saw frame of the default bank), crossfade mode.
// The Bench's alias test takes the WAV and --f0; it skips the first
// 0.5 s, so 3 s gives it a clean 2.5 s to measure.
#include "../Source/DSP/Oscillators/WavetableBank.h"
#include "../Source/DSP/Oscillators/WavetableOscillator.h"
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

static void putU16(std::vector<uint8_t>& out, uint16_t v)
{
    out.push_back((uint8_t)(v & 0xFF));
    out.push_back((uint8_t)((v >> 8) & 0xFF));
}
static void putU32(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back((uint8_t)(v & 0xFF));
    out.push_back((uint8_t)((v >> 8) & 0xFF));
    out.push_back((uint8_t)((v >> 16) & 0xFF));
    out.push_back((uint8_t)((v >> 24) & 0xFF));
}
static void putTag(std::vector<uint8_t>& out, const char* t)
{
    out.insert(out.end(), t, t + 4);
}

static bool writeWav(const char* path, const std::vector<float>& samples,
                     int sampleRate)
{
    std::vector<uint8_t> wav;
    const uint32_t dataBytes = (uint32_t)samples.size() * 2;
    putTag(wav, "RIFF");
    putU32(wav, 36 + dataBytes);
    putTag(wav, "WAVE");
    putTag(wav, "fmt ");
    putU32(wav, 16);
    putU16(wav, 1); // PCM
    putU16(wav, 1); // mono
    putU32(wav, (uint32_t)sampleRate);
    putU32(wav, (uint32_t)(sampleRate * 2));
    putU16(wav, 2); // block align
    putU16(wav, 16);
    putTag(wav, "data");
    putU32(wav, dataBytes);
    for (float s : samples)
    {
        float clamped = s < -1.0f ? -1.0f : (s > 1.0f ? 1.0f : s);
        const int16_t v = (int16_t)std::lround(clamped * 32767.0f);
        putU16(wav, (uint16_t)v);
    }
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr)
        return false;
    const bool ok = std::fwrite(wav.data(), 1, wav.size(), f) == wav.size();
    std::fclose(f);
    return ok;
}

int main(int argc, char** argv)
{
    double f0 = 997.0;
    double sr = 48000.0;
    double seconds = 3.0;
    std::string out = "wavetable_997hz.wav";
    bool mip = true;
    float position = 2.0f / 3.0f; // saw frame of the default bank
    bool morph = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        auto value = [&](const char* name) -> const char*
        {
            if (i + 1 >= argc)
            {
                std::fprintf(stderr, "missing value for %s\n", name);
                return nullptr;
            }
            return argv[++i];
        };
        if (arg == "--f0")
        {
            const char* v = value("--f0");
            if (v == nullptr) return 1;
            f0 = std::atof(v);
        }
        else if (arg == "--sr")
        {
            const char* v = value("--sr");
            if (v == nullptr) return 1;
            sr = std::atof(v);
        }
        else if (arg == "--seconds")
        {
            const char* v = value("--seconds");
            if (v == nullptr) return 1;
            seconds = std::atof(v);
        }
        else if (arg == "--out")
        {
            const char* v = value("--out");
            if (v == nullptr) return 1;
            out = v;
        }
        else if (arg == "--mip")
            mip = true;
        else if (arg == "--no-mip")
            mip = false;
        else if (arg == "--position")
        {
            const char* v = value("--position");
            if (v == nullptr) return 1;
            position = (float)std::atof(v);
        }
        else if (arg == "--morph")
            morph = true;
        else
        {
            std::fprintf(stderr, "unknown argument: %s\n", arg.c_str());
            return 1;
        }
    }

    WavetableBank bank;
    bank.buildDefaultBank();
    if (mip)
        bank.buildMipmaps();

    WavetableOscillator osc;
    osc.prepare(sr);
    osc.setBank(&bank);
    osc.setFrequency((float)f0);
    osc.setPosition(position);
    if (morph)
        osc.setMorphMode(WavetableOscillator::MorphMode::Spectral);

    const int total = (int)(seconds * sr);
    std::vector<float> samples((size_t)total);
    for (int i = 0; i < total; ++i)
        samples[(size_t)i] = osc.process();

    if (!writeWav(out.c_str(), samples, (int)sr))
    {
        std::fprintf(stderr, "cannot write %s\n", out.c_str());
        return 1;
    }
    std::printf("rendered %d samples (%.1f s at %.0f Hz, f0 %.1f Hz, "
                "position %.3f, mip %s, morph %s) -> %s\n",
                total, seconds, sr, f0, position, mip ? "on" : "off",
                morph ? "spectral" : "crossfade", out.c_str());
    return 0;
}
