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
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> sa;   // declared after the slider so it is destroyed first
    };

    struct Row
    {
        juce::String title;
        std::vector<Item*> items;
    };

    void addItem (Row& row, const juce::String& id, const juce::String& text);

    GlitchSeqProcessor& proc;
    std::vector<std::unique_ptr<Item>> items;
    std::vector<Row> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlitchSeqEditor)
};
