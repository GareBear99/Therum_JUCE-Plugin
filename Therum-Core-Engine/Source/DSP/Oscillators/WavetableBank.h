#pragma once
// Wavetable bank: the table store for the wavetable engine (paper section 6,
// rank 2). Holds up to 256 frames of 2048 samples each, matching Serum 2's
// table format so Serum-format tables can be imported.
//
// The bank stores only the base frames on disk / in memory. Mipmaps are
// generated in RAM at load (see buildMipmaps) and live beside the frames.
// The oscillator reads frames through the bank; it never owns table data.
#include <vector>

class WavetableBank
{
public:
    static constexpr int kFrameSize = 2048;
    static constexpr int kMaxFrames = 256;

    // One sine frame (the original scaffold behaviour). Kept for
    // compatibility: the voice manager used to build exactly this.
    // `size` is resampled to kFrameSize when it differs.
    void buildDefaultSineTable(int size = kFrameSize);

    // Four classic frames: sine, triangle, saw, square. This is the
    // default bank the voice uses: position 0 is the sine, so existing
    // behaviour is unchanged, and the other waves are there for the
    // position parameter to reach later.
    void buildDefaultBank();

    // Add one frame. `samples` is a single cycle of `length` samples; it
    // is resampled to kFrameSize when the length differs. Returns false
    // when the bank is full (kMaxFrames) or the input is empty. Adding a
    // frame invalidates any mipmaps: call buildMipmaps() again.
    bool addFrame(const float* samples, int length);

    int getFrameCount() const { return (int)frames.size(); }
    // Frame i (0-based). Out of range returns an empty table.
    const std::vector<float>& getFrame(int index) const;
    // Compatibility: frame 0 (or an empty table when the bank is empty).
    const std::vector<float>& getTable() const { return getFrame(0); }
    bool isReady() const { return !frames.empty(); }

    // Mipmaps (generated at load, paper section 4.1). See buildMipmaps.
    struct MipLevel
    {
        int maxHarmonic = 0;              // highest harmonic this level keeps
        std::vector<float> table;         // length is a power of two, >= 2*maxHarmonic
    };
    struct FrameMips
    {
        std::vector<MipLevel> levels;     // ordered by falling maxHarmonic
    };
    // Generate the mip ladder for every frame (FFT -> zero bins above the
    // level cap -> IFFT -> decimate to the level length). Levels are one
    // per 3 semitones, crossfaded at render time. Rebuilding replaces any
    // previous mipmaps. Returns false when the bank is empty.
    bool buildMipmaps();
    bool hasMipmaps() const { return !frameMips.empty(); }
    // Mip ladder for frame i (empty when no mipmaps were built).
    const FrameMips& getFrameMips(int index) const;
    // Number of mip levels (0 when no mipmaps were built).
    int getMipLevelCount() const;

private:
    static void resampleToFrame(const float* samples, int length,
                                std::vector<float>& out);

    std::vector<std::vector<float>> frames;
    std::vector<FrameMips> frameMips;
    static const std::vector<float> kEmptyTable;
    static const FrameMips kEmptyMips;
};
