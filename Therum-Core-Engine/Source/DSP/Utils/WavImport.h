// WAV import for the wavetable bank (paper section 6, rank 2).
//
// Reads a WAV file from a memory buffer and adds its cycles as frames.
// Serum-format wavetables are WAV files whose samples are 2048-sample
// frames, so they import through the same path — there is no separate
// Serum parser, and the format needs no special casing.
//
// Accepted: RIFF/WAVE, fmt PCM (1) / IEEE float (3) / WAVE_FORMAT_
// EXTENSIBLE (0xFFFE, SubFormat 1 or 3), 16/24/32-bit integer and
// 32-bit float, mono or multi-channel (mixed to mono by averaging).
// Unknown chunks (LIST, fact, JUNK, ...) are skipped. The sample rate
// in the file is ignored: a frame is one cycle, played at the note
// frequency whatever the file was recorded at.
//
// Framing: the mono samples are cut into 2048-sample frames. A file
// shorter than one frame is resampled to 2048 as a single frame (one
// cycle). A longer file whose length is not a multiple of 2048 has its
// trailing partial frame dropped and reported. Frames beyond the
// bank's remaining capacity are skipped and reported. Import does NOT
// build mipmaps — call bank.buildMipmaps() after importing, as with
// addFrame.
#pragma once
#include "../Oscillators/WavetableBank.h"
#include <cstddef>
#include <cstdint>

struct WavImportResult
{
    bool ok = false;
    const char* error = nullptr; // short reason when !ok, else null
    int framesAdded = 0;
    int framesDropped = 0;  // trailing partial frame(s), not a full 2048
    int framesSkipped = 0;  // bank full — no room for these frames
    int samplesDropped = 0; // trailing samples that made the partial frame
    int totalSamples = 0;   // mono samples decoded from the file
};

WavImportResult importWav(WavetableBank& bank, const uint8_t* data,
                          size_t size);
