#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <limits>

// ---------------------------------------------------------------- DSP helpers
struct DelayLine
{
    std::vector<float> buf;
    int w = 0;
    void resize (int n) { buf.assign ((size_t) n, 0.f); w = 0; }
    int size() const { return (int) buf.size(); }
    void push (float x) { buf[(size_t) w] = x; if (++w >= size()) w = 0; }
    // d = samples behind the most recent write (d >= 1)
    float read (float d) const
    {
        float p = (float) w - d;
        while (p < 0.f) p += (float) size();
        int i0 = (int) p; if (i0 >= size()) i0 -= size();
        int i1 = i0 + 1;  if (i1 >= size()) i1 = 0;
        float f = p - (float) i0;
        return buf[(size_t) i0] * (1.f - f) + buf[(size_t) i1] * f;
    }
};

// Classic two-head crossfading delay-line pitch shifter
struct PitchShifter
{
    DelayLine dl;
    float ph = 0.f, grain = 2048.f;
    void prepare (double sr) { grain = (float) (0.04 * sr); dl.resize ((int) (grain * 2.f) + 8); ph = 0.f; }
    float process (float x, float ratio)
    {
        dl.push (x);
        ph += (1.f - ratio) / grain;
        ph -= std::floor (ph);
        float p2 = ph + 0.5f; p2 -= std::floor (p2);
        const float tp = juce::MathConstants<float>::twoPi;
        float w1 = 0.5f - 0.5f * std::cos (tp * ph);
        float w2 = 0.5f - 0.5f * std::cos (tp * p2);
        return w1 * dl.read (1.f + ph * grain) + w2 * dl.read (1.f + p2 * grain);
    }
};

// Snapshot of all parameters for one block
struct Cfg
{
    float noteProb, velMin, velMax, gateMin, gateMax, smoothMs;
    float stutProb; int stutMax; float revProb; int speedMax;
    float panProb, panOffset, panWidth, panMs;
    float crushProb; int crushMinBits, crushDsMax;
    float pitchProb; int pitchRange;
    float combProb, combFMin, combFMax, combFb;
    float filtProb, filtMin, filtMax, filtRes;
    int fbInsert, fbType; float fbMs, fbAmt;
    int seed; float mix;
};

// ---------------------------------------------------------------- processor
class GlitchSeqProcessor : public juce::AudioProcessor
{
public:
    GlitchSeqProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GlitchSeq"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    Cfg readCfg();
    void startStep (const Cfg&, long long idx, double stepQN, double loopQN, int stepSamplesIn);
    void startSub (const Cfg&);
    float readRing (int ch, float p) const;

    double sr = 44100.0;
    juce::Random rng;

    // capture ring
    std::vector<float> ring[2];
    int ringSize = 0, ringW = 0;

    // step state
    long long lastStepIdx = std::numeric_limits<long long>::min();
    int posInStep = 0, stepSamples = 1000;
    bool active = true;
    float baseGain = 1.f, gateFrac = 1.f, seqGain = 0.f;

    // stutter state
    int nStut = 0, subLen = 1000, nextSubAt = 0;
    int grainStart = 0, grainLen = 100, fade = 16;
    float grainPhase = 0.f;
    bool rev = false;

    // per sub-glitch FX state
    float pan = 0.f, panTarget = 0.f;
    bool crushOn = false; float crushQ = 1.f; int crushDs = 1;
    bool pitchOn = false; float pitchRatio = 1.f;
    bool combOn = false; float combD = 100.f;
    bool filtOn = false;

    // per channel DSP
    float hold[2] = { 0, 0 }; int holdCnt[2] = { 0, 0 };
    PitchShifter pitch[2];
    DelayLine combDl[2], fbDl[2];
    float fbLp[2] = { 0, 0 };
    juce::dsp::StateVariableTPTFilter<float> filt;
    float lfoPh = 0.f;
    double freeRunPpq = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlitchSeqProcessor)
};
