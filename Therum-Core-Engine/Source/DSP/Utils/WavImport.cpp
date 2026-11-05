// WAV import (see WavImport.h). Pure byte parsing over a memory buffer:
// no file I/O, no framework, no allocation beyond the decoded samples.
#include "WavImport.h"
#include <cmath>
#include <cstring>
#include <vector>

namespace
{

uint16_t readU16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
uint32_t readU32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
           | ((uint32_t)p[3] << 24);
}
int32_t readI32(const uint8_t* p) { return (int32_t)readU32(p); }

bool tagIs(const uint8_t* p, const char* tag)
{
    return std::memcmp(p, tag, 4) == 0;
}

// One decoded mono sample at `offset` in the data chunk. Returns false
// when the sample is out of range (truncated file).
bool decodeSample(const uint8_t* data, size_t offset, int bytesPerSample,
                  int format, float& out)
{
    switch (format)
    {
        case 1: // PCM integer
            if (bytesPerSample == 2)
            {
                const int16_t v = (int16_t)readU16(data + offset);
                out = (float)v / 32768.0f;
                return true;
            }
            if (bytesPerSample == 3)
            {
                int32_t v = (int32_t)data[offset]
                            | ((int32_t)data[offset + 1] << 8)
                            | ((int32_t)data[offset + 2] << 16);
                if (v & 0x800000)
                    v -= 0x1000000; // sign-extend the 24-bit value
                out = (float)v / 8388608.0f;
                return true;
            }
            if (bytesPerSample == 4)
            {
                out = (float)readI32(data + offset) / 2147483648.0f;
                return true;
            }
            return false;
        case 3: // IEEE float
            if (bytesPerSample == 4)
            {
                float v;
                std::memcpy(&v, data + offset, 4);
                out = v;
                return true;
            }
            return false;
        default:
            return false;
    }
}

} // namespace

WavImportResult importWav(WavetableBank& bank, const uint8_t* data,
                          size_t size)
{
    WavImportResult result;
    if (data == nullptr || size < 12 || !tagIs(data, "RIFF")
        || !tagIs(data + 8, "WAVE"))
    {
        result.error = "not a RIFF/WAVE file";
        return result;
    }

    // Scan the chunks: find fmt and data, skip anything else.
    const uint8_t* fmt = nullptr;
    size_t fmtSize = 0;
    const uint8_t* wavData = nullptr;
    size_t wavDataSize = 0;
    size_t pos = 12;
    while (pos + 8 <= size)
    {
        const uint32_t chunkSize = readU32(data + pos + 4);
        const size_t payload = pos + 8;
        if (payload + chunkSize > size)
        {
            result.error = "truncated chunk";
            return result;
        }
        if (tagIs(data + pos, "fmt "))
        {
            fmt = data + payload;
            fmtSize = chunkSize;
        }
        else if (tagIs(data + pos, "data"))
        {
            wavData = data + payload;
            wavDataSize = chunkSize;
        }
        pos = payload + chunkSize + (chunkSize & 1); // chunks are word-aligned
    }
    if (fmt == nullptr || fmtSize < 16 || wavData == nullptr)
    {
        result.error = "missing fmt or data chunk";
        return result;
    }

    int format = readU16(fmt);
    if (format == 0xFFFE && fmtSize >= 40)
    {
        // WAVE_FORMAT_EXTENSIBLE: the real format is the SubFormat
        // GUID's first word, at offset 24 of the fmt chunk.
        format = readU16(fmt + 24);
    }
    if (format != 1 && format != 3)
    {
        result.error = "unsupported format (only PCM and IEEE float)";
        return result;
    }
    const int channels = readU16(fmt + 2);
    const int bitsPerSample = readU16(fmt + 14);
    if (channels < 1)
    {
        result.error = "no channels";
        return result;
    }
    const int bytesPerSample = (bitsPerSample + 7) / 8;
    if ((format == 1 && bytesPerSample != 2 && bytesPerSample != 3
         && bytesPerSample != 4)
        || (format == 3 && bytesPerSample != 4))
    {
        result.error = "unsupported bit depth";
        return result;
    }
    const int frameBytes = bytesPerSample * channels;
    if (frameBytes <= 0 || wavDataSize % (size_t)frameBytes != 0)
    {
        result.error = "data chunk is not a whole number of frames";
        return result;
    }
    const size_t totalFrames = wavDataSize / (size_t)frameBytes;
    if (totalFrames == 0)
    {
        result.error = "no samples";
        return result;
    }

    // Decode to mono (average the channels), then cut into frames.
    std::vector<float> mono(totalFrames);
    for (size_t i = 0; i < totalFrames; ++i)
    {
        double sum = 0.0;
        for (int ch = 0; ch < channels; ++ch)
        {
            float v = 0.0f;
            if (!decodeSample(wavData, i * (size_t)frameBytes
                                          + (size_t)(ch * bytesPerSample),
                              bytesPerSample, format, v))
            {
                result.error = "undecodable sample";
                return result;
            }
            sum += (double)v;
        }
        mono[i] = (float)(sum / (double)channels);
    }
    result.totalSamples = (int)totalFrames;

    const int room = WavetableBank::kMaxFrames - bank.getFrameCount();
    if ((int)totalFrames < WavetableBank::kFrameSize)
    {
        // One cycle, shorter than a frame: resample it to 2048.
        if (room <= 0)
        {
            result.framesSkipped = 1;
        }
        else if (bank.addFrame(mono.data(), (int)totalFrames))
        {
            result.framesAdded = 1;
        }
        result.ok = true;
        return result;
    }
    const int fullFrames = (int)(totalFrames / (size_t)WavetableBank::kFrameSize);
    const int trailing = (int)(totalFrames % (size_t)WavetableBank::kFrameSize);
    if (trailing > 0)
    {
        result.framesDropped = 1;
        result.samplesDropped = trailing;
    }
    for (int f = 0; f < fullFrames; ++f)
    {
        if (f >= room)
        {
            result.framesSkipped = fullFrames - f;
            break;
        }
        if (bank.addFrame(mono.data() + (size_t)f * WavetableBank::kFrameSize,
                          WavetableBank::kFrameSize))
            ++result.framesAdded;
        else
        {
            result.framesSkipped = fullFrames - f;
            break;
        }
    }
    result.ok = true;
    return result;
}
