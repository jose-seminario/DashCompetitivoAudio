#include "PluginProcessor.h"
#include "PluginEditor.h"

AmbienceGuardAudioProcessor::AmbienceGuardAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "DASH_AMBIENCE_GUARD_PARAMS", createParameterLayout())
{
    ambGuarParam_ = apvts_.getRawParameterValue(ParamIDs::ambGuar);
    distantParam_ = apvts_.getRawParameterValue(ParamIDs::distant);
    targetParam_  = apvts_.getRawParameterValue(ParamIDs::target);
    releaseParam_ = apvts_.getRawParameterValue(ParamIDs::release);
    detRelParam_  = apvts_.getRawParameterValue(ParamIDs::detRel);
    scLowParam_   = apvts_.getRawParameterValue(ParamIDs::scLow);
    scHighParam_  = apvts_.getRawParameterValue(ParamIDs::scHigh);
    bandModParam_ = apvts_.getRawParameterValue(ParamIDs::bandMod);
    bypassParam_  = apvts_.getRawParameterValue(ParamIDs::bypass);
}

juce::AudioProcessorValueTreeState::ParameterLayout AmbienceGuardAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Parámetros base de nuestro motor táctico compatibles con Equalizer APO:
    // Distant 1 | Target 0.388889 | Release 0 | DetRel 0.458333 | SCLow 0.2269 (350 Hz) | SCHigh 0.5293 (2.5 kHz) | BandMod 1 | AmbGuar 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::ambGuar, 1 },
        ParamIDs::ambGuar,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::distant, 1 },
        ParamIDs::distant,
        true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::target, 1 },
        ParamIDs::target,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.000001f),
        0.388889f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::release, 1 },
        ParamIDs::release,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::detRel, 1 },
        ParamIDs::detRel,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.000001f),
        0.458333f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::scLow, 1 },
        ParamIDs::scLow,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.2269f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::scHigh, 1 },
        ParamIDs::scHigh,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.5293f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::bandMod, 1 },
        ParamIDs::bandMod,
        true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::bypass, 1 },
        ParamIDs::bypass,
        false));

    return { params.begin(), params.end() };
}

void AmbienceGuardAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    guard_.prepare(
        sampleRate,
        juce::jmax(1, samplesPerBlock),
        juce::jmax(1, getTotalNumOutputChannels()));
}

void AmbienceGuardAudioProcessor::releaseResources()
{
    guard_.reset();
}

bool AmbienceGuardAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut || mainOut.isDisabled())
        return false;

    const int numChannels = mainOut.size();
    if (numChannels < 1 || numChannels > 16)
        return false;

    // Compatible con cadenas Mono, Estéreo, 5.1, 7.1, 7.1.4 Atmos y layouts discretos de hasta 16 canales
    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo()
        || mainOut == juce::AudioChannelSet::create5point1()
        || mainOut == juce::AudioChannelSet::create7point1()
        || mainOut == juce::AudioChannelSet::create7point1point4()
        || mainOut == juce::AudioChannelSet::discreteChannels(numChannels);
}

void AmbienceGuardAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    dash::core::AmbienceParameters dspParams {};
    dspParams.ambGuar = ambGuarParam_->load(std::memory_order_relaxed);
    dspParams.distant = (distantParam_->load(std::memory_order_relaxed) >= 0.5f);
    dspParams.target  = targetParam_->load(std::memory_order_relaxed);
    dspParams.release = releaseParam_->load(std::memory_order_relaxed);
    dspParams.detRel  = detRelParam_->load(std::memory_order_relaxed);
    dspParams.scLow   = scLowParam_->load(std::memory_order_relaxed);
    dspParams.scHigh  = scHighParam_->load(std::memory_order_relaxed);
    dspParams.bandMod = (bandModParam_->load(std::memory_order_relaxed) >= 0.5f);
    dspParams.bypass  = (bypassParam_->load(std::memory_order_relaxed) >= 0.5f);

    guard_.updateParameters(dspParams);
    guard_.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());

    meterHpHz_.store(guard_.getCurrentHpHz(), std::memory_order_relaxed);
    meterLpHz_.store(guard_.getCurrentLpHz(), std::memory_order_relaxed);
    meterGainDb_.store(guard_.getCurrentGainDb(), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* AmbienceGuardAudioProcessor::createEditor()
{
    return new AmbienceGuardAudioProcessorEditor(*this);
}

void AmbienceGuardAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto state = apvts_.copyState();
    if (std::unique_ptr<juce::XmlElement> xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void AmbienceGuardAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(apvts_.state.getType()))
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmbienceGuardAudioProcessor();
}
