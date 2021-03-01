#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

DistortionAudioProcessorEditor::DistortionAudioProcessorEditor(DistortionAudioProcessor& p)
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
                    new SliderAttachment(processor.ppManager.apvts, parameter->paramID, *slider));

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
                    new ButtonAttachment(processor.ppManager.apvts, parameter->paramID, *button));

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
                    new ComboBoxAttachment(processor.ppManager.apvts, parameter->paramID, *comboBox));

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

    //======================================

    height += components.size() * editorPadding;
    setSize(editorWidth, height);
}

DistortionAudioProcessorEditor::~DistortionAudioProcessorEditor()
{
}

//==============================================================================

void DistortionAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(Colours::purple);
}

void DistortionAudioProcessorEditor::resized()
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
}