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
// Pestaña 1: "ECUALIZADOR" (Preamp + Reset a la izquierda, 6 Faders verticales ±18 dB a la derecha)
// ==============================================================================
class MainComponent::EqualizerTabComponent final : public juce::Component
{
public:
    std::function<void(float)>         onPreampChanged;
    std::function<void(size_t, float)> onBandChanged;
    std::function<void()>              onResetClicked;

    EqualizerTabComponent()
    {
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

        auto leftInner = leftCardBounds_.reduced(12, 14);
        preampTitle_.setBounds(leftInner.removeFromTop(22));
        preampSubLabel_.setBounds(leftInner.removeFromTop(18));
        leftInner.removeFromTop(6);

        auto resetArea = leftInner.removeFromBottom(34);
        leftInner.removeFromBottom(10);
        preampSlider_.setBounds(leftInner);
        resetButton_.setBounds(resetArea.reduced(4, 0));

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
        slider.setRange(
            static_cast<double>(dash::core::kMinEqGainDb),
            static_cast<double>(dash::core::kMaxEqGainDb),
            0.1);
        slider.setValue(0.0, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.setTextValueSuffix(" dB");
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 24);
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
// Pestaña 2: "AJUSTES" (Salida, Bypass, Telemetría y Auto-Configurar APO)
// ==============================================================================
class MainComponent::SettingsTabComponent final : public juce::Component
{
public:
    std::function<void(int)>  onOutputDeviceSelected;
    std::function<void(bool)> onBypassToggled;
    std::function<void()>     onAutoConfigApoClicked;

    SettingsTabComponent()
    {
        // 1. Selector de Dispositivo de Salida
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

        // 2. Control Maestro (Bypass)
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

        // 3. Telemetría en Tiempo Real
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

        // 4. Integración Directa con Equalizer APO
        apoHeader_.setText(juce::String::fromUTF8("Integración con Equalizer APO"), juce::dontSendNotification);
        apoHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.5f).withStyle("Bold")));
        apoHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(apoHeader_);

        autoConfigButton_.setButtonText(juce::String::fromUTF8("Auto-Configurar APO"));
        autoConfigButton_.onClick = [this] {
            if (onAutoConfigApoClicked)
                onAutoConfigApoClicked();
        };
        addAndMakeVisible(autoConfigButton_);

        autoConfigStatusLabel_.setText(
            juce::String::fromUTF8("Escribe las 6 bandas tácticas y el Preamp directamente en config.txt de Equalizer APO."),
            juce::dontSendNotification);
        autoConfigStatusLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
        autoConfigStatusLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(autoConfigStatusLabel_);
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

    void setAutoConfigStatus(const juce::String& text, bool isError)
    {
        autoConfigStatusLabel_.setText(text, juce::dontSendNotification);
        autoConfigStatusLabel_.setColour(
            juce::Label::textColourId,
            isError ? DashLookAndFeel::kAccentCoralLight : DashLookAndFeel::kSuccessEmerald);
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
        auto area = getLocalBounds().reduced(24, 16);
        const int maxCardWidth = juce::jmin(700, area.getWidth());
        auto column = area.withSizeKeepingCentre(maxCardWidth, area.getHeight());

        cards_[0] = column.removeFromTop(98);
        column.removeFromTop(12);
        cards_[1] = column.removeFromTop(88);
        column.removeFromTop(12);
        cards_[2] = column.removeFromTop(94);
        column.removeFromTop(12);
        cards_[3] = column.removeFromTop(110);

        auto c0 = cards_[0].reduced(18, 12);
        deviceHeader_.setBounds(c0.removeFromTop(22));
        c0.removeFromTop(8);
        outputDeviceBox_.setBounds(c0.removeFromTop(34));

        auto c1 = cards_[1].reduced(18, 12);
        bypassHeader_.setBounds(c1.removeFromTop(22));
        c1.removeFromTop(6);
        bypassToggle_.setBounds(c1.removeFromTop(30));

        auto c2 = cards_[2].reduced(18, 12);
        telemetryHeader_.setBounds(c2.removeFromTop(22));
        c2.removeFromTop(10);
        const int halfW = c2.getWidth() / 2;
        cpuLabel_.setBounds(c2.removeFromLeft(halfW));
        latencyLabel_.setBounds(c2);

        auto c3 = cards_[3].reduced(18, 12);
        apoHeader_.setBounds(c3.removeFromTop(22));
        c3.removeFromTop(8);
        autoConfigButton_.setBounds(c3.removeFromTop(36));
        c3.removeFromTop(4);
        autoConfigStatusLabel_.setBounds(c3.removeFromTop(20));
    }

private:
    std::array<juce::Rectangle<int>, 4> cards_;

