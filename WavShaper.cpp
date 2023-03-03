#include "WavShaper.h"
#include "IControls.h"
#include "IPlug_include_in_plug_src.h"

WavShaper::WavShaper(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    GetParam(kGainI)->InitDouble("In Gain", 100., 0., 200.0, 0.02, "%");
    GetParam(kGainO)->InitDouble("Out Gain", 100., 0., 200.0, 0.02, "%");
    GetParam(kEnable)->InitBool("Enable", false, "Enable", 0, "", " ", " ");
    GetParam(kFade)->InitDouble("Shaping", 100., 0., 100.0, 0.01, "%");
    GetParam(kOffset)->InitDouble("Offset", 0., -1., 1.0, 0.01, "");
    GetParam(kRotation)->InitDouble("Rotation", 0., -180., 180.0, 0.01, "deg");
    GetParam(kOptimize)->InitBool("Optimize", false, "Optimize", 0, "", "No", "Yes");
    GetParam(kCenter)->InitBool("Center", false, "Center", 0, "", " ", " ");
    GetParam(kBank)->InitInt("Bank", 0, 0, 512, "Bank");

    loadShape();

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
    mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };

    mLayoutFunc = [&](IGraphics* pGraphics) {
        pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
        pGraphics->AttachPanelBackground(IColor(255, 201, 185, 162));
        pGraphics->LoadFont("Hack-Regular", HACK_FN);
        pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
        IBitmap logo = pGraphics->LoadBitmap(WS_LOGO_FN);

        const IVStyle style{
            true, // Show label
            true, // Show value
            {
                DEFAULT_BGCOLOR, // Background
                DEFAULT_FGCOLOR,               // Foreground
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
        auto bankchange = [this](IControl* pCaller) {
            lastBank = (GetParam(kBank)->Value() < targets.nBanks) ? (GetParam(kBank)->Value()) : (targets.nBanks - 1);
            loadShape(lastBank, false);
        };
        auto reload = [this](IControl* pCaller) { loadShape(lastBank,true); };

        const IRECT b = pGraphics->GetBounds();
        pGraphics->AttachControl(new IBitmapControl(b.GetCentredInside(400,320), logo));
        pGraphics->AttachControl(new IVSliderControl(b.GetCentredInside(80, 150).GetVShifted(220).GetHShifted(-165), kBank, " "))->SetAnimationEndActionFunction(bankchange);
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(120, 80).GetVShifted(-240).GetHShifted(-140), kGainI));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100, 80).GetVShifted(-240).GetHShifted(-45), kOffset));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100, 80).GetVShifted(-240).GetHShifted(45), kFade));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(120, 80).GetVShifted(-240).GetHShifted(140), kGainO));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100, 80).GetVShifted(230).GetHShifted(140), kRotation));
        pGraphics->AttachControl(new IVSlideSwitchControl(b.GetCentredInside(70, 50).GetVShifted(230).GetHShifted(-80), kEnable));
        pGraphics->AttachControl(new IVToggleControl(b.GetCentredInside(120, 50).GetVShifted(250).GetHShifted(35), kOptimize))->SetAnimationEndActionFunction(reload);
        pGraphics->AttachControl(new IVButtonControl(b.GetCentredInside(120, 30).GetVShifted(200).GetHShifted(35), SplashClickActionFunc, "Choose Shape...", style))
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

    int sNum = 1;
    while (sNum - 1 < nFrames)
    {
        if (targets.left.empty())
        {
            outputs[0][sNum - 1] = 0;
            outputs[1][sNum - 1] = 0;
        }
        else
        {
            sample samp = offset + (inputs[0][sNum - 1] + inputs[1][sNum - 1]) * gainI * 0.5;
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
            sample sampL = doShapingL(samp) * multiplier;
            sampL = lerp<sample>(inputs[0][sNum - 1], sampL, fade);
            sample sampR = doShapingR(samp) * multiplier;
            sampR = lerp<sample>(inputs[1][sNum - 1], sampR, fade);
            outputs[0][sNum - 1] = on ? (sampL * gainO) : inputs[0][sNum - 1];
            outputs[1][sNum - 1] = on ? (sampR * gainO) : inputs[1][sNum - 1];
        }
        sNum = sNum + 1;
    }
}
#endif

