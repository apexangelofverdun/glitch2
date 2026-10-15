#include "PluginProcessor.h"
#include "PluginEditor.h"

// ------------------------------------------------------------------ parameters
static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.f)
    { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.f, skew), def)); };
    auto I = [&] (const char* id, const char* name, int lo, int hi, int def)
    { l.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def)); };
    auto C = [&] (const char* id, const char* name, StringArray s, int def)
    { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, s, def)); };

    // time allocation / sequencer
    C ("stepDiv",  "Step Length",  { "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar" }, 1);
    C ("loopLen",  "Loop Length",  { "Off", "1/8", "1/4", "1/2", "1", "2", "4", "8", "16" }, 0);
    I ("seed",     "Seed", 0, 9999, 1);
    F ("noteProb", "Note Probability", 0.f, 1.f, 1.f);
    F ("velMin",   "Velocity Min", 0.f, 1.f, 0.6f);
    F ("velMax",   "Velocity Max", 0.f, 1.f, 1.f);
    F ("gateMin",  "Gate Min", 0.05f, 1.f, 0.6f);
    F ("gateMax",  "Gate Max", 0.05f, 1.f, 1.f);
    F ("smoothMs", "Smooth (ms)", 0.f, 100.f, 5.f);

    // stutter engine
    F ("stutProb", "Stutter Probability", 0.f, 1.f, 0.7f);
    I ("stutMax",  "Max Sub-Glitches", 1, 32, 8);
    F ("revProb",  "Reverse Probability", 0.f, 1.f, 0.3f);
    I ("speedMax", "Max Speed (x)", 1, 8, 4);

    // repan
    F ("panProb",   "Repan Probability", 0.f, 1.f, 0.5f);
    F ("panOffset", "Pan Offset", -1.f, 1.f, 0.f);
    F ("panWidth",  "Pan Width", 0.f, 1.f, 0.8f);
    F ("panMs",     "Pan Glide (ms)", 0.f, 500.f, 20.f);

    // bitcrush / downsample
    F ("crushProb", "Crush Probability", 0.f, 1.f, 0.3f);
    I ("crushBits", "Crush Min Bits", 1, 16, 6);
    I ("crushDs",   "Downsample Max", 1, 32, 8);

    // pitch
    F ("pitchProb",  "Pitch Probability", 0.f, 1.f, 0.3f);
    I ("pitchRange", "Pitch Range (+/- st)", 0, 48, 12);

    // comb
    F ("combProb", "Comb Probability", 0.f, 1.f, 0.2f);
    F ("combMin",  "Comb Freq Min", 40.f, 8000.f, 100.f, 0.3f);
    F ("combMax",  "Comb Freq Max", 40.f, 8000.f, 2000.f, 0.3f);
    F ("combFb",   "Comb Feedback", 0.f, 0.95f, 0.7f);

    // filter
    F ("filtProb", "Filter Probability", 0.f, 1.f, 0.3f);
    F ("filtMin",  "Filter Cutoff Min", 40.f, 18000.f, 200.f, 0.3f);
    F ("filtMax",  "Filter Cutoff Max", 40.f, 18000.f, 8000.f, 0.3f);
    F ("filtRes",  "Filter Resonance", 0.5f, 10.f, 1.5f, 0.5f);

    // feedback
    C ("fbInsert", "Feedback Insert", { "PreSeq", "PostSeq", "PostStut" }, 2);
    C ("fbType",   "Feedback Type", { "BBD", "Digital" }, 0);
    F ("fbMs",     "Feedback Delay (ms)", 1.f, 2000.f, 250.f, 0.4f);
    F ("fbAmt",    "Feedback Amount", 0.f, 0.95f, 0.f);

    F ("mix", "Dry/Wet", 0.f, 1.f, 1.f);
    return l;
}

GlitchSeqProcessor::GlitchSeqProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", makeLayout())
{
}

bool GlitchSeqProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

