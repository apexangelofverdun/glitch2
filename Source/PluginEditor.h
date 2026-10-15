#pragma once
#include "PluginProcessor.h"

class GlitchSeqEditor : public juce::AudioProcessorEditor
{
public:
    explicit GlitchSeqEditor (GlitchSeqProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

    struct Item
    {
        juce::String id;
        bool randomizable = true;
        float maxNorm = 1.f;             // upper limit (0..1) used when randomizing
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> sa;   // declared after the slider so it is destroyed first
    };

    struct Row
    {
        juce::String title;
        std::vector<Item*> items;
        std::unique_ptr<juce::TextButton> rnd;
    };

    void addItem (Row& row, const juce::String& id, const juce::String& text,
                  bool randomizable = true, float maxNorm = 1.f);
    void randomizeItem (Item&);

    GlitchSeqProcessor& proc;
    std::vector<std::unique_ptr<Item>> items;
    std::vector<Row> rows;
    juce::TextButton randomAll { "RANDOMIZE ALL" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlitchSeqEditor)
};
