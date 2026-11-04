#include "MainComponent.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>

namespace dash::ui
{

namespace
{
    constexpr double kDefaultSampleRate = 48000.0;
    constexpr int    kDefaultBufferSize = 512;

    constexpr std::array<const char*, dash::core::ParametricEQProcessor::kMaxBands> kBandFreqLabels {
        "60 Hz",
        "150 Hz",
        "400 Hz",
        "1.5 kHz",
        "3.5 kHz",
        "8.0 kHz"
    };

    std::string wideToNarrowAscii(const std::wstring& wide)
    {
        return juce::String(wide.c_str()).toStdString();
    }
} // namespace

// ==============================================================================
// Pestaña 1: "ECUALIZADOR" (Preamp + Reset a la izquierda, 6 Faders verticales a la derecha)
// ==============================================================================
class MainComponent::EqualizerTabComponent final : public juce::Component
{
public:
    std::function<void(float)>         onPreampChanged;
    std::function<void(size_t, float)> onBandChanged;
    std::function<void()>              onResetClicked;

    EqualizerTabComponent()
    {
        // Configuración del canal izquierdo: Preamp + Reset
        preampTitle_.setText("Preamp", juce::dontSendNotification);
        preampTitle_.setJustificationType(juce::Justification::centred);
        preampTitle_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
        preampTitle_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(preampTitle_);

        preampSubLabel_.setText("Ganancia", juce::dontSendNotification);
        preampSubLabel_.setJustificationType(juce::Justification::centred);
        preampSubLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
        preampSubLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(preampSubLabel_);

        configureVerticalFader(preampSlider_);
        preampSlider_.onValueChange = [this] {
            if (onPreampChanged)
                onPreampChanged(static_cast<float>(preampSlider_.getValue()));
        };
        addAndMakeVisible(preampSlider_);

        resetButton_.setButtonText("Reset");
        resetButton_.onClick = [this] {
            if (onResetClicked)
                onResetClicked();
        };
        addAndMakeVisible(resetButton_);

        // Configuración de los 6 faders verticales tácticos
        for (size_t i = 0; i < kNumBands; ++i)
        {
            bandTitles_[i].setText(
                juce::String::fromUTF8(dash::core::ParametricEQProcessor::kBandNames[i]),
                juce::dontSendNotification);
            bandTitles_[i].setJustificationType(juce::Justification::centred);
            bandTitles_[i].setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.5f).withStyle("Bold")));
            bandTitles_[i].setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
            addAndMakeVisible(bandTitles_[i]);

