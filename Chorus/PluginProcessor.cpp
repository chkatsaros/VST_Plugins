#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

//==============================================================================

ChorusAudioProcessor::ChorusAudioProcessor() :
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
    , delaySlider(ppManager, "Delay", "ms", 10.0f, 50.0f, 30.0f, [](float value) { return value * 0.001f; })
    , widthSlider(ppManager, "Width", "ms", 10.0f, 50.0f, 20.0f, [](float value) { return value * 0.001f; })
    , depthSlider(ppManager, "Depth", "", 0.0f, 1.0f, 1.0f)
    , voiceBox(ppManager, "Number of voices", { "2", "3", "4", "5" }, 0, [](float value) { return value + 2; })
    , freqSlider(ppManager, "LFO Frequency", "Hz", 0.05f, 2.0f, 0.2f)
    , waveformBox(ppManager, "LFO Waveform", waveformUI, waveformSine)
    , stereoButton(ppManager, "Stereo", true)
{
    ppManager.apvts.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

ChorusAudioProcessor::~ChorusAudioProcessor()
{
}

//==============================================================================

void ChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    delaySlider.reset(sampleRate, smoothTime);
    widthSlider.reset(sampleRate, smoothTime);
    depthSlider.reset(sampleRate, smoothTime);
    voiceBox.reset(sampleRate, smoothTime);
    freqSlider.reset(sampleRate, smoothTime);
    waveformBox.reset(sampleRate, smoothTime);
    stereoButton.reset(sampleRate, smoothTime);

    //======================================

    float maxDelayTime = delaySlider.maxValue + widthSlider.maxValue;
    delayBufferSamples = (int)(maxDelayTime * (float)sampleRate) + 1;
    if (delayBufferSamples < 1)
        delayBufferSamples = 1;

    delayBufferChannels = getTotalNumInputChannels();
    delayBuffer.setSize(delayBufferChannels, delayBufferSamples);
    delayBuffer.clear();

    lfoPhase = 0.0f;
    inverseSR = 1.0f / (float)sampleRate;

    delayWritePos = 0;
}

void ChorusAudioProcessor::releaseResources()
{
}

void ChorusAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    bool stereo = (bool)stereoButton.getTargetValue();
    int numVoices = (int)voiceBox.getTargetValue();
    float depth = depthSlider.getNextValue();
    float delayTime = delaySlider.getNextValue();
    float width = widthSlider.getNextValue();
    float frequency = freqSlider.getNextValue();

    int writePos;
    float phase;

    for (int channel = 0; channel < numInputChannels; ++channel) {
        
        float* channelData = buffer.getWritePointer(channel);
        float* delayData = delayBuffer.getWritePointer(channel);
        
        writePos = delayWritePos;
        phase = lfoPhase;

        for (int sample = 0; sample < numSamples; sample++) {
            
            const float input = channelData[sample];
            float output = 0.0f;
           
            float weight = 1.0f;
            float phaseOffset = .0f;

            for (int voice = 0; voice < numVoices - 1; voice++) {

                if (stereo) {
                    if (numVoices > 2) {
                        weight = (float)voice / (float)(numVoices - 2);
                        if (!channel) weight = 1.0f - weight;
                    }
                }

                float currDelay = (delayTime + width * lowFrequencyOscillator(phase + phaseOffset, (int)waveformBox.getTargetValue())) * (float)getSampleRate();

                float readPos = fmodf((float)writePos - currDelay + (float)delayBufferSamples, delayBufferSamples);
                
                int currReadPos = floorf(readPos);

                float nearestSample = delayData[currReadPos % delayBufferSamples];
                output = nearestSample;

                if (stereo) {
                    if (numVoices == 2) {
                        if (channel == 0) channelData[sample] = input;
                        else channelData[sample] = output * depth;
                    }
                }
                else channelData[sample] += output * depth * weight;

                if (numVoices == 3) phaseOffset += 0.25f;
                else if (numVoices > 3) phaseOffset += 1.0f / (float)(numVoices - 1);
            }

            delayData[writePos] = input;

            writePos++;

            if (writePos >= delayBufferSamples) writePos -= delayBufferSamples;

            phase += frequency * inverseSR;

            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    delayWritePos = writePos;
    lfoPhase = phase;

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);
}

//==============================================================================

float ChorusAudioProcessor::lowFrequencyOscillator(float phase, int waveform)
{
    float out = 0.0f;

    switch (waveform) {
    case waveformSine: {
        out = 0.5f + 0.5f * sinf(2.0f * M_PI * phase);
        break;
    }
    case waveformTriangle: {
        if (phase < 0.25f)
            out = 0.5f + 2.0f * phase;
        else if (phase < 0.75f)
            out = 1.0f - 2.0f * (phase - 0.25f);
        else
            out = 2.0f * (phase - 0.75f);
        break;
    }
    case waveformSawtooth: {
        if (phase < 0.5f)
            out = 0.5f + phase;
        else
            out = phase - 0.5f;
        break;
    }
    case waveformInverseSawtooth: {
        if (phase < 0.5f)
            out = 0.5f - phase;
        else
            out = 1.5f - phase;
        break;
    }
    }

    return out;
}

//==============================================================================






//==============================================================================

void ChorusAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = ppManager.apvts.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChorusAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(ppManager.apvts.state.getType()))
            ppManager.apvts.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================

AudioProcessorEditor* ChorusAudioProcessor::createEditor()
{
    return new ChorusAudioProcessorEditor(*this);
}

bool ChorusAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChorusAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

const String ChorusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChorusAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ChorusAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ChorusAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ChorusAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int ChorusAudioProcessor::getNumPrograms()
{
    return 1;   
}

int ChorusAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ChorusAudioProcessor::setCurrentProgram(int index)
{
}

const String ChorusAudioProcessor::getProgramName(int index)
{
    return {};
}

void ChorusAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChorusAudioProcessor();
}