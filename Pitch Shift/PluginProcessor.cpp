#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

//==============================================================================

PitchShiftAudioProcessor::PitchShiftAudioProcessor() :
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
    , pitchShiftSlider(ppManager, "Shift", " Semitone(s)", -12.0f, 12.0f, 0.0f,
        [this](float value) { return powf(2.0f, value / 12.0f); })
    , fftSizeCB(ppManager, "FFT size", fftSizeItemsUI, fftSize512,
        [this](float value) {
            const ScopedLock sl(lock);
            value = (float)(1 << ((int)value + 5));
            fftSizeCB.setCurrentAndTargetValue(value);
            updateFftSize();
            updateHopSize();

            for (int sample = 0; sample < fftSize; sample++) {
                fftWindow[sample] = 1.0f - fabs(2.0f * (float)sample / (float)(fftSize - 1) - 1.0f);
            }
            
            updateWindowScaleFactor();
            return value;
        })
    , hopSizeCB(ppManager, "Hop size", hopSizeItemsUI, hopSize8,
        [this](float value) {
            const ScopedLock sl(lock);
            value = (float)(1 << ((int)value + 1));
            hopSizeCB.setCurrentAndTargetValue(value);
            updateFftSize();
            updateHopSize();
            
            for (int sample = 0; sample < fftSize; sample++) {
                fftWindow[sample] = 1.0f - fabs(2.0f * (float)sample / (float)(fftSize - 1) - 1.0f);
            }

            updateWindowScaleFactor();
            return value;
        })
    , windowTypeCB(ppManager, "Window type", windowTypeItemsUI, windowTypeHann,
        [this](float value) {
            const ScopedLock sl(lock);
            windowTypeCB.setCurrentAndTargetValue(value);
            updateFftSize();
            updateHopSize();

            for (int sample = 0; sample < fftSize; sample++) {
                fftWindow[sample] = 1.0f - fabs(2.0f * (float)sample / (float)(fftSize - 1) - 1.0f);
            }

            updateWindowScaleFactor();
            return value;
        })
{
    ppManager.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

PitchShiftAudioProcessor::~PitchShiftAudioProcessor()
{
}

//==============================================================================

void PitchShiftAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    pitchShiftSlider.reset(sampleRate, smoothTime);
    fftSizeCB.reset(sampleRate, smoothTime);
    hopSizeCB.reset(sampleRate, smoothTime);
    windowTypeCB.reset(sampleRate, smoothTime);

    //======================================

    resetPhases = true;
}

void PitchShiftAudioProcessor::releaseResources()
{
}

void PitchShiftAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const ScopedLock sl(lock);

    ScopedNoDenormals noDenormals;
    
    int currOutReadPos;
    int currInWritePos;
    int currOutWritePos;
    int currSamplesLastFFT;

    float pitchShift = pitchShiftSlider.getNextValue();
    float ratio = roundf(pitchShift * (float)hopSize) / (float)hopSize;
    int resampledLength = floorf((float)fftSize / ratio);

    HeapBlock<float> resampledOutput(resampledLength, true);
    HeapBlock<float> synthesisWindow(resampledLength, true);

    for (int sample = 0; sample < resampledLength; sample++) {
        synthesisWindow[sample] = 1.0f - fabs(2.0f * (float)sample / (float)(resampledLength - 1) - 1.0f);
    }

    for (int channel = 0; channel < getTotalNumInputChannels(); channel++) {
        float* channelData = buffer.getWritePointer(channel);

        currInWritePos = inWritePos;
        currOutWritePos = outWritePos;
        currOutReadPos = outReadPos;
        currSamplesLastFFT = samplesLastFFT;

        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            
            const float in = channelData[sample];
            channelData[sample] = outputBuffer.getSample(channel, currOutReadPos);

            outputBuffer.setSample(channel, currOutReadPos, 0.0f);
            currOutReadPos++;
            if (currOutReadPos >= outLength) currOutReadPos = 0;

            inputBuffer.setSample(channel, currInWritePos, in);
            currInWritePos++;
            if (currInWritePos >= inLength) currInWritePos = 0;

            currSamplesLastFFT++;
            if (currSamplesLastFFT >= hopSize) {

                currSamplesLastFFT = 0;
                int inIndex = currInWritePos;

                for (int index = 0; index < fftSize; ++index) {
                    fftTimeDomain[index].real(sqrtf(fftWindow[index]) * inputBuffer.getSample(channel, inIndex));
                    fftTimeDomain[index].imag(0.0f);

                    inIndex++;
                    if (inIndex >= inLength) inIndex = 0;
                }

                fft->perform(fftTimeDomain, fftFrequencyDomain, false);

                if (pitchShiftSlider.isSmoothing()) resetPhases = true;

                if (resetPhases) {
                    if (pitchShift == pitchShiftSlider.getTargetValue()) {
                        inputPhase.clear();
                        outputPhase.clear();
                        resetPhases = false;
                    }
                }

                for (int fftIndex = 0; fftIndex < 512; fftIndex++) {

                    float magnitude = abs(fftFrequencyDomain[fftIndex]);
                    float phase = arg(fftFrequencyDomain[fftIndex]);

                    float phaseDev = phase - inputPhase.getSample(channel, fftIndex) - omega[fftIndex] * (float)hopSize;
                    float df = omega[fftIndex] * hopSize + princArg(phaseDev);
                    float newPhase = princArg(outputPhase.getSample(channel, fftIndex) + df * ratio);

                    inputPhase.setSample(channel, fftIndex, phase);
                    outputPhase.setSample(channel, fftIndex, newPhase);
                    fftFrequencyDomain[fftIndex] = std::polar(magnitude, newPhase);
                }

                fft->perform(fftFrequencyDomain, fftTimeDomain, true);

                for (int fftIndex = 0; fftIndex < resampledLength; fftIndex++) {

                    float x = (float)fftIndex * (float)fftSize / (float)resampledLength;
                    int ix = (int)floorf(x);
                    float dx = x - (float)ix;

                    float sample1 = fftTimeDomain[ix].real();
                    float sample2 = fftTimeDomain[(ix + 1) % fftSize].real();
                    resampledOutput[fftIndex] = sample1 + dx * (sample2 - sample1);
                    resampledOutput[fftIndex] *= sqrtf(synthesisWindow[fftIndex]);
                }

                int outIndex = currOutWritePos;
                for (int fftIndex = 0; fftIndex < resampledLength; fftIndex++) {

                    float out = outputBuffer.getSample(channel, outIndex);
                    out += resampledOutput[fftIndex] * windowScaleFactor;
                    outputBuffer.setSample(channel, outIndex, out);

                    outIndex++;
                    if (outIndex >= outLength) outIndex = 0;
                }

                currOutWritePos += hopSize;
                if (currOutWritePos >= outLength) currOutWritePos = 0;
            }
        }
    }

    inWritePos = currInWritePos;
    outWritePos = currOutWritePos;
    outReadPos = currOutReadPos;
    samplesLastFFT = currSamplesLastFFT;

    //======================================

    for (int channel = getNumInputChannels(); channel < getTotalNumOutputChannels(); channel++) {
        buffer.clear(channel, 0, buffer.getNumSamples());
    }
}

