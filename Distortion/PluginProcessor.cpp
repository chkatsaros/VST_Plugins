#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

//==============================================================================

DistortionAudioProcessor::DistortionAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    ppManager(*this)
    , distortionType(ppManager, "Distortion type", distortionItemsUI, fullWave)
    , inGainSlider(ppManager, "Input gain", "dB", -24.0f, 24.0f, 12.0f,
        [](float value) { return powf(10.0f, value * 0.05f); })
    , outGainSlider(ppManager, "Output gain", "dB", -24.0f, 24.0f, -24.0f,
        [](float value) { return powf(10.0f, value * 0.05f); })
    , toneSlider(ppManager, "Tone", "dB", -24.0f, 24.0f, 12.0f,
        [this](float value) { toneSlider.setCurrentAndTargetValue(value); updateFilters(); return value; })
{
    ppManager.apvts.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

DistortionAudioProcessor::~DistortionAudioProcessor()
{
}

//==============================================================================

void DistortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    distortionType.reset(sampleRate, smoothTime);
    inGainSlider.reset(sampleRate, smoothTime);
    outGainSlider.reset(sampleRate, smoothTime);
    toneSlider.reset(sampleRate, smoothTime);

    //======================================

    filters.clear();
    for (int i = 0; i < getTotalNumInputChannels(); ++i) {
        Filter* filter;
        filters.add(filter = new Filter());
    }
    updateFilters();
}

void DistortionAudioProcessor::releaseResources()
{
}

void DistortionAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    for (int channel = 0; channel < numInputChannels; channel++) {
        float* channelData = buffer.getWritePointer(channel);

        float output;
        float thresholdHC = 0.5f;
        float thresholdSC1 = 1.0f / 3.0f;
        float thresholdSC2 = 2.0f / 3.0f;

        for (int sample = 0; sample < numSamples; sample++) {
            const float input = channelData[sample] * inGainSlider.getNextValue();

            if ((int)distortionType.getTargetValue() == hardClipping) {
               
                if (input > thresholdHC) output = thresholdHC;
                else if (input < -thresholdHC) output = -thresholdHC;
                else output = input;
            }
            else if ((int)distortionType.getTargetValue() == softClipping) {
               
                if (input > thresholdSC2) output = 1.0f;
                else if (input > thresholdSC1) output = 1.0f - powf(2.0f - 3.0f * input, 2.0f) / 3.0f;
                else if (input < -thresholdSC2) output = -1.0f;
                else if (input < -thresholdSC1) output = -1.0f + powf(2.0f + 3.0f * input, 2.0f) / 3.0f;
                else output = 2.0f * input;

                output *= 0.5f;
            }
            else if ((int)distortionType.getTargetValue() == expoSoftClipping) {
                
                if (input > 0.0f) output = 1.0f - expf(-input);
                else output = -1.0f + expf(input);
            }
            else if (halfWave) {

                if (input > 0.0f) output = input;
                else output = 0.0f;
            }
            else output = fabsf(input);

            channelData[sample] = filters[channel]->processSingleSampleRaw(output) * outGainSlider.getNextValue();
        }
    }

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);
}

//==============================================================================

void DistortionAudioProcessor::updateFilters()
{
    for (int i = 0; i < filters.size(); ++i) {
        filters[i]->updateCoefficients(M_PI * 0.01, pow(10.0, (double)toneSlider.getTargetValue() * 0.05));
    }
}

//==============================================================================






//==============================================================================

void DistortionAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = ppManager.apvts.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DistortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(ppManager.apvts.state.getType()))
            ppManager.apvts.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================

AudioProcessorEditor* DistortionAudioProcessor::createEditor()
{
    return new DistortionAudioProcessorEditor(*this);
}

bool DistortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool DistortionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

//==============================================================================

const String DistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DistortionAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool DistortionAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool DistortionAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double DistortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int DistortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DistortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DistortionAudioProcessor::setCurrentProgram(int index)
{
}

const String DistortionAudioProcessor::getProgramName(int index)
{
    return {};
}

void DistortionAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortionAudioProcessor();
}