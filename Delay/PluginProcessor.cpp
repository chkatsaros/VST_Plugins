#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

DelayAudioProcessor::DelayAudioProcessor() :
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
    , delayTimeSlider(ppManager, "Delay time", "s", 0.0f, 5.0f, 0.1f)
    , feedbackSlider(ppManager, "Feedback", "", 0.0f, 0.9f, 0.7f)
    , mixSlider(ppManager, "Mix", "", 0.0f, 1.0f, 1.0f)
{
    ppManager.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

void DelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const double tiny = 0.001000000000000000021;
    delayTimeSlider.reset(sampleRate, tiny);
    feedbackSlider.reset(sampleRate, tiny);
    mixSlider.reset(sampleRate, tiny);

    float maxDelay = delayTimeSlider.max;
    delaySamples = (int)(maxDelay * (float)sampleRate) + 1;
    if (delaySamples < 1) delaySamples = 1;

    delayBufferChannels = getTotalNumInputChannels();
    delayBuffer.setSize(delayBufferChannels, delaySamples);
    delayBuffer.clear();

    delayWritePos = 0;
}

void DelayAudioProcessor::releaseResources()
{
}

void DelayAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();

    float currentMix = mixSlider.getNextValue();
    float currentFB = feedbackSlider.getNextValue();
    float currentDT = delayTimeSlider.getTargetValue() * (float)getSampleRate();

    int writePos;

    for (int channel = 0; channel < numInputChannels; channel++) {
        
        float* channelData = buffer.getWritePointer(channel);
        float* delayData = delayBuffer.getWritePointer(channel);
        writePos = delayWritePos;

        for (int sample = 0; sample < numSamples; sample++) {

            float output = 0.0f;

            float currReadPos = fmodf((float)writePos - currentDT + (float)delaySamples, delaySamples);
            int readPos = floorf(currReadPos);

            if (readPos != writePos) {

                output = delayData[(readPos + 0)] + (currReadPos - (float)readPos) * ((delayData[(readPos + 1) % delaySamples]) - (delayData[(readPos + 0)]));

                channelData[sample] = channelData[sample] + (currentMix * (output - channelData[sample]));
                delayData[writePos] = channelData[sample] + (output * currentFB);
            }

            writePos++;

            if (writePos >= delaySamples) writePos -= delaySamples;
        }
    }

    delayWritePos = writePos;

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);
}

void DelayAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = ppManager.valueTreeState.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(ppManager.valueTreeState.state.getType()))
            ppManager.valueTreeState.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================

AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor(*this);
}

bool DelayAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

const String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool DelayAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram(int index)
{
}

const String DelayAudioProcessor::getProgramName(int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName(int index, const String& newName)
{
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}