    juce::Label        deviceHeader_;
    juce::ComboBox     outputDeviceBox_;

    juce::Label        bypassHeader_;
    juce::ToggleButton bypassToggle_;

    juce::Label        telemetryHeader_;
    juce::Label        cpuLabel_;
    juce::Label        latencyLabel_;

    juce::Label        apoHeader_;
    juce::TextButton   autoConfigButton_;
    juce::Label        autoConfigStatusLabel_;
};

// ==============================================================================
// Pantalla Bloqueante (Onboarding): Detección y Tutorial de Equalizer APO
// ==============================================================================
class MainComponent::ApoOnboardingOverlay final : public juce::Component
{
public:
    std::function<void()> onVerifyRequested;

    ApoOnboardingOverlay()
    {
        titleLabel_.setText(
            juce::String::fromUTF8("EQUALIZER APO NO DETECTADO"),
            juce::dontSendNotification);
        titleLabel_.setJustificationType(juce::Justification::centred);
        titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(18.0f).withStyle("Bold")));
        titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyanGlow);
        addAndMakeVisible(titleLabel_);

        descLabel_.setText(
            juce::String::fromUTF8("Para usar este ecualizador táctico y procesar el audio nativo del juego, necesitas tener instalado Equalizer APO en Windows."),
            juce::dontSendNotification);
        descLabel_.setJustificationType(juce::Justification::centred);
        descLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f)));
        descLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(descLabel_);

        stepsHeader_.setText(
            juce::String::fromUTF8("Tutorial de Instalación en 3 Pasos:"),
            juce::dontSendNotification);
        stepsHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
        stepsHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(stepsHeader_);

        step1Label_.setText(
            juce::String::fromUTF8("1. Descarga e instala Equalizer APO (gratuito y de código abierto)."),
            juce::dontSendNotification);
        step1Label_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f)));
        step1Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(step1Label_);

        step2Label_.setText(
            juce::String::fromUTF8("2. En la ventana del Configurator que abre el instalador, marca tus auriculares o altavoces."),
            juce::dontSendNotification);
        step2Label_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f)));
        step2Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(step2Label_);

        step3Label_.setText(
            juce::String::fromUTF8("3. Reinicia tu PC para que Windows cargue el controlador de audio de APO."),
            juce::dontSendNotification);
        step3Label_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f)));
        step3Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(step3Label_);

        downloadButton_.setButtonText(juce::String::fromUTF8("Descargar APO (Web Oficial)"));
        downloadButton_.onClick = [] {
            juce::URL("https://sourceforge.net/projects/equalizerapo/").launchInDefaultBrowser();
        };
        addAndMakeVisible(downloadButton_);

        verifyButton_.setButtonText(juce::String::fromUTF8("Ya lo instalé, verificar"));
        verifyButton_.onClick = [this] {
            if (onVerifyRequested)
                onVerifyRequested();
        };
        addAndMakeVisible(verifyButton_);

        statusLabel_.setText({}, juce::dontSendNotification);
        statusLabel_.setJustificationType(juce::Justification::centred);
        statusLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f).withStyle("Bold")));
        addAndMakeVisible(statusLabel_);
    }

    void setStatusMessage(const juce::String& message, bool isError)
    {
        statusLabel_.setText(message, juce::dontSendNotification);
        statusLabel_.setColour(
            juce::Label::textColourId,
            isError ? DashLookAndFeel::kAccentCoralLight : DashLookAndFeel::kSuccessEmerald);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xee0e1117));

        if (!cardBounds_.isEmpty())
        {
            g.setColour(DashLookAndFeel::kBgCard);
            g.fillRoundedRectangle(cardBounds_.toFloat(), 12.0f);

            g.setColour(DashLookAndFeel::kBorderSubtle);
            g.drawRoundedRectangle(cardBounds_.toFloat().reduced(0.5f), 12.0f, 1.2f);

            g.setColour(DashLookAndFeel::kAccentCyanGlow);
            g.fillRoundedRectangle(cardBounds_.getX() + 16.0f, cardBounds_.getY(), cardBounds_.getWidth() - 32.0f, 3.0f, 1.5f);
        }

        if (!tutorialBounds_.isEmpty())
        {
            g.setColour(DashLookAndFeel::kBgCardElevated);
            g.fillRoundedRectangle(tutorialBounds_.toFloat(), 8.0f);
            g.setColour(DashLookAndFeel::kBorderSubtle);
            g.drawRoundedRectangle(tutorialBounds_.toFloat().reduced(0.5f), 8.0f, 1.0f);
        }
    }

    void resized() override
    {
        const int cardW = juce::jlimit(420, 680, getWidth() - 48);
        const int cardH = juce::jlimit(380, 480, getHeight() - 40);

        cardBounds_ = getLocalBounds().withSizeKeepingCentre(cardW, cardH);
        auto inner = cardBounds_.reduced(24, 22);

        titleLabel_.setBounds(inner.removeFromTop(26));
        inner.removeFromTop(8);
        descLabel_.setBounds(inner.removeFromTop(38));
        inner.removeFromTop(14);

        tutorialBounds_ = inner.removeFromTop(140);
        auto tutInner = tutorialBounds_.reduced(16, 12);
        stepsHeader_.setBounds(tutInner.removeFromTop(22));
        tutInner.removeFromTop(6);
        step1Label_.setBounds(tutInner.removeFromTop(24));
        step2Label_.setBounds(tutInner.removeFromTop(24));
        step3Label_.setBounds(tutInner.removeFromTop(24));

        inner.removeFromTop(18);

        const int btnGap = 16;
        const int btnW = (inner.getWidth() - btnGap) / 2;
        auto btnArea = inner.removeFromTop(38);
        downloadButton_.setBounds(btnArea.removeFromLeft(btnW));
        btnArea.removeFromLeft(btnGap);
        verifyButton_.setBounds(btnArea);

        inner.removeFromTop(10);
        statusLabel_.setBounds(inner.removeFromTop(24));
    }

