#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginParameter.h"

//==============================================================================

class DistortionAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================

    DistortionAudioProcessor();
    ~DistortionAudioProcessor();

    //==============================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioSampleBuffer&, MidiBuffer&) override;

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

    StringArray distortionItemsUI = {
        "Hard Clipping",
        "Soft Clipping",
        "Exponential",
        "Full-Wave Rectifier",
        "Half-Wave Rectifier"
    };

    enum distortionType {
        hardClipping = 0,
        softClipping = 1,
        expoSoftClipping = 2,
        fullWave = 3,
        halfWave = 4,
    };

    //======================================

    class Filter : public IIRFilter
    {
    public:
        void updateCoefficients(const double discreteFrequency,
            const double gain) noexcept
        {
            jassert(discreteFrequency > 0);

            coeffs = IIRCoefficients(sqrt(gain) * tan(discreteFrequency / 2.0) + gain, sqrt(gain) * tan(discreteFrequency / 2.0) - gain, 0.0, sqrt(gain) * tan(discreteFrequency / 2.0) + 1.0, sqrt(gain) * tan(discreteFrequency / 2.0) - 1.0, 0.0);

            setCoefficients(coeffs);
        }
    };

    OwnedArray<Filter> filters;
    void updateFilters();

    //======================================

    PluginParametersManager ppManager;

    PluginParameterComboBox distortionType;
    PluginParameterLinSlider toneSlider;
    PluginParameterLinSlider inGainSlider;
    PluginParameterLinSlider outGainSlider;
    

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistortionAudioProcessor)
};