#include "PluginProcessor.h"
#include "PluginEditor.h"

LoudnessCompressorAudioProcessor::LoudnessCompressorAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "DASH_LOUDNESS_COMPRESSOR_PARAMS", createParameterLayout())
{
    amountParam_  = apvts_.getRawParameterValue(ParamIDs::amount);
    targetParam_  = apvts_.getRawParameterValue(ParamIDs::target);
    releaseParam_ = apvts_.getRawParameterValue(ParamIDs::release);
    detRelParam_  = apvts_.getRawParameterValue(ParamIDs::detRel);
    limRelParam_  = apvts_.getRawParameterValue(ParamIDs::limRel);
    scLowParam_   = apvts_.getRawParameterValue(ParamIDs::scLow);
    scHighParam_  = apvts_.getRawParameterValue(ParamIDs::scHigh);
    liftParam_    = apvts_.getRawParameterValue(ParamIDs::lift);
    gateParam_    = apvts_.getRawParameterValue(ParamIDs::gate);
    bypassParam_  = apvts_.getRawParameterValue(ParamIDs::bypass);
}

juce::AudioProcessorValueTreeState::ParameterLayout LoudnessCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Los nombres coinciden exactamente con los identificadores esperados por Equalizer APO:
    // Amount 0.750000 | Target 0.277778 (-18 LUFS en [-28..+8]) | Release 0.5 | DetRel 0.583333 | LimRel 0.612903 | SCLow 0 | SCHigh 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::amount, 1 },
        ParamIDs::amount,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.75f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::target, 1 },
        ParamIDs::target,
        juce::NormalisableRange<float>(-28.0f, 8.0f, 0.01f),
        -18.0f,
        juce::AudioParameterFloatAttributes().withLabel("LUFS")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::release, 1 },
        ParamIDs::release,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.50f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::detRel, 1 },
        ParamIDs::detRel,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.583333f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::limRel, 1 },
        ParamIDs::limRel,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.612903f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::scLow, 1 },
        ParamIDs::scLow,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::scHigh, 1 },
        ParamIDs::scHigh,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::lift, 1 },
        ParamIDs::lift,
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f),
        9.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::gate, 1 },
        ParamIDs::gate,
        juce::NormalisableRange<float>(-80.0f, -35.0f, 0.1f),
        -62.0f,
        juce::AudioParameterFloatAttributes().withLabel("LUFS")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::bypass, 1 },
        ParamIDs::bypass,
        false));

    return { params.begin(), params.end() };
}

void LoudnessCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock));
    spec.numChannels      = static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels()));

    compressor_.prepare(spec);
}

void LoudnessCompressorAudioProcessor::releaseResources()
{
    compressor_.reset();
}

bool LoudnessCompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut || mainOut.isDisabled())
        return false;

    // Compatible con cadenas Estéreo, 5.1 y 7.1 Surround (antes de la convolución HeSuVi)
    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo()
        || mainOut == juce::AudioChannelSet::create5point1()
        || mainOut == juce::AudioChannelSet::create7point1();
}

void LoudnessCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    dash::dsp::CompressorParameters dspParams {};
    dspParams.amount      = amountParam_->load(std::memory_order_relaxed);
    dspParams.targetLufs  = targetParam_->load(std::memory_order_relaxed);
    dspParams.releaseNorm = releaseParam_->load(std::memory_order_relaxed);
    dspParams.detRelNorm  = detRelParam_->load(std::memory_order_relaxed);
    dspParams.limRelNorm  = limRelParam_->load(std::memory_order_relaxed);
    dspParams.scLowNorm   = scLowParam_->load(std::memory_order_relaxed);
    dspParams.scHighNorm  = scHighParam_->load(std::memory_order_relaxed);
    dspParams.maxLiftDb   = liftParam_->load(std::memory_order_relaxed);
    dspParams.gateFloorDb = gateParam_->load(std::memory_order_relaxed);
    dspParams.bypass      = (bypassParam_->load(std::memory_order_relaxed) >= 0.5f);

    compressor_.updateParameters(dspParams);
    compressor_.processBlock(buffer);

    meterLufs_.store(compressor_.getCurrentLufs(), std::memory_order_relaxed);
    meterGainDb_.store(compressor_.getCurrentGainDb(), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* LoudnessCompressorAudioProcessor::createEditor()
{
    return new LoudnessCompressorAudioProcessorEditor(*this);
}

void LoudnessCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto state = apvts_.copyState();
    if (std::unique_ptr<juce::XmlElement> xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void LoudnessCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(apvts_.state.getType()))
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoudnessCompressorAudioProcessor();
}