sample WavShaper::doShapingL(sample in)
{
    int index = findInSampleSet(in) - 1 + (lastBank * MAX_SHAPE_SAMPLES);
    double t = (in + 1) - (index * SHAPE_SAMPLE_SCALE);
    sample out = lerp<sample>(targets.left[index], targets.left[index + 1], t);
    return out;
}

sample WavShaper::doShapingR(sample in)
{
    int index = findInSampleSet(in) - 1 + (lastBank * MAX_SHAPE_SAMPLES);
    double t = (in + 1) - (index * SHAPE_SAMPLE_SCALE);
    sample out = lerp<sample>(targets.right[index], targets.right[index + 1], t);
    return out;
}

int WavShaper::findInSampleSet(sample in)
{
    int i = 1 + (int)((in + 1) * (HALF_SHAPE_SAMPLES - 1));
    return i;
}

void WavShaper::loadShape(const char* name)
{
    fs::path path = name;
    copyToPluginDir(path);
    targets.name = path.filename().generic_string();
    GetParam(kBank)->Set(0);
    lastBank = 0;
    loadShape(lastBank, true);
}

void WavShaper::loadShape(int num, bool reinit) {
    if (reinit || targets.init)
    {
        targets.init = false;
        if (targets.name != "((init))")
        {
            targets.left.clear();
            targets.right.clear();
            AudioFile<sample> f;
            char* p;
            size_t len;
            _dupenv_s(&p, &len, "APPDATA");
            fs::path target = p;
            target = target / "DJ_Level_3" / "WavShaper" / targets.name;
            try
            {
                f.load(target.generic_string());
                int nSamps = (int)(f.getNumSamplesPerChannel() / MAX_SHAPE_SAMPLES) * MAX_SHAPE_SAMPLES;
                for (int i = 0; i < nSamps; i++)
                {
                    targets.left.push_back(f.samples[0][i]);
                    targets.right.push_back(f.samples[1][i]);
                }
                targets.nBanks = nSamps / MAX_SHAPE_SAMPLES;
            }
            catch (std::exception& e)
            {
                targets.left.clear();
                targets.right.clear();
                for (int i = 0; i < MAX_SHAPE_SAMPLES; i++)
                {
                    targets.left.push_back(0);
                    targets.right.push_back(0);
                }
                targets.nBanks = 1;
            }
        }
        else
        {
            for (int i = 0; i < targets.left.size(); i++)
            {
                targets.left[i] = 0;
                targets.right[i] = 0;
            }
        }
    }
    if (GetParam(kOptimize)->Value())
    {
        optimizeShape();
    }
    GetParam(kBank)->Set(num);
}

void WavShaper::optimizeShape()
{
    float scale = 0;
    if (!targets.left.empty())
    {
        double maxX = *std::max_element(targets.left.begin(), targets.left.end());
        double minX = *std::min_element(targets.left.begin(), targets.left.end());

        double maxY = *std::max_element(targets.right.begin(), targets.right.end());
        double minY = *std::min_element(targets.right.begin(), targets.right.end());

        scale = (std::max(std::abs(std::max(maxX, maxY)), std::abs(std::min(minX, minY))));
        scale = (scale == 0) ? (0.) : (1.0 / scale);
    }
    for (int i = 0; i < targets.left.size(); i++)
    {
        targets.left[i] = targets.left[i] * scale;
        targets.right[i] = targets.right[i] * scale;
    }
}

void WavShaper::loadShape() { loadShape(lastBank, true); }

bool WavShaper::SerializeState(IByteChunk& chunk) const
{
    // serialize the multislider state state before serializing the regular params
    chunk.PutStr(std::to_string(lastBank).c_str());
    chunk.PutStr(targets.name.c_str());

    return SerializeParams(chunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int WavShaper::UnserializeState(const IByteChunk& chunk, int startPos)
{
    WDL_String name;
    WDL_String bank;

    // unserialize the steps state before unserializing the regular params
    startPos = chunk.GetStr(bank, startPos);
    startPos = chunk.GetStr(name, startPos);
    lastBank = std::stoi(bank.Get());
    targets.name = std::string(name.Get());
    targets.init = true;

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