//==============================================================================

void PitchShiftAudioProcessor::updateFftSize()
{
    fftSize = 512;
    fft = std::make_unique<dsp::FFT>(log2(fftSize));

    inLength = fftSize;
    inWritePos = 0;
    inputBuffer.clear();
    inputBuffer.setSize(getTotalNumInputChannels(), inLength);

    float maxRatio = powf(2.0f, pitchShiftSlider.minValue / 12.0f);
    outLength = (int)floorf((float)fftSize / maxRatio);
    outWritePos = 0;
    outReadPos = 0;
    outputBuffer.clear();
    outputBuffer.setSize(getTotalNumInputChannels(), outLength);

    fftWindow.realloc(fftSize);
    fftWindow.clear(fftSize);

    fftTimeDomain.realloc(fftSize);
    fftTimeDomain.clear(fftSize);

    fftFrequencyDomain.realloc(fftSize);
    fftFrequencyDomain.clear(fftSize);

    samplesLastFFT = 0;

    omega.realloc(fftSize);
    for (int index = 0; index < fftSize; ++index) {
        omega[index] = 2.0f * M_PI * index / (float)fftSize;
    }

    inputPhase.clear();
    inputPhase.setSize(getTotalNumInputChannels(), outLength);

    outputPhase.clear();
    outputPhase.setSize(getTotalNumInputChannels(), outLength);
}

void PitchShiftAudioProcessor::updateHopSize()
{
    overlap = (int)hopSizeCB.getTargetValue();
    if (overlap != 0) {
        hopSize = fftSize / overlap;
        outWritePos = hopSize % outLength;
    }
}

void PitchShiftAudioProcessor::updateWindowScaleFactor()
{
    float windowSum = 0.0f;
    for (int sample = 0; sample < fftSize; ++sample)
        windowSum += fftWindow[sample];

    windowScaleFactor = 0.0f;
    if (overlap != 0 && windowSum != 0.0f)
        windowScaleFactor = 1.0f / (float)overlap / windowSum * (float)fftSize;
}

float PitchShiftAudioProcessor::princArg(const float phase)
{
    if (phase >= 0.0f) return fmod(phase + M_PI, 2.0f * M_PI) - M_PI;
    else return fmod(phase + M_PI, -2.0f * M_PI) + M_PI;
}

//==============================================================================

void PitchShiftAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = ppManager.valueTreeState.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PitchShiftAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(ppManager.valueTreeState.state.getType()))
            ppManager.valueTreeState.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================

AudioProcessorEditor* PitchShiftAudioProcessor::createEditor()
{
    return new PitchShiftAudioProcessorEditor(*this);
}

bool PitchShiftAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================


#ifndef JucePlugin_PreferredChannelConfigurations
bool PitchShiftAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

const String PitchShiftAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PitchShiftAudioProcessor::acceptsMidi() const
{
    #if JucePlugin_WantsMidiInput
                    return true;
    #else
                    return false;
    #endif
}

bool PitchShiftAudioProcessor::producesMidi() const
{
    #if JucePlugin_ProducesMidiOutput
                    return true;
    #else
                    return false;
    #endif
}

bool PitchShiftAudioProcessor::isMidiEffect() const
{
    #if JucePlugin_IsMidiEffect
        return true;
    #else
        return false;
    #endif
}

double PitchShiftAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int PitchShiftAudioProcessor::getNumPrograms()
{
    return 1;
}

int PitchShiftAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PitchShiftAudioProcessor::setCurrentProgram(int index)
{
}

const String PitchShiftAudioProcessor::getProgramName(int index)
{
    return {};
}

void PitchShiftAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchShiftAudioProcessor();
}