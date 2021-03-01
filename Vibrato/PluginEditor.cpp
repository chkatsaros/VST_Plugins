#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

VibratoAudioProcessorEditor::VibratoAudioProcessorEditor(VibratoAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    const Array<AudioProcessorParameter*> parameters = processor.getParameters();
    int counter = 0;

    int height = 2 * margin;
    for (int i = 0; i < parameters.size(); ++i) {
        if (const AudioProcessorParameterWithID* parameter =
            dynamic_cast<AudioProcessorParameterWithID*> (parameters[i])) {

            if (processor.ppManager.parameterTypes[i] == "Slider") {
                Slider* slider;
                sliders.add(slider = new Slider());
                slider->setTextValueSuffix(parameter->label);
                slider->setTextBoxStyle(Slider::TextBoxLeft,
                    false,
                    sliderTextEntryBoxWidth,
                    sliderTextEntryBoxHeight);

                SliderAttachment* sliderAttachment;
                sliderAttachments.add(sliderAttachment =
                    new SliderAttachment(processor.ppManager.valueTreeState, parameter->paramID, *slider));

                components.add(slider);
                height += sliderHeight;
            }

            //======================================

            else if (processor.ppManager.parameterTypes[i] == "ToggleButton") {
                ToggleButton* button;
                toggles.add(button = new ToggleButton());
                button->setToggleState(parameter->getDefaultValue(), dontSendNotification);

                ButtonAttachment* buttonAttachment;
                buttonAttachments.add(buttonAttachment =
                    new ButtonAttachment(processor.ppManager.valueTreeState, parameter->paramID, *button));

                components.add(button);
                height += buttonHeight;
            }

            //======================================

            else if (processor.ppManager.parameterTypes[i] == "ComboBox") {
                ComboBox* comboBox;
                comboBoxes.add(comboBox = new ComboBox());
                comboBox->setEditableText(false);
                comboBox->setJustificationType(Justification::left);
                comboBox->addItemList(processor.ppManager.comboBoxItemLists[counter++], 1);

                ComboBoxAttachment* comboBoxAttachment;
                comboBoxAttachments.add(comboBoxAttachment =
                    new ComboBoxAttachment(processor.ppManager.valueTreeState, parameter->paramID, *comboBox));

                components.add(comboBox);
                height += comboBoxHeight;
            }

            //======================================

            Label* label;
            labels.add(label = new Label(parameter->name, parameter->name));
            label->attachToComponent(components.getLast(), true);
            addAndMakeVisible(label);

            components.getLast()->setName(parameter->name);
            components.getLast()->setComponentID(parameter->paramID);
            addAndMakeVisible(components.getLast());
        }
    }

    addAndMakeVisible(&pitchShiftLabel);
    height += 20;

    //======================================

    height += components.size() * editorPadding;
    setSize(editorWidth, height);
    startTimer(50);
}

VibratoAudioProcessorEditor::~VibratoAudioProcessorEditor()
{
}

//==============================================================================

void VibratoAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(Colours::purple);
}

void VibratoAudioProcessorEditor::resized()
{
    Rectangle<int> layout = getLocalBounds().reduced(margin);
    layout = layout.removeFromRight(layout.getWidth() - labelWidth);

    for (int i = 0; i < components.size(); ++i) {
        if (Slider* slider = dynamic_cast<Slider*> (components[i]))
            components[i]->setBounds(layout.removeFromTop(sliderHeight));

        if (ToggleButton* button = dynamic_cast<ToggleButton*> (components[i]))
            components[i]->setBounds(layout.removeFromTop(buttonHeight));

        if (ComboBox* comboBox = dynamic_cast<ComboBox*> (components[i]))
            components[i]->setBounds(layout.removeFromTop(comboBoxHeight));

        layout = layout.removeFromBottom(layout.getHeight() - editorPadding);
    }

    pitchShiftLabel.setBounds(0, getBottom() - 20, getWidth(), 20);
}

//==============================================================================

void VibratoAudioProcessorEditor::timerCallback()
{
    updateUIcomponents();
}

void VibratoAudioProcessorEditor::updateUIcomponents()
{
    float minPitch = 0.0f;
    float maxPitch = 0.0f;
    float minSpeed = 1.0f;
    float maxSpeed = 1.0f;
    String pitchShiftText = "";

    float width = processor.widthSlider.getTargetValue();
    float frequency = processor.freqSlider.getTargetValue();
    int waveform = (int)processor.paramWaveform.getTargetValue();

    if (waveform == VibratoAudioProcessor::waveform::waveformSine) {
        minSpeed = 1.0f - M_PI * width * frequency;
        maxSpeed = 1.0f + M_PI * width * frequency;
    }
    else if (waveform == VibratoAudioProcessor::waveform::waveformTriangle) {
        minSpeed = 1.0f - 2.0f * width * frequency;
        maxSpeed = 1.0f + 2.0f * width * frequency;
    }
    else if (waveform == VibratoAudioProcessor::waveform::waveformSawtooth) {
        minSpeed = 1.0f - width * frequency;
        maxSpeed = 1.0f;
    }
    else {
        minSpeed = 1.0f;
        maxSpeed = 1.0f + width * frequency;
    }

    maxPitch = 12.0f * logf(maxSpeed) / logf(2.0f);

    if (minSpeed > 0.0f) {
        minPitch = 12.0f * logf(minSpeed) / logf(2.0f);
        pitchShiftText = String::formatted("Vibrato range: %+.2f to %+.2f semitones (speed %.3f to %.3f)",
            minPitch, maxPitch, minSpeed, maxSpeed);
    }
    else {
        pitchShiftText = String::formatted("Vibrato range: ----- to %+.2f semitones (speed %.3f to %.3f)",
            minPitch, maxPitch, minSpeed, maxSpeed);
    }

    pitchShiftLabel.setText(pitchShiftText, dontSendNotification);
}