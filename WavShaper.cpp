#include "WavShaper.h"
#include "IControls.h"
#include "IPlug_include_in_plug_src.h"

WavShaper::WavShaper(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    GetParam(kGainI)->InitDouble("Input Gain", 100., 0., 200.0, 0.02, "%");
    GetParam(kGainO)->InitDouble("Output Gain", 100., 0., 200.0, 0.02, "%");
    GetParam(kEnable)->InitBool("Enable", false, "Enable", 0, "", " ", " ");
    GetParam(kFade)->InitDouble("Shaping", 100., 0., 100.0, 0.01, "%");
    GetParam(kOffset)->InitDouble("Offset", 0., -1., 1.0, 0.01, "");

    loadShape();

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
    mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };

    mLayoutFunc = [&](IGraphics* pGraphics) {
        pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
        pGraphics->AttachPanelBackground(COLOR_GRAY);
        pGraphics->LoadFont("Hack-Regular", HACK_FN);
        pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

        const IVStyle style{
          true, // Show label
          true, // Show value
          {
            DEFAULT_BGCOLOR,          // Background
            DEFAULT_FGCOLOR,          // Foreground
            DEFAULT_PRCOLOR,          // Pressed
            COLOR_BLACK,              // Frame
            DEFAULT_HLCOLOR,          // Highlight
            DEFAULT_SHCOLOR,          // Shadow
            COLOR_BLACK,              // Extra 1
            DEFAULT_X2COLOR,          // Extra 2
            DEFAULT_X3COLOR           // Extra 3
          },                          // Colors
          IText(12.f, "Hack-Regular") // Label text
        };

        const IRECT b = pGraphics->GetBounds();
        pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "WavShaper", IText(50)));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(120, 100).GetVShifted(-200).GetHShifted(-80), kGainI));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(120, 100).GetVShifted(-200).GetHShifted(80), kGainO));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(200).GetHShifted(-80), kFade));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(200).GetHShifted(80), kOffset));
        pGraphics->AttachControl(new IVSlideSwitchControl(b.GetCentredInside(70, 50).GetVShifted(-100), kEnable));

        auto promptFunction = [this, pGraphics](IControl* pCaller) {
            WDL_String fileName;
            WDL_String path;
            fs::path full;
            pGraphics->PromptForFile(fileName, path, EFileAction::Open, "wav");
            full = fileName.Get();

            if (fileName.GetLength())
            {
                loadShape(fileName.Get());
            }
        };

        pGraphics->AttachControl(new IVButtonControl(b.GetCentredInside(120).SubRectVertical(4, 1).GetVShifted(100), SplashClickActionFunc, "Choose Shape...", style))
          ->SetAnimationEndActionFunction(promptFunction);
    };
#endif
}

#if IPLUG_DSP
void WavShaper::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    const double gainI = GetParam(kGainI)->Value() / 100.;
    const double gainO = GetParam(kGainO)->Value() / 100.;
    const double fade = GetParam(kFade)->Value() / 100;
    const double offset = GetParam(kOffset)->Value();
    const bool on = GetParam(kEnable)->Value();
    const int nChans = NOutChansConnected();

    for (int s = 0; s < nFrames; s++)
    {
        sample samp = offset + (inputs[0][s] + inputs[1][s]) * gainI / 2;
        if (samp == last)
        {
            multiplier = (multiplier > 0) ? (multiplier - 0.0006) : 0;
        }
        else
        {
            multiplier = (multiplier < 1) ? (multiplier + 0.002) : 1;
            last = samp;
        }
        while (samp >= 1)
            samp -= 2.;
        while (samp < -1)
            samp += 2.;
        samp = samp / 1.00001;
        sample sampL = doShaping(samp, true) * multiplier;
        sampL = lerp<sample>(inputs[0][s], sampL, fade);
        sample sampR = doShaping(samp, false) * multiplier;
        sampR = lerp<sample>(inputs[1][s], sampR, fade);
        outputs[0][s] = on ? (sampL * gainO) : inputs[0][s];
        outputs[1][s] = on ? (sampR * gainO) : inputs[1][s];
    }
}
#endif

sample WavShaper::doShaping(sample in, bool left)
{
    short int index = findInSampleSet(in) - 1;
    double t = (in + 1) - (index * SHAPE_SAMPLE_SCALE);
    sample out = lerp<sample>((left ? tgt.samplesL[index] : tgt.samplesR[index]), (left ? tgt.samplesL[index + 1] : tgt.samplesR[index + 1]), t);
    return out;
}

short int WavShaper::findInSampleSet(sample in)
{
    short int i = 1 + (short int)((in + 1) * (HALF_SHAPE_SAMPLES - 1));
    return i;
}

void WavShaper::loadShape(const char* name)
{
    fs::path path;
    path = name;
    copyToPluginDir(path);
    tgt.name = path.filename().generic_string();
    loadShape();
}

void WavShaper::loadShape()
{
    if (tgt.name != "__init__")
    {
        AudioFile<sample> f;
        char* p;
        size_t len;
        _dupenv_s(&p, &len, "APPDATA");
        fs::path target = p;
        target = target / "DJ_Level_3" / "WavShaper" / tgt.name;
        try
        {
            f.load(target.generic_string());
            int nSamps = f.getNumSamplesPerChannel();
            for (int i = 0; i < MAX_SHAPE_SAMPLES && i < nSamps; i++)
            {
                tgt.samplesL[i] = f.samples[0][i];
                tgt.samplesR[i] = f.samples[1][i];
            }
        }
        catch (std::exception& e)
        {
            for (int i = 0; i < MAX_SHAPE_SAMPLES; i++)
            {
                tgt.samplesL[i] = 0;
                tgt.samplesR[i] = 0;
            }
        }
    }
    else
    {
        for (int i = 0; i < MAX_SHAPE_SAMPLES; i++)
        {
            tgt.samplesL[i] = 0;
            tgt.samplesR[i] = 0;
        }
    }
}

bool WavShaper::SerializeState(IByteChunk& chunk) const
{
    // serialize the multislider state state before serializing the regular params
    chunk.PutStr(tgt.name.c_str());

    return SerializeParams(chunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int WavShaper::UnserializeState(const IByteChunk& chunk, int startPos)
{
    WDL_String name;

    // unserialize the steps state before unserializing the regular params
    startPos = chunk.GetStr(name, startPos);
    tgt.name = std::string(name.Get());

    // If UI exists
    updateUI();

    // must remember to call UnserializeParams at the end
    return UnserializeParams(chunk, startPos);
}

void WavShaper::updateUI() { loadShape(); }

void WavShaper::DoPreset(const char* name) {}

void WavShaper::copyToPluginDir(fs::path src)
{
    char* p;
    size_t len;
    _dupenv_s(&p, &len, "APPDATA");
    fs::path target = p;
    target = target / "DJ_Level_3" / "WavShaper" / src.filename();

    if (src.extension().generic_string().compare("wav"))
        DoPreset("");

    try // If you want to avoid exception handling, then use the error code overload of the following functions.
    {
        fs::create_directories(target.parent_path()); // Recursively create target directory if not existing.
        fs::copy_file(src, target, fs::copy_options::overwrite_existing);
    }
    catch (std::exception& e) // Not using fs::filesystem_error since std::bad_alloc can throw too.
    {
        std::cout << e.what() << std::endl;
    }
}