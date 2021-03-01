#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

DelayAudioProcessorEditor::DelayAudioProcessorEditor(DelayAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    const Array<AudioProcessorParameter*> parameters = processor.getParameters();
    int counter = 0;

    int height = 2 * margin;
    
    for (int i = 0; i < parameters.size(); i++) {
        if (const AudioProcessorParameterWithID* parameter =
            dynamic_cast<AudioProcessorParameterWithID*> (parameters[i])) {

            if (processor.ppManager.types[i] == "Slider") {
                Slider* slider;
                sliders.add(slider = new Slider());
                slider->setTextValueSuffix(parameter->label);
                slider->setTextBoxStyle(Slider::TextBoxLeft, false, sliderTextEntryBoxWidth, sliderTextEntryBoxHeight);

                SliderAttachment* sliderAttachment;
                sliderAttachments.add(sliderAttachment = new SliderAttachment(processor.ppManager.valueTreeState, parameter->paramID, *slider));

                components.add(slider);
                height += sliderHeight;
            }

            //======================================

            else if (processor.ppManager.types[i] == "ToggleButton") {
                ToggleButton* button;
                toggles.add(button = new ToggleButton());
                button->setToggleState(parameter->getDefaultValue(), dontSendNotification);

                ButtonAttachment* aButtonAttachment;
                buttonAttachments.add(aButtonAttachment =
                    new ButtonAttachment(processor.ppManager.valueTreeState, parameter->paramID, *button));

                components.add(button);
                height += buttonHeight;
            }

            //======================================

            else if (processor.ppManager.types[i] == "ComboBox") {
                ComboBox* comboBox;
                comboBoxes.add(comboBox = new ComboBox());
                comboBox->setEditableText(false);
                comboBox->setJustificationType(Justification::left);
                comboBox->addItemList(processor.ppManager.boxLists[counter++], 1);

                ComboBoxAttachment* aComboBoxAttachment;
                comboBoxAttachments.add(aComboBoxAttachment =
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

    //======================================

    height += components.size() * editorPadding;
    setSize(editorWidth, height);
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
}

//==============================================================================

void DelayAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(Colours::purple);
}

void DelayAudioProcessorEditor::resized()
{
    Rectangle<int> layout = getLocalBounds().reduced(margin);
    layout = layout.removeFromRight(layout.getWidth() - labelWidth);

    for (int i = 0; i < components.size(); ++i) {
        if (Slider* aSlider = dynamic_cast<Slider*> (components[i])) components[i]->setBounds(layout.removeFromTop(sliderHeight));

        if (ToggleButton* aButton = dynamic_cast<ToggleButton*> (components[i])) components[i]->setBounds(layout.removeFromTop(buttonHeight));

        if (ComboBox* aComboBox = dynamic_cast<ComboBox*> (components[i])) components[i]->setBounds(layout.removeFromTop(comboBoxHeight));

        layout = layout.removeFromBottom(layout.getHeight() - editorPadding);
    }
}