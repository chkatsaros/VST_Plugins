#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
using Parameter = AudioProcessorValueTreeState::Parameter;

//==============================================================================

class PluginParametersManager
{
public:
    PluginParametersManager(AudioProcessor& p) : valueTreeState(p, nullptr)
    {
    }

    AudioProcessorValueTreeState valueTreeState;
    StringArray types;
    Array<StringArray> boxLists;
};

//==============================================================================

class PluginParameter
    : public LinearSmoothedValue<float>
    , public AudioProcessorValueTreeState::Listener
{
protected:
    PluginParameter(PluginParametersManager& parametersManager,
        const std::function<float(float)> callback = nullptr)
        : ppManager(parametersManager)
        , callback(callback)
    {
    }

public:
    void updateValue(float value)
    {
        if (callback == nullptr)
            setCurrentAndTargetValue(value);
        else
            setCurrentAndTargetValue(callback(value));
    }

    void parameterChanged(const String& parameterID, float newValue) override
    {
        updateValue(newValue);
    }

    PluginParametersManager& ppManager;
    std::function<float(float)> callback;
    String parameter;
};

//==============================================================================

class PluginParameterSlider : public PluginParameter
{
protected:
    PluginParameterSlider(PluginParametersManager& parametersManager,
        const String& name,
        const String& label,
        const float min,
        const float max,
        const float defaultValue,
        const std::function<float(float)> callback,
        const bool log)
        : PluginParameter(parametersManager, callback)
        , name(name)
        , label(label)
        , min(min)
        , max(max)
        , defaultValue(defaultValue)
    {
        parameter = name.removeCharacters(" ").toLowerCase();
        parametersManager.types.add("Slider");

        NormalisableRange<float> range(min, max);
        if (log)
            range.setSkewForCentre(sqrt(min * max));

        parametersManager.valueTreeState.createAndAddParameter(std::make_unique<Parameter>
            (parameter, name, label, range, defaultValue,
                [](float value) { return String(value, 2); },
                [](const String& text) { return text.getFloatValue(); })
        );

        parametersManager.valueTreeState.addParameterListener(parameter, this);
        updateValue(defaultValue);
    }

public:
    const String& name;
    const String& label;
    const float min;
    const float max;
    const float defaultValue;
};

//======================================

class PluginParameterLinSlider : public PluginParameterSlider
{
public:
    PluginParameterLinSlider(PluginParametersManager& parametersManager,
        const String& paramName,
        const String& labelText,
        const float minValue,
        const float maxValue,
        const float defaultValue,
        const std::function<float(float)> callback = nullptr)
        : PluginParameterSlider(parametersManager,
            paramName,
            labelText,
            minValue,
            maxValue,
            defaultValue,
            callback,
            false)
    {
    }
};

//======================================

class PluginParameterLogSlider : public PluginParameterSlider
{
public:
    PluginParameterLogSlider(PluginParametersManager& parametersManager,
        const String& paramName,
        const String& labelText,
        const float minValue,
        const float maxValue,
        const float defaultValue,
        const std::function<float(float)> callback = nullptr)
        : PluginParameterSlider(parametersManager,
            paramName,
            labelText,
            minValue,
            maxValue,
            defaultValue,
            callback,
            true)
    {
    }
};

//==============================================================================

class PluginParameterToggle : public PluginParameter
{
public:
    PluginParameterToggle(PluginParametersManager& parametersManager,
        const String& paramName,
        const bool defaultState = false,
        const std::function<float(float)> callback = nullptr)
        : PluginParameter(parametersManager, callback)
        , paramName(paramName)
        , defaultState(defaultState)
    {
        parameter = paramName.removeCharacters(" ").toLowerCase();
        parametersManager.types.add("ToggleButton");

        const StringArray toggleStates = { "False", "True" };
        NormalisableRange<float> range(0.0f, 1.0f, 1.0f);

        parametersManager.valueTreeState.createAndAddParameter(std::make_unique<Parameter>
            (parameter, paramName, "", range, (float)defaultState,
                [toggleStates](float value) { return toggleStates[(int)value]; },
                [toggleStates](const String& text) { return toggleStates.indexOf(text); })
        );

        parametersManager.valueTreeState.addParameterListener(parameter, this);
        updateValue((float)defaultState);
    }

    const String& paramName;
    const bool defaultState;
};

//==============================================================================

class PluginParameterComboBox : public PluginParameter
{
public:
    PluginParameterComboBox(PluginParametersManager& parametersManager,
        const String& paramName,
        const StringArray items,
        const int defaultChoice = 0,
        const std::function<float(const float)> callback = nullptr)
        : PluginParameter(parametersManager, callback)
        , paramName(paramName)
        , items(items)
        , defaultChoice(defaultChoice)
    {
        parameter = paramName.removeCharacters(" ").toLowerCase();
        parametersManager.types.add("ComboBox");

        parametersManager.boxLists.add(items);
        NormalisableRange<float> range(0.0f, (float)items.size() - 1.0f, 1.0f);

        parametersManager.valueTreeState.createAndAddParameter(std::make_unique<Parameter>
            (parameter, paramName, "", range, (float)defaultChoice,
                [items](float value) { return items[(int)value]; },
                [items](const String& text) { return items.indexOf(text); })
        );

        parametersManager.valueTreeState.addParameterListener(parameter, this);
        updateValue((float)defaultChoice);
    }

    const String& paramName;
    const StringArray items;
    const int defaultChoice;
};