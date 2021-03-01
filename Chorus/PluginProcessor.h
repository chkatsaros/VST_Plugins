#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginParameter.h"

//==============================================================================

class ChorusAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================

    ChorusAudioProcessor();
    ~ChorusAudioProcessor();

    //==============================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================






    //==============================================================================

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    //==============================================================================

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    //==============================================================================






    //==============================================================================

    enum waveformIndex {
        waveformSine = 0,
        waveformTriangle,
        waveformSawtooth,
        waveformInverseSawtooth,
    };


    StringArray waveformUI = {
        "Sine",
        "Triangle",
        "Sawtooth",
        "Inverse Sawtooth"
    };

    //======================================

    AudioSampleBuffer delayBuffer;
    int delayBufferSamples;
    int delayBufferChannels;
    int delayWritePos;

    float lfoPhase;
    float inverseSR;

    float lowFrequencyOscillator(float phase, int waveform);

    //======================================

    PluginParametersManager ppManager;

    PluginParameterToggle stereoButton;

    PluginParameterLinSlider widthSlider;
    PluginParameterLinSlider delaySlider;
    PluginParameterComboBox voiceBox;
    PluginParameterLinSlider freqSlider;
    PluginParameterLinSlider depthSlider;
    PluginParameterComboBox waveformBox;
    
    

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusAudioProcessor)
};