            bandFreqLabels_[i].setText(kBandFreqLabels[i], juce::dontSendNotification);
            bandFreqLabels_[i].setJustificationType(juce::Justification::centred);
            bandFreqLabels_[i].setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
            bandFreqLabels_[i].setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyanGlow);
            addAndMakeVisible(bandFreqLabels_[i]);

            configureVerticalFader(bandSliders_[i]);
            bandSliders_[i].onValueChange = [this, i] {
                if (onBandChanged)
                    onBandChanged(i, static_cast<float>(bandSliders_[i].getValue()));
            };
            addAndMakeVisible(bandSliders_[i]);
        }
    }

    void setValues(float preampDb, const std::array<float, dash::core::ParametricEQProcessor::kMaxBands>& gainsDb)
    {
        preampSlider_.setValue(static_cast<double>(preampDb), juce::dontSendNotification);
        for (size_t i = 0; i < kNumBands; ++i)
            bandSliders_[i].setValue(static_cast<double>(gainsDb[i]), juce::dontSendNotification);
    }

    void setBypassedVisualState(bool bypassed)
    {
        const float alpha = bypassed ? 0.45f : 1.0f;
        preampSlider_.setEnabled(!bypassed);
        preampSlider_.setAlpha(alpha);
        for (auto& s : bandSliders_)
        {
            s.setEnabled(!bypassed);
            s.setAlpha(alpha);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(DashLookAndFeel::kBgDark);

        if (!leftCardBounds_.isEmpty())
        {
            g.setColour(DashLookAndFeel::kBgCard);
            g.fillRoundedRectangle(leftCardBounds_.toFloat(), 10.0f);
            g.setColour(DashLookAndFeel::kBorderSubtle);
            g.drawRoundedRectangle(leftCardBounds_.toFloat().reduced(0.5f), 10.0f, 1.0f);
        }

        if (!rightCardBounds_.isEmpty())
        {
            g.setColour(DashLookAndFeel::kBgCard);
            g.fillRoundedRectangle(rightCardBounds_.toFloat(), 10.0f);
            g.setColour(DashLookAndFeel::kBorderSubtle);
            g.drawRoundedRectangle(rightCardBounds_.toFloat().reduced(0.5f), 10.0f, 1.0f);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(18, 16);
        const int leftWidth = juce::jlimit(125, 165, area.getWidth() / 6);

        leftCardBounds_ = area.removeFromLeft(leftWidth);
        area.removeFromLeft(14);
        rightCardBounds_ = area;

        // Layout de tarjeta izquierda: Preamp + botón Reset
        auto leftInner = leftCardBounds_.reduced(12, 14);
        preampTitle_.setBounds(leftInner.removeFromTop(22));
        preampSubLabel_.setBounds(leftInner.removeFromTop(18));
        leftInner.removeFromTop(6);

        auto resetArea = leftInner.removeFromBottom(34);
        leftInner.removeFromBottom(10);
        preampSlider_.setBounds(leftInner);
        resetButton_.setBounds(resetArea.reduced(4, 0));

        // Layout de consola derecha: 6 faders verticales grandes
        auto rightInner = rightCardBounds_.reduced(12, 14);
        const int stripWidth = rightInner.getWidth() / static_cast<int>(kNumBands);

        for (size_t i = 0; i < kNumBands; ++i)
        {
            auto strip = (i + 1 == kNumBands)
                ? rightInner
                : rightInner.removeFromLeft(stripWidth);

            auto stripInner = strip.reduced(4, 0);
            bandTitles_[i].setBounds(stripInner.removeFromTop(22));
            bandFreqLabels_[i].setBounds(stripInner.removeFromTop(18));
            stripInner.removeFromTop(6);
            stripInner.removeFromBottom(4);
            bandSliders_[i].setBounds(stripInner);
        }
    }

private:
    static constexpr size_t kNumBands = dash::core::ParametricEQProcessor::kMaxBands;

    static void configureVerticalFader(juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setRange(-12.0, 12.0, 0.1);
        slider.setValue(0.0, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.setTextValueSuffix(" dB");
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 24);
    }

    juce::Rectangle<int> leftCardBounds_;
    juce::Rectangle<int> rightCardBounds_;

    juce::Label      preampTitle_;
    juce::Label      preampSubLabel_;
    juce::Slider     preampSlider_;
    juce::TextButton resetButton_;

    std::array<juce::Label,  kNumBands> bandTitles_;
    std::array<juce::Label,  kNumBands> bandFreqLabels_;
    std::array<juce::Slider, kNumBands> bandSliders_;
};

// ==============================================================================
// Pestaña 2: "AJUSTES" (Selector de salida, Bypass global y Telemetría CPU/Latencia)
// ==============================================================================
class MainComponent::SettingsTabComponent final : public juce::Component
{
public:
    std::function<void(int)>  onOutputDeviceSelected;
    std::function<void(bool)> onBypassToggled;

    SettingsTabComponent()
    {
        deviceHeader_.setText(juce::String::fromUTF8("Dispositivo de Salida (Estéreo 2.0)"), juce::dontSendNotification);
        deviceHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.5f).withStyle("Bold")));
        deviceHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(deviceHeader_);

        outputDeviceBox_.setTextWhenNothingSelected(juce::String::fromUTF8("Salida Predeterminada de Windows"));
        outputDeviceBox_.onChange = [this] {
            if (onOutputDeviceSelected)
                onOutputDeviceSelected(outputDeviceBox_.getSelectedItemIndex());
        };
        addAndMakeVisible(outputDeviceBox_);

        bypassHeader_.setText("Control Maestro", juce::dontSendNotification);
        bypassHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.5f).withStyle("Bold")));
        bypassHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(bypassHeader_);

        bypassToggle_.setButtonText(juce::String::fromUTF8("Bypass Global (Desactivar Ecualizador)"));
        bypassToggle_.onClick = [this] {
            if (onBypassToggled)
                onBypassToggled(bypassToggle_.getToggleState());
        };
        addAndMakeVisible(bypassToggle_);

        telemetryHeader_.setText(juce::String::fromUTF8("Telemetría en Tiempo Real"), juce::dontSendNotification);
        telemetryHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.5f).withStyle("Bold")));
        telemetryHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(telemetryHeader_);

        cpuLabel_.setText("CPU: 0.0 %", juce::dontSendNotification);
        cpuLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(15.0f).withStyle("Bold")));
        cpuLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyanGlow);
        addAndMakeVisible(cpuLabel_);

        latencyLabel_.setText("Latencia: 0.0 ms", juce::dontSendNotification);
        latencyLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(15.0f).withStyle("Bold")));
        latencyLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyanGlow);
        addAndMakeVisible(latencyLabel_);
    }

    void setDeviceList(const juce::StringArray& names, int selectedIndex)
    {
        outputDeviceBox_.clear(juce::dontSendNotification);
        for (int i = 0; i < names.size(); ++i)
            outputDeviceBox_.addItem(names[i], i + 1);

        if (selectedIndex >= 0 && selectedIndex < names.size())
            outputDeviceBox_.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
    }

    void setBypassState(bool bypassed)
    {
        bypassToggle_.setToggleState(bypassed, juce::dontSendNotification);
    }

    void updateTelemetry(float cpuPercent, float latencyMs)
    {
        cpuLabel_.setText("CPU: " + juce::String(cpuPercent, 1) + " %", juce::dontSendNotification);
        latencyLabel_.setText("Latencia: " + juce::String(latencyMs, 1) + " ms", juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(DashLookAndFeel::kBgDark);

        for (const auto& card : cards_)
        {
            if (!card.isEmpty())
            {
                g.setColour(DashLookAndFeel::kBgCard);
                g.fillRoundedRectangle(card.toFloat(), 10.0f);
                g.setColour(DashLookAndFeel::kBorderSubtle);
                g.drawRoundedRectangle(card.toFloat().reduced(0.5f), 10.0f, 1.0f);
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(24, 20);
        const int maxCardWidth = juce::jmin(680, area.getWidth());
        auto column = area.withSizeKeepingCentre(maxCardWidth, area.getHeight());

        cards_[0] = column.removeFromTop(104);
        column.removeFromTop(16);
        cards_[1] = column.removeFromTop(96);
        column.removeFromTop(16);
        cards_[2] = column.removeFromTop(104);

        auto c0 = cards_[0].reduced(20, 16);
        deviceHeader_.setBounds(c0.removeFromTop(24));
        c0.removeFromTop(10);
        outputDeviceBox_.setBounds(c0.removeFromTop(36));

        auto c1 = cards_[1].reduced(20, 16);
        bypassHeader_.setBounds(c1.removeFromTop(24));
        c1.removeFromTop(8);
        bypassToggle_.setBounds(c1.removeFromTop(32));

        auto c2 = cards_[2].reduced(20, 16);
        telemetryHeader_.setBounds(c2.removeFromTop(24));
        c2.removeFromTop(12);
        const int halfW = c2.getWidth() / 2;
        cpuLabel_.setBounds(c2.removeFromLeft(halfW));
        latencyLabel_.setBounds(c2);
    }

private:
    std::array<juce::Rectangle<int>, 3> cards_;

    juce::Label        deviceHeader_;
    juce::ComboBox     outputDeviceBox_;

    juce::Label        bypassHeader_;
    juce::ToggleButton bypassToggle_;

    juce::Label        telemetryHeader_;
    juce::Label        cpuLabel_;
    juce::Label        latencyLabel_;
};

// ==============================================================================
// MainComponent
// ==============================================================================
MainComponent::MainComponent()
{
    setLookAndFeel(&dashLookAndFeel_);

    // 1. Inicializar parámetros estéreo y Limiter a -0.5 dBFS
    graphParams_.preampDb = 0.0f;
    graphParams_.bandGainsDb.fill(0.0f);
    graphParams_.limiter.ceilingDb = -0.5f;
    graphParams_.limiter.bypass    = false;
    graphParams_.bypassAll         = false;

    audioGraph_ = std::make_unique<dash::core::AudioGraph>();
    audioGraph_->prepare(kDefaultSampleRate, kDefaultBufferSize, 2);
    audioGraph_->updateParameters(graphParams_);

    audioBackend_ = std::make_unique<dash::audio::JuceWasapiAudioBackend>();
    audioBackend_->initialize();

    // 2. Construir las 2 únicas pestañas de la interfaz
    equalizerTab_ = std::make_unique<EqualizerTabComponent>();
    settingsTab_  = std::make_unique<SettingsTabComponent>();

    equalizerTab_->onPreampChanged = [this](float preampDb) {
        graphParams_.preampDb = juce::jlimit(-12.0f, 12.0f, preampDb);
        syncDspParameters();
        scheduleDebouncedApoSync();
    };

    equalizerTab_->onBandChanged = [this](size_t bandIdx, float gainDb) {
        if (bandIdx < graphParams_.bandGainsDb.size())
        {
            graphParams_.bandGainsDb[bandIdx] = juce::jlimit(-12.0f, 12.0f, gainDb);
            syncDspParameters();
            scheduleDebouncedApoSync();
        }
    };

    equalizerTab_->onResetClicked = [this] {
        resetAllEqAndPreamp();
    };

    settingsTab_->onOutputDeviceSelected = [this](int index) {
        if (index >= 0 && static_cast<size_t>(index) < outputEndpoints_.size())
        {
            const auto& ep = outputEndpoints_[static_cast<size_t>(index)];
            selectedDeviceName_   = ep.friendlyName;
            selectedEndpointGuid_ = wideToNarrowAscii(ep.endpointGuid);

            if (!ep.id.empty() && ep.id.front() == L'{')
                policyManager_.setDefaultEndpoint(ep.id);

            restartAudioStream();
            scheduleDebouncedApoSync();
        }
    };

    settingsTab_->onBypassToggled = [this](bool bypassed) {
        graphParams_.bypassAll = bypassed;
        if (audioBackend_)
            audioBackend_->setBypass(bypassed);
        equalizerTab_->setBypassedVisualState(bypassed);
        syncDspParameters();
        scheduleDebouncedApoSync();
    };

    tabs_.setTabBarDepth(44);
    tabs_.setOutline(0);
    tabs_.addTab("ECUALIZADOR", DashLookAndFeel::kBgDark, equalizerTab_.get(), false);
    tabs_.addTab("AJUSTES",     DashLookAndFeel::kBgDark, settingsTab_.get(),  false);
    addAndMakeVisible(tabs_);

    // 3. Detectar dispositivos de salida estéreo e iniciar motor WASAPI
    populateOutputDevices();
    restartAudioStream();
    scheduleDebouncedApoSync();

    setSize(960, 580);
    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();

    if (audioBackend_)
        audioBackend_->shutdown();

    const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
    if (dash::audio::ApoConfigManager::hasManagedSection(apoConfigPath))
        dash::audio::ApoConfigManager::removeManagedSection(apoConfigPath);

    tabs_.clearTabs();
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);
}

void MainComponent::resized()
{
    tabs_.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
    if (needsApoSync_)
    {
        if (++debounceCounter_ >= 2)
        {
            needsApoSync_    = false;
            debounceCounter_ = 0;
            executeApoSync();
        }
    }

    if (settingsTab_ && audioBackend_)
    {
        const auto state = audioBackend_->getState();
        const bool active = (state == dash::audio::AudioBackendState::Running ||
                             state == dash::audio::AudioBackendState::Bypassed);

        const float cpuPercent = active ? audioBackend_->getCpuUsage() : 0.0f;
        float latencyMs        = active ? audioBackend_->getCurrentLatencyMs() : 0.0f;
        if (active && latencyMs <= 0.0f)
            latencyMs = static_cast<float>((kDefaultBufferSize * 1000.0) / kDefaultSampleRate);

        settingsTab_->updateTelemetry(cpuPercent, latencyMs);
    }
}

void MainComponent::populateOutputDevices()
{
    outputEndpoints_.clear();
    juce::StringArray names;
    int defaultIdx = 0;

    const auto mmEndpoints = policyManager_.enumerateActiveRenderEndpoints();
    if (!mmEndpoints.empty())
    {
        for (size_t i = 0; i < mmEndpoints.size(); ++i)
        {
            OutputEndpointEntry entry;
            entry.id           = mmEndpoints[i].id;
            entry.friendlyName = mmEndpoints[i].friendlyName;
            entry.endpointGuid = mmEndpoints[i].endpointGuid;
            outputEndpoints_.push_back(entry);

            names.add(juce::String(entry.friendlyName.c_str()));
            if (mmEndpoints[i].isDefaultConsole || mmEndpoints[i].isDefaultMultimedia)
                defaultIdx = static_cast<int>(i);
        }
    }
    else if (audioBackend_)
    {
        const auto backendDevs = audioBackend_->enumerateRenderDevices();
        for (size_t i = 0; i < backendDevs.size(); ++i)
        {
            OutputEndpointEntry entry;
            entry.id           = backendDevs[i].id;
            entry.friendlyName = backendDevs[i].friendlyName;
            entry.endpointGuid.clear();
            outputEndpoints_.push_back(entry);

            names.add(juce::String(entry.friendlyName.c_str()));
            if (backendDevs[i].isDefault)
                defaultIdx = static_cast<int>(i);
        }
    }

    if (!outputEndpoints_.empty())
    {
        const auto& chosen = outputEndpoints_[static_cast<size_t>(defaultIdx)];
        selectedDeviceName_   = chosen.friendlyName;
        selectedEndpointGuid_ = wideToNarrowAscii(chosen.endpointGuid);
    }

    if (settingsTab_)
        settingsTab_->setDeviceList(names, outputEndpoints_.empty() ? -1 : defaultIdx);
}

void MainComponent::restartAudioStream()
{
    if (!audioBackend_ || !audioGraph_)
        return;

    audioGraph_->prepare(kDefaultSampleRate, kDefaultBufferSize, 2);
    audioGraph_->updateParameters(graphParams_);

    dash::audio::AudioBackendConfig cfg;
    cfg.deviceId         = selectedDeviceName_;
    cfg.sampleRate       = kDefaultSampleRate;
    cfg.bufferSize       = kDefaultBufferSize;
    cfg.numChannels      = 2;
    cfg.numInputChannels = 2;
    cfg.exclusiveMode    = false;

    audioBackend_->start(
        cfg,
        [this](float* const* channels, int numChannels, int numSamples) {
            if (audioGraph_)
                audioGraph_->process(channels, numChannels, numSamples);
        },
        nullptr);

    audioBackend_->setBypass(graphParams_.bypassAll);
}

void MainComponent::syncDspParameters()
{
    if (audioGraph_)
        audioGraph_->updateParameters(graphParams_);
}

void MainComponent::resetAllEqAndPreamp()
{
    graphParams_.preampDb = 0.0f;
    graphParams_.bandGainsDb.fill(0.0f);

    if (equalizerTab_)
        equalizerTab_->setValues(0.0f, graphParams_.bandGainsDb);

    syncDspParameters();
    scheduleDebouncedApoSync();
}

void MainComponent::scheduleDebouncedApoSync()
{
    needsApoSync_    = true;
    debounceCounter_ = 0;
}

void MainComponent::executeApoSync()
{
    const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
    std::error_code ec;
    if (!std::filesystem::exists(apoConfigPath.parent_path(), ec))
        return;

    dash::audio::ApoChainConfig apoCfg;
    apoCfg.targetEndpointGuid = selectedEndpointGuid_;
    apoCfg.preampDb           = static_cast<double>(graphParams_.preampDb);
    apoCfg.bypass             = graphParams_.bypassAll;

    for (size_t i = 0; i < graphParams_.bandGainsDb.size(); ++i)
        apoCfg.bandGainsDb[i] = static_cast<double>(graphParams_.bandGainsDb[i]);

    const std::string directives = dash::audio::ApoConfigInjector::buildConfigContent(apoCfg, apoConfigPath);
    dash::audio::ApoConfigManager::injectManagedSection(apoConfigPath, directives);
}

} // namespace dash::ui