Cfg GlitchSeqProcessor::readCfg()
{
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    Cfg c;
    c.noteProb = P ("noteProb");
    c.velMin = P ("velMin"); c.velMax = P ("velMax");
    c.gateMin = P ("gateMin"); c.gateMax = P ("gateMax");
    c.smoothMs = P ("smoothMs");
    c.stutProb = P ("stutProb"); c.stutMax = (int) P ("stutMax");
    c.revProb = P ("revProb"); c.speedMax = (int) P ("speedMax");
    c.panProb = P ("panProb"); c.panOffset = P ("panOffset");
    c.panWidth = P ("panWidth"); c.panMs = P ("panMs");
    c.crushProb = P ("crushProb"); c.crushMinBits = (int) P ("crushBits"); c.crushDsMax = (int) P ("crushDs");
    c.pitchProb = P ("pitchProb"); c.pitchRange = (int) P ("pitchRange");
    c.combProb = P ("combProb"); c.combFMin = P ("combMin"); c.combFMax = P ("combMax"); c.combFb = P ("combFb");
    c.filtProb = P ("filtProb"); c.filtMin = P ("filtMin"); c.filtMax = P ("filtMax"); c.filtRes = P ("filtRes");
    c.fbInsert = (int) P ("fbInsert"); c.fbType = (int) P ("fbType");
    c.fbMs = P ("fbMs"); c.fbAmt = P ("fbAmt");
    c.seed = (int) P ("seed"); c.mix = P ("mix");
    if (c.velMin > c.velMax)   std::swap (c.velMin, c.velMax);
    if (c.gateMin > c.gateMax) std::swap (c.gateMin, c.gateMax);
    if (c.combFMin > c.combFMax) std::swap (c.combFMin, c.combFMax);
    if (c.filtMin > c.filtMax)   std::swap (c.filtMin, c.filtMax);
    return c;
}

// ------------------------------------------------------------------ lifecycle
void GlitchSeqProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    ringSize = (int) (8.0 * sr) + 8;
    for (int ch = 0; ch < 2; ++ch)
    {
        ring[ch].assign ((size_t) ringSize, 0.f);
        pitch[ch].prepare (sr);
        combDl[ch].resize ((int) (sr / 15.0) + 8);
        fbDl[ch].resize ((int) (2.1 * sr) + 8);
        hold[ch] = 0.f; holdCnt[ch] = 0; fbLp[ch] = 0.f;
    }
    ringW = 0;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    filt.prepare (spec);
    filt.reset();
    lastStepIdx = std::numeric_limits<long long>::min();
    seqGain = 0.f; pan = panTarget = 0.f; nStut = 0; lfoPh = 0.f;
    rng.setSeed (12345);
}

// ------------------------------------------------------------------ engine
void GlitchSeqProcessor::startStep (const Cfg& c, long long idx, double stepQN, double loopQN, int stepSamplesIn)
{
    stepSamples = juce::jmax (64, stepSamplesIn);

    // loop mode: re-seed deterministically so the random pattern repeats
    if (loopQN > 0.0)
    {
        int per = juce::jmax (1, (int) std::round (loopQN / stepQN));
        long long k = ((idx % per) + per) % per;
        rng.setSeed ((juce::int64) c.seed * 7919 + k * 104729 + 17);
    }

    active   = rng.nextFloat() < c.noteProb;
    baseGain = c.velMin + rng.nextFloat() * (c.velMax - c.velMin);
    gateFrac = c.gateMin + rng.nextFloat() * (c.gateMax - c.gateMin);
    nStut    = (rng.nextFloat() < c.stutProb) ? 1 + rng.nextInt (juce::jmax (1, c.stutMax)) : 0;
    subLen   = nStut > 0 ? juce::jmax (32, stepSamples / nStut) : stepSamples;
    posInStep = 0;
    nextSubAt = 0;
}