private:
    juce::Rectangle<int> cardBounds_;
    juce::Rectangle<int> tutorialBounds_;

    juce::Label      titleLabel_;
    juce::Label      descLabel_;
    juce::Label      stepsHeader_;
    juce::Label      step1Label_;
    juce::Label      step2Label_;
    juce::Label      step3Label_;

    juce::TextButton downloadButton_;
    juce::TextButton verifyButton_;
    juce::Label      statusLabel_;
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
    graphParams_.limiter.ceilingDb = dash::core::kLimiterCeilingDb;
    graphParams_.limiter.bypass    = false;
    graphParams_.bypassAll         = false;

    audioGraph_ = std::make_unique<dash::core::AudioGraph>();
    audioGraph_->prepare(kDefaultSampleRate, kDefaultBufferSize, 2);
    audioGraph_->updateParameters(graphParams_);

    audioBackend_ = std::make_unique<dash::audio::JuceWasapiAudioBackend>();
    audioBackend_->initialize();

    // 2. Construir pestañas de la interfaz
    equalizerTab_ = std::make_unique<EqualizerTabComponent>();
    settingsTab_  = std::make_unique<SettingsTabComponent>();

    equalizerTab_->onPreampChanged = [this](float preampDb) {
        graphParams_.preampDb = juce::jlimit(
            dash::core::kMinEqGainDb,
            dash::core::kMaxEqGainDb,
            preampDb);
        syncDspParameters();
        scheduleDebouncedApoSync();
    };

    equalizerTab_->onBandChanged = [this](size_t bandIdx, float gainDb) {
        if (bandIdx < graphParams_.bandGainsDb.size())
        {
            graphParams_.bandGainsDb[bandIdx] = juce::jlimit(
                dash::core::kMinEqGainDb,
                dash::core::kMaxEqGainDb,
                gainDb);
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

    settingsTab_->onAutoConfigApoClicked = [this] {
        std::string err;
        if (executeApoSync(&err))
        {
            settingsTab_->setAutoConfigStatus(
                juce::String::fromUTF8("¡APO auto-configurado con éxito con las 6 bandas tácticas!"),
                false);
        }
        else
        {
            const juce::String msg = err.empty()
                ? juce::String::fromUTF8("No se pudo escribir config.txt. Verifica que Equalizer APO esté instalado.")
                : juce::String::fromUTF8(err.c_str());
            settingsTab_->setAutoConfigStatus(msg, true);
        }
    };

    tabs_.setTabBarDepth(44);
    tabs_.setOutline(0);
    tabs_.addTab("ECUALIZADOR", DashLookAndFeel::kBgDark, equalizerTab_.get(), false);
    tabs_.addTab("AJUSTES",     DashLookAndFeel::kBgDark, settingsTab_.get(),  false);
    addAndMakeVisible(tabs_);

    // 3. Crear overlay de detección/bloqueo de Equalizer APO
    apoOverlay_ = std::make_unique<ApoOnboardingOverlay>();
    apoOverlay_->onVerifyRequested = [this] {
        checkApoInstallation(true);
    };
    addChildComponent(apoOverlay_.get());

    // 4. Detectar dispositivos de salida estéreo e iniciar motor WASAPI
    populateOutputDevices();
    restartAudioStream();

    // 5. Verificar presencia de APO y bloquear interfaz si no está instalado
    checkApoInstallation(false);

    setSize(960, 580);
    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();

    if (audioBackend_)
        audioBackend_->shutdown();

    if (dash::audio::ApoConfigManager::isApoInstalled())
    {
        const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
        if (dash::audio::ApoConfigManager::hasManagedSection(apoConfigPath))
            dash::audio::ApoConfigManager::removeManagedSection(apoConfigPath);
    }

    tabs_.clearTabs();
    setLookAndFeel(nullptr);
}

void MainComponent::checkApoInstallation(bool userInitiated)
{
    const bool apoInstalled = dash::audio::ApoConfigManager::isApoInstalled();

    if (apoInstalled)
    {
        if (apoOverlay_)
            apoOverlay_->setVisible(false);
        tabs_.setVisible(true);
        executeApoSync();
    }
    else
    {
        tabs_.setVisible(false);
        if (apoOverlay_)
        {
            apoOverlay_->setVisible(true);
            apoOverlay_->toFront(true);
            if (userInitiated)
            {
                apoOverlay_->setStatusMessage(
                    juce::String::fromUTF8("Equalizer APO aún no fue detectado. Asegúrate de instalarlo y reiniciar."),
                    true);
            }
            else
            {
                apoOverlay_->setStatusMessage({}, false);
            }
        }
        resized();
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);
}

void MainComponent::resized()
{
    tabs_.setBounds(getLocalBounds());

    if (apoOverlay_ && apoOverlay_->isVisible())
    {
        apoOverlay_->setBounds(getLocalBounds());
        apoOverlay_->toFront(true);
    }
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
    if (!dash::audio::ApoConfigManager::isApoInstalled())
        return;

    needsApoSync_    = true;
    debounceCounter_ = 0;
}

bool MainComponent::executeApoSync(std::string* outError)
{
    if (!dash::audio::ApoConfigManager::isApoInstalled())
    {
        if (outError)
            *outError = "Equalizer APO no está instalado en el sistema.";
        return false;
    }

    const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
    std::error_code ec;
    auto parentDir = apoConfigPath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec))
    {
        std::filesystem::create_directories(parentDir, ec);
    }

    dash::audio::ApoChainConfig apoCfg;
    apoCfg.targetEndpointGuid = selectedEndpointGuid_;
    apoCfg.preampDb           = static_cast<double>(graphParams_.preampDb);
    apoCfg.bypass             = graphParams_.bypassAll;

    for (size_t i = 0; i < graphParams_.bandGainsDb.size(); ++i)
        apoCfg.bandGainsDb[i] = static_cast<double>(graphParams_.bandGainsDb[i]);

    const std::string directives = dash::audio::ApoConfigInjector::buildConfigContent(apoCfg, apoConfigPath);
    std::string err;
    const auto res = dash::audio::ApoConfigManager::injectManagedSection(apoConfigPath, directives, &err);
    if (outError)
        *outError = err;

    return res == dash::audio::ApoConfigResult::Success;
}

} // namespace dash::ui
