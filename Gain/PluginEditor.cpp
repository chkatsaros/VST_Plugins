/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MyPlugAudioProcessorEditor::MyPlugAudioProcessorEditor (MyPlugAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    addAndMakeVisible(mGainSlider);
    mGainSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    mGainSlider.setRange(0.0f, 1.0f, 0.1f);
    mGainSlider.setValue(.5f);
    mGainSlider.addListener(this);
    mGainSlider.setTextBoxStyle(Slider::TextBoxBelow, true, 50, 20);

    setSize (200, 300);
}

MyPlugAudioProcessorEditor::~MyPlugAudioProcessorEditor()
{
}

//==============================================================================
void MyPlugAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    
    g.fillAll(Colours::purple);
}

void MyPlugAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    mGainSlider.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 75, 100, 150);
}

void MyPlugAudioProcessorEditor::sliderValueChanged(Slider* slider) {
    if (slider != NULL)
        audioProcessor.mGain = slider->getValue();
}
