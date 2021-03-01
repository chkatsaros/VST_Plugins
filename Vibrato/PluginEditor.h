#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================

class VibratoAudioProcessorEditor : public AudioProcessorEditor, private Timer
{
public:
    //==============================================================================

    VibratoAudioProcessorEditor(VibratoAudioProcessor&);
    ~VibratoAudioProcessorEditor();

    //==============================================================================

    void paint(Graphics&) override;
    void resized() override;

private:
    //==============================================================================

    VibratoAudioProcessor& processor;

    enum {
        editorWidth = 500,
        margin = 10,
        editorPadding = 10,

        sliderTextEntryBoxWidth = 100,
        sliderTextEntryBoxHeight = 25,
        sliderHeight = 25,
        buttonHeight = 25,
        comboBoxHeight = 25,
        labelWidth = 100,
    };

    //======================================

    OwnedArray<Slider> sliders;
    OwnedArray<ToggleButton> toggles;
    OwnedArray<ComboBox> comboBoxes;

    OwnedArray<Label> labels;
    Array<Component*> components;

    typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
    typedef AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;

    OwnedArray<SliderAttachment> sliderAttachments;
    OwnedArray<ButtonAttachment> buttonAttachments;
    OwnedArray<ComboBoxAttachment> comboBoxAttachments;

    //======================================

    void timerCallback() override;
    void updateUIcomponents();
    Label pitchShiftLabel;

    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VibratoAudioProcessorEditor)
};