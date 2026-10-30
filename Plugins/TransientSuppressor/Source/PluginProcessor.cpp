#include "PluginProcessor.h"
#include "PluginEditor.h"

TransientSuppressorAudioProcessor::TransientSuppressorAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "DASH_TRANSIENT_SUPPRESSOR_PARAMS", createParameterLayout())
{
    gunParam_     = apvts_.getRawParameterValue(ParamIDs::gun);
    stepsParam_   = apvts_.getRawParameterValue(ParamIDs::steps);
    frontParam_   = apvts_.getRawParameterValue(ParamIDs::front);
    restoreParam_ = apvts_.getRawParameterValue(ParamIDs::restore);
    releaseParam_ = apvts_.getRawParameterValue(ParamIDs::release);
    bypassParam_  = apvts_.getRawParameterValue(ParamIDs::bypass);
}

juce::AudioProcessorValueTreeState::ParameterLayout TransientSuppressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Parámetros auditados exactos para Equalizer APO:
    // VSTPlugin: Library TransientSuppressor.dll Gun 0.450000 Steps 0 Front 1 Restore 0 Bypass 0
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::gun, 1 },
        ParamIDs::gun,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.45f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::steps, 1 },
        ParamIDs::steps,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::front, 1 },
        ParamIDs::front,
        true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::restore, 1 },
        ParamIDs::restore,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::release, 1 },
        ParamIDs::release,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.35f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::bypass, 1 },
        ParamIDs::bypass,
        false));

    return { params.begin(), params.end() };
}

void TransientSuppressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock));
    spec.numChannels      = static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels()));

    const auto channelSet = getBusesLayout().getMainOutputChannelSet();
    int centerIdx = channelSet.getChannelIndexForType(juce::AudioChannelSet::centre);
    if (centerIdx < 0)
        centerIdx = 2; // Fallback estándar Windows WAVEFORMATEXTENSIBLE 5.1/7.1 (L=0, R=1, C=2)

    suppressor_.prepare(spec, centerIdx);
    activeChannels_.store(getTotalNumOutputChannels(), std::memory_order_relaxed);
}

void TransientSuppressorAudioProcessor::releaseResources()
{
    suppressor_.reset();
}

bool TransientSuppressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut || mainOut.isDisabled())
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo()
        || mainOut == juce::AudioChannelSet::create5point1()
        || mainOut == juce::AudioChannelSet::create7point1();
}

void TransientSuppressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    dash::dsp::TransientSuppressorParams dspParams {};
    dspParams.gun     = gunParam_->load(std::memory_order_relaxed);
    dspParams.steps   = stepsParam_->load(std::memory_order_relaxed);
    dspParams.front   = (frontParam_->load(std::memory_order_relaxed) >= 0.5f);
    dspParams.restore = restoreParam_->load(std::memory_order_relaxed);
    dspParams.release = releaseParam_->load(std::memory_order_relaxed);
    dspParams.bypass  = (bypassParam_->load(std::memory_order_relaxed) >= 0.5f);

    suppressor_.updateParameters(dspParams);
    suppressor_.processBlock(buffer);

    meterReductionDb_.store(suppressor_.getLastReductionDb(), std::memory_order_relaxed);
    activeChannels_.store(buffer.getNumChannels(), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* TransientSuppressorAudioProcessor::createEditor()
{
    return new TransientSuppressorAudioProcessorEditor(*this);
}

void TransientSuppressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto state = apvts_.copyState();
    if (std::unique_ptr<juce::XmlElement> xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void TransientSuppressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(apvts_.state.getType()))
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TransientSuppressorAudioProcessor();
}
