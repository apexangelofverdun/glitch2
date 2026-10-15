#include "PluginEditor.h"

static const int kTitleW = 110, kItemW = 90, kRowH = 84, kPad = 10;

void GlitchSeqEditor::addItem (Row& row, const juce::String& id, const juce::String& text)
{
    auto item = std::make_unique<Item>();

    item->label.setText (text, juce::dontSendNotification);
    item->label.setJustificationType (juce::Justification::centred);
    item->label.setFont (juce::FontOptions (12.f));
    addAndMakeVisible (item->label);

    item->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    item->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    item->slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe0a030));
    item->slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    item->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (item->slider);
    item->sa = std::make_unique<SliderAtt> (proc.apvts, id, item->slider);   // also works for choice params

    row.items.push_back (item.get());
    items.push_back (std::move (item));
}

GlitchSeqEditor::GlitchSeqEditor (GlitchSeqProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    rows.reserve (8);   // keep Row addresses stable while filling

    auto newRow = [this] (const char* title) -> Row& { rows.push_back ({ title, {} }); return rows.back(); };

    { auto& r = newRow ("TIME");
      addItem (r, "stepDiv", "Step");
      addItem (r, "loopLen", "Loop");
      addItem (r, "seed",    "Seed");
      addItem (r, "mix",     "Dry/Wet"); }

    { auto& r = newRow ("SEQUENCER");
      addItem (r, "noteProb", "Note Proba");
      addItem (r, "velMin",   "Vel Min");
      addItem (r, "velMax",   "Vel Max");
      addItem (r, "gateMin",  "Gate Min");
      addItem (r, "gateMax",  "Gate Max");
      addItem (r, "smoothMs", "Smooth"); }

    { auto& r = newRow ("STUTTER");
      addItem (r, "stutProb", "Stutter Proba");
      addItem (r, "stutMax",  "Max Sub");
      addItem (r, "revProb",  "Reverse Proba");
      addItem (r, "speedMax", "Max Speed"); }

    { auto& r = newRow ("PANNING");
      addItem (r, "panProb",   "Repan Proba");
      addItem (r, "panOffset", "Offset");
      addItem (r, "panWidth",  "Width");
      addItem (r, "panMs",     "Time (ms)"); }

    { auto& r = newRow ("CRUSH / PITCH");
      addItem (r, "crushProb",  "Crush Proba");
      addItem (r, "crushBits",  "Min Bits");
      addItem (r, "crushDs",    "Downsample");
      addItem (r, "pitchProb",  "Pitch Proba");
      addItem (r, "pitchRange", "Range (st)"); }

    { auto& r = newRow ("COMB");
      addItem (r, "combProb", "Comb Proba");
      addItem (r, "combMin",  "Freq Min");
      addItem (r, "combMax",  "Freq Max");
      addItem (r, "combFb",   "Feedback"); }

    { auto& r = newRow ("FILTER");
      addItem (r, "filtProb", "Filter Proba");
      addItem (r, "filtMin",  "Cutoff Min");
      addItem (r, "filtMax",  "Cutoff Max");
      addItem (r, "filtRes",  "Resonance"); }

    { auto& r = newRow ("FEEDBACK");
      addItem (r, "fbInsert", "Insert");
      addItem (r, "fbType",   "Type");
      addItem (r, "fbMs",     "Delay (ms)");
      addItem (r, "fbAmt",    "Amount"); }

    setSize (kTitleW + 6 * kItemW + 2 * kPad, (int) rows.size() * kRowH + 2 * kPad);
}

void GlitchSeqEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15151a));
    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto area = juce::Rectangle<int> (kPad, kPad + (int) i * kRowH, getWidth() - 2 * kPad, kRowH - 4);
        g.setColour (juce::Colour (0xff202028));
        g.fillRoundedRectangle (area.toFloat(), 6.f);
        g.setColour (juce::Colour (0xffe0a030));
        g.setFont (juce::FontOptions (14.f, juce::Font::bold));
        g.drawText (rows[i].title, area.withWidth (kTitleW).reduced (8, 0), juce::Justification::centredLeft);
    }
}

void GlitchSeqEditor::resized()
{
    for (size_t r = 0; r < rows.size(); ++r)
    {
        int x = kPad + kTitleW;
        int y = kPad + (int) r * kRowH;
        for (auto* it : rows[r].items)
        {
            it->label.setBounds (x, y + 2, kItemW, 14);
            it->slider.setBounds (x, y + 16, kItemW, 64);
            x += kItemW;
        }
    }
}
