#pragma once

#include "AudioFile.h"
#include "IPlugUtilities.h"
#include "IPlug_include_in_plug_hdr.h"
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <algorithm>

using std::vector;

const static int MAX_SHAPE_SAMPLES = 4800; // Samples in 1 period (4800 = 20Hz @ 96k or 10Hz @ 48k)
const static double HALF_SHAPE_SAMPLES = MAX_SHAPE_SAMPLES / 2;
const static double SHAPE_SAMPLE_SCALE = 1.0 / (MAX_SHAPE_SAMPLES);

const int kNumPresets = 1;

enum EParams
{
    kGainI = 0,
    kGainO = 1,
    kEnable = 2,
    kFade = 3,
    kOffset = 4,
    kRotation = 5,
    kOptimize = 6,
    kCenter = 7,
    kBank = 8,
    kNumParams
};

using namespace iplug;
using namespace igraphics;
namespace fs = std::filesystem;

struct shapeTarget
{
    float samplesL[MAX_SHAPE_SAMPLES] = {0.};
    float samplesR[MAX_SHAPE_SAMPLES] = {0.};
    std::string name = "((init))";
    bool init = true;
};

struct shapeTargetV2
{
    vector<float> left;
    vector<float> right;
    std::string name = "((init))";
    int nBanks = 0;
    bool init = true;
};

template <class type>
type lerp(type a, type b, double t)
{
    return a + (b - a) * t;
}

class WavShaper final : public Plugin
{
  public:
    WavShaper(const InstanceInfo& info);

    bool SerializeState(IByteChunk& chunk) const override;
    int UnserializeState(const IByteChunk& chunk, int startPos) override;

    void DoPreset(const char* name);

    void loadShape(const char* name);

#if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
  private:
    double multiplier = 0.0;
    sample last;
    int lastBank;
    shapeTargetV2 targets;
    sample doShaping(sample in, bool left);
    int findInSampleSet(sample in);
    void updateUI();
    void loadShape(int num, bool reinit);
    void loadShape();
    void copyToPluginDir(fs::path src);
    void optimizeShape();
};
