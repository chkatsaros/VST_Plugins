#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

//==============================================================================

VibratoAudioProcessor::VibratoAudioProcessor() :
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
    , widthSlider(ppManager, "Width", "ms", 1.0f, 50.0f, 10.0f, [](float value) { return value * 0.001f; })
    , freqSlider(ppManager, "LFO Frequency", "Hz", 0.0f, 10.0f, 2.0f)
    , paramWaveform(ppManager, "LFO Waveform", waveformUI, waveformSine)
{
    ppManager.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

VibratoAudioProcessor::~VibratoAudioProcessor()
{
}

//==============================================================================

void VibratoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    widthSlider.reset(sampleRate, smoothTime);
    freqSlider.reset(sampleRate, smoothTime);
    paramWaveform.reset(sampleRate, smoothTime);

    //======================================

    float maxDelayTime = widthSlider.maxValue;
    delaySamples = (int)(maxDelayTime * (float)sampleRate) + 1;

    if (delaySamples < 1) delaySamples = 1;

    delayChannels = getTotalNumInputChannels();
    delayBuffer.setSize(delayChannels, delaySamples);
    delayBuffer.clear();

    delayWritePos = 0;
    inverseSR = 1.0f / (float)sampleRate;
    lfoPhase = 0.0f;
}

void VibratoAudioProcessor::releaseResources()
{
}

void VibratoAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    float currFrequency = freqSlider.getNextValue();
    float currWidth = widthSlider.getNextValue();

    int currWritePos;
    float phase;

    for (int channel = 0; channel < numInputChannels; channel++) {

        float* channelData = buffer.getWritePointer(channel);
        float* delayData = delayBuffer.getWritePointer(channel);

        currWritePos = delayWritePos;
        phase = lfoPhase;

        for (int sample = 0; sample < numSamples; ++sample) {

            const float input = channelData[sample];
            float output = 0.0f;

            float readPosition = fmodf((float)currWritePos - currWidth * lfo(phase, (int)paramWaveform.getTargetValue()) * (float)getSampleRate() + (float)delaySamples - 1.0f, delaySamples);
            int currReadPos = floorf(readPosition);

            float nearestSample = delayData[currReadPos % delaySamples];
            output = nearestSample;
           
            channelData[sample] = output;
            delayData[currWritePos] = input;

            currWritePos++;

            if (currWritePos >= delaySamples)
                currWritePos -= delaySamples;

            phase += currFrequency * inverseSR;

            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    lfoPhase = phase;
    delayWritePos = currWritePos;
    
    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; channel++)
        buffer.clear(channel, 0, numSamples);
}

//==============================================================================

float VibratoAudioProcessor::lfo(float phase, int waveform)
{
    float out = 0.0f;

    if (waveform == waveformSine) {
        out = 0.5f + 0.5f * sinf(2.f * M_PI * phase);
    }
    else if (waveform == waveformTriangle) {
        if (phase < 0.25f)
            out = 0.5f + 2.0f * phase;
        else if (phase < 0.75f)
            out = 1.0f - 2.0f * (phase - 0.25f);
        else
            out = 2.0f * (phase - 0.75f);
    }
    else if (waveform == waveformSawtooth) {
        if (phase < 0.5f)
            out = 0.5f + phase;
        else
            out = phase - 0.5f;
    }
    else {
        if (phase < 0.5f)
            out = 0.5f - phase;
        else
            out = 1.5f - phase;
    }

    return out;
}

//==============================================================================






//==============================================================================

void VibratoAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = ppManager.valueTreeState.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VibratoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(ppManager.valueTreeState.state.getType()))
            ppManager.valueTreeState.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================

AudioProcessorEditor* VibratoAudioProcessor::createEditor()
{
    return new VibratoAudioProcessorEditor(*this);
}

bool VibratoAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool VibratoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

//==============================================================================

const String VibratoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VibratoAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool VibratoAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool VibratoAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double VibratoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int VibratoAudioProcessor::getNumPrograms()
{
    return 1;
}
int VibratoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VibratoAudioProcessor::setCurrentProgram(int index)
{
}

const String VibratoAudioProcessor::getProgramName(int index)
{
    return {};
}

void VibratoAudioProcessor::changeProgramName(int index, const String& newName)
{
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibratoAudioProcessor();
}