void GlitchSeqProcessor::startSub (const Cfg& c)
{
    auto logLerp = [] (float a, float b, float t) { return std::exp (std::log (a) + t * (std::log (b) - std::log (a))); };

    rev = rng.nextFloat() < c.revProb;
    int maxPow = (int) std::floor (std::log2 ((double) juce::jmax (1, c.speedMax)));
    int rate = 1 << rng.nextInt (maxPow + 1);              // 1,2,4,8
    // a captured grain repeats 2*rate times inside this sub-glitch
    grainLen   = juce::jlimit (16, ringSize / 2, subLen / (2 * rate));
    grainStart = ((ringW - grainLen) % ringSize + ringSize) % ringSize;
    grainPhase = 0.f;
    fade = juce::jmin (64, grainLen / 4);

    // repan
    if (rng.nextFloat() < c.panProb)
        panTarget = juce::jlimit (-1.f, 1.f, c.panOffset + (rng.nextFloat() * 2.f - 1.f) * c.panWidth);
    else
        panTarget = juce::jlimit (-1.f, 1.f, c.panOffset);

    // bitcrush / downsample
    crushOn = rng.nextFloat() < c.crushProb;
    int bits = c.crushMinBits + rng.nextInt (juce::jmax (1, 16 - c.crushMinBits + 1));
    crushQ  = std::pow (2.f, (float) bits - 1.f);
    crushDs = 1 + rng.nextInt (juce::jmax (1, c.crushDsMax));

    // pitch
    pitchOn = rng.nextFloat() < c.pitchProb;
    float semis = std::round ((rng.nextFloat() * 2.f - 1.f) * (float) c.pitchRange);
    pitchRatio = pitchOn ? std::pow (2.f, semis / 12.f) : 1.f;

    // comb
    combOn = rng.nextFloat() < c.combProb;
    float cf = logLerp (c.combFMin, c.combFMax, rng.nextFloat());
    combD = juce::jlimit (2.f, (float) combDl[0].size() - 4.f, (float) sr / cf);

    // multimode filter
    filtOn = rng.nextFloat() < c.filtProb;
    int ft = rng.nextInt (3);
    float cut = juce::jmin (logLerp (c.filtMin, c.filtMax, rng.nextFloat()), (float) sr * 0.45f);
    filt.setType (ft == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                : ft == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                          : juce::dsp::StateVariableTPTFilterType::highpass);
    filt.setCutoffFrequency (cut);
    filt.setResonance (c.filtRes);
}

float GlitchSeqProcessor::readRing (int ch, float p) const
{
    if (p >= (float) ringSize) p -= (float) ringSize;
    int i0 = (int) p;
    int i1 = i0 + 1; if (i1 >= ringSize) i1 = 0;
    float f = p - (float) i0;
    return ring[ch][(size_t) i0] * (1.f - f) + ring[ch][(size_t) i1] * f;
}

void GlitchSeqProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || ringSize == 0) return;

    const Cfg c = readCfg();

    // transport
    double bpm = 120.0, ppq = freeRunPpq;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) { if (pos->getIsPlaying()) { playing = true; ppq = *p; } }
        }
    const double ppqPerSample = bpm / 60.0 / sr;

    static const double divQN[]  = { 0.125, 0.25, 0.5, 1.0, 2.0, 4.0 };
    static const double loopQN[] = { 0.0, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0 };
    const double stepQN = divQN[juce::jlimit (0, 5, (int) apvts.getRawParameterValue ("stepDiv")->load())];
    const double loopLenQN = loopQN[juce::jlimit (0, 8, (int) apvts.getRawParameterValue ("loopLen")->load())];
    const int stepSamplesNow = (int) (stepQN / ppqPerSample);

    const float smoothCoef = c.smoothMs < 0.01f ? 1.f : 1.f - std::exp (-1.f / (c.smoothMs * 0.001f * (float) sr));
    const float panCoef    = c.panMs < 0.1f ? 1.f : 1.f - std::exp (-1.f / (c.panMs * 0.001f * (float) sr));
    const float fbSamples  = juce::jlimit (1.f, (float) fbDl[0].size() - 8.f, c.fbMs * 0.001f * (float) sr);
    const float lpCoef     = 1.f - std::exp (-juce::MathConstants<float>::twoPi * 3500.f / (float) sr);
    const float lfoInc     = juce::MathConstants<float>::twoPi * 0.7f / (float) sr;
    const bool  bbd        = c.fbType == 0;

    float* d0 = buffer.getWritePointer (0);
    float* d1 = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        // ---- step clock
        long long stepIdx = (long long) std::floor (ppq / stepQN);
        if (stepIdx != lastStepIdx)
        {
            lastStepIdx = stepIdx;
            startStep (c, stepIdx, stepQN, loopLenQN, stepSamplesNow);
        }

        const float in[2] = { d0[i], d1[i] };

        // ---- feedback tap (delayed, filtered output)
        lfoPh += lfoInc; if (lfoPh > juce::MathConstants<float>::twoPi) lfoPh -= juce::MathConstants<float>::twoPi;
        float fbSig[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            float d = fbSamples + (bbd ? 0.0004f * (float) sr * std::sin (lfoPh) : 0.f);
            float r = fbDl[ch].read (juce::jmax (1.f, d));
            if (bbd) { fbLp[ch] += lpCoef * (r - fbLp[ch]); r = std::tanh (1.3f * fbLp[ch]); }
            fbSig[ch] = r * c.fbAmt;
        }

        // ---- sequencer: note probability / velocity / gate length, smoothed
        const bool gateOpen = active && (float) posInStep < gateFrac * (float) stepSamples;
        seqGain += ((gateOpen ? baseGain : 0.f) - seqGain) * smoothCoef;

        float x[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            float v = in[ch] + (c.fbInsert == 0 ? fbSig[ch] : 0.f);      // PreSeq
            v *= seqGain;
            v += (c.fbInsert == 1 ? fbSig[ch] : 0.f);                    // PostSeq
            x[ch] = v;
            ring[ch][(size_t) ringW] = v;
        }
        if (++ringW >= ringSize) ringW = 0;

        // ---- stutter engine
        if (nStut > 0 && posInStep >= nextSubAt) { startSub (c); nextSubAt += subLen; }

        float s[2];
        if (nStut > 0)
        {
            float env = 1.f;
            if (fade > 0)
                env = juce::jmin (1.f, grainPhase / (float) fade, ((float) grainLen - grainPhase) / (float) fade);
            float off = rev ? juce::jmax (0.f, (float) grainLen - 1.f - grainPhase) : grainPhase;
            for (int ch = 0; ch < 2; ++ch)
                s[ch] = readRing (ch, (float) grainStart + off) * env;
            grainPhase += 1.f;
            if (grainPhase >= (float) grainLen) grainPhase -= (float) grainLen;
        }
        else { s[0] = x[0]; s[1] = x[1]; }

        if (c.fbInsert == 2) { s[0] += fbSig[0]; s[1] += fbSig[1]; }     // PostStut

        // ---- FX chain: crush -> pitch -> comb -> filter -> pan
        pan += (panTarget - pan) * panCoef;
        const float a  = (pan + 1.f) * juce::MathConstants<float>::pi * 0.25f;
        const float gL = std::cos (a) * 1.41421356f, gR = std::sin (a) * 1.41421356f;

        float wet[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            float v = s[ch];

            if (crushOn)
            {
                if (holdCnt[ch] <= 0) { hold[ch] = v; holdCnt[ch] = crushDs; }
                --holdCnt[ch];
                v = std::round (hold[ch] * crushQ) / crushQ;
            }

            float p = pitch[ch].process (v, pitchRatio);
            if (pitchOn) v = p;

            {
                float y = v + c.combFb * combDl[ch].read (combD);
                combDl[ch].push (y);
                if (combOn) v = y * std::sqrt (1.f - c.combFb * c.combFb);
            }

            float f = filt.processSample (ch, v);
            if (filtOn) v = f;

            wet[ch] = v * (ch == 0 ? gL : gR);
        }

        // ---- feedback write, mix
        fbDl[0].push (wet[0]);
        fbDl[1].push (wet[1]);

        d0[i] = in[0] * (1.f - c.mix) + wet[0] * c.mix;
        d1[i] = in[1] * (1.f - c.mix) + wet[1] * c.mix;

        ++posInStep;
        ppq += ppqPerSample;
    }
    freeRunPpq = ppq;
}

// ------------------------------------------------------------------ state / editor
juce::AudioProcessorEditor* GlitchSeqProcessor::createEditor()
{
    return new GlitchSeqEditor (*this);
}

void GlitchSeqProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void GlitchSeqProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GlitchSeqProcessor(); }
