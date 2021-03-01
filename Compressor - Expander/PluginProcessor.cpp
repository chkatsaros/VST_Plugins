#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

//==============================================================================

CompressorExpanderAudioProcessor::CompressorExpanderAudioProcessor() :
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
    , mode(ppManager, "Mode", { "Compressor / Limiter", "Expander / Noise gate" }, 1)
    , thresholdSlider(ppManager, "Threshold", "dB", -60.0f, 0.0f, -24.0f)
    , ratioSlider(ppManager, "Ratio", ":1", 1.0f, 100.0f, 50.0f)
    , attackSlider(ppManager, "Attack", "ms", 0.1f, 100.0f, 2.0f, [](float value) { return value * 0.001f; })
    , releaseSlider(ppManager, "Release", "ms", 10.0f, 1000.0f, 300.0f, [](float value) { return value * 0.001f; })
    , gainSlider(ppManager, "Makeup gain", "dB", -12.0f, 12.0f, 0.0f)
{
    ppManager.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

CompressorExpanderAudioProcessor::~CompressorExpanderAudioProcessor()
{
}

//==============================================================================

void CompressorExpanderAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    thresholdSlider.reset(sampleRate, smoothTime);
    ratioSlider.reset(sampleRate, smoothTime);
    attackSlider.reset(sampleRate, smoothTime);
    releaseSlider.reset(sampleRate, smoothTime);
    gainSlider.reset(sampleRate, smoothTime);
    //======================================

    mixedDownInput.setSize(1, samplesPerBlock);

    inputLvl = 0.0f;
    prevOutputLevel = 0.0f;

    inverseSampleRate = 1.0f / (float)getSampleRate();
    inverseE = 1.0f / M_E;
}

void CompressorExpanderAudioProcessor::releaseResources()
{
}

void CompressorExpanderAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    mixedDownInput.clear();
    for (int channel = 0; channel < numInputChannels; channel++)
        mixedDownInput.addFrom(0, 0, buffer, channel, 0, numSamples, 1.0f / numInputChannels);

    for (int sample = 0; sample < numSamples; ++sample) {
        
        float threshold = thresholdSlider.getNextValue();
        float ratio = ratioSlider.getNextValue();
        float attack = calculateAttackOrRelease(attackSlider.getNextValue());
        float release = calculateAttackOrRelease(releaseSlider.getNextValue());
        float extraGain = gainSlider.getNextValue();
        bool compressor = (bool)mode.getTargetValue();

        if (compressor) inputLvl = powf(mixedDownInput.getSample(0, sample), 2.0f);
        else inputLvl = 0.9999f * inputLvl + (1.0f - 0.9999f) * powf(mixedDownInput.getSample(0, sample), 2.0f);

        if (inputLvl <= 1e-6f) inputGain = -60.0f;
        else inputGain = 10.0f * log10f(inputLvl);

        if (compressor) {
           
            if (inputGain < threshold) outputGain = inputGain;
            else outputGain = threshold + (inputGain - threshold) / ratio;

            inputLevel = inputGain - outputGain;

            if (inputLevel > prevOutputLevel) outputLevel = ((1.0f - attack) * inputLevel) + (attack * prevOutputLevel);
            else outputLevel = ((1.0f - release) * inputLevel) + (release * prevOutputLevel);
        }
        else {

            if (inputGain > threshold) outputGain = inputGain;
            else outputGain = threshold + (inputGain - threshold) * ratio;

            inputLevel = inputGain - outputGain;

            if (inputLevel < prevOutputLevel) outputLevel = ((1.0f - attack) * inputLevel) + (attack * prevOutputLevel);
            else outputLevel = ((1.0f - release) * inputLevel) + (release * prevOutputLevel);
        }

        prevOutputLevel = outputLevel;

        for (int channel = 0; channel < numInputChannels; channel++) {
            float newValue = buffer.getSample(channel, sample) * powf(10.0f, (extraGain - outputLevel) * 0.05f);
            buffer.setSample(channel, sample, newValue);
        }
    }

    for (int channel = numInputChannels; channel < numOutputChannels; channel++) {
        buffer.clear(channel, 0, numSamples);
    }
}

//==============================================================================

float CompressorExpanderAudioProcessor::calculateAttackOrRelease(float value)
{
    if (value == 0.0f) return 0.0f;
    else return pow(inverseE, inverseSampleRate / value);
}

//==============================================================================






//==============================================================================

void CompressorExpanderAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = ppManager.valueTreeState.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CompressorExpanderAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(ppManager.valueTreeState.state.getType()))
            ppManager.valueTreeState.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================

AudioProcessorEditor* CompressorExpanderAudioProcessor::createEditor()
{
    return new CompressorExpanderAudioProcessorEditor(*this);
}

bool CompressorExpanderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool CompressorExpanderAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

const String CompressorExpanderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompressorExpanderAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool CompressorExpanderAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool CompressorExpanderAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double CompressorExpanderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int CompressorExpanderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CompressorExpanderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CompressorExpanderAudioProcessor::setCurrentProgram(int index)
{
}

const String CompressorExpanderAudioProcessor::getProgramName(int index)
{
    return {};
}

void CompressorExpanderAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorExpanderAudioProcessor();
}