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
        "60 HZ",
        "150 HZ",
        "400 HZ",
        "1.5 KHZ",
        "3.5 KHZ",
        "8.0 KHZ"
    };

    std::string wideToNarrowAscii(const std::wstring& wide)
    {
        return juce::String(wide.c_str()).toStdString();
    }

    void styleHudFader(juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setRange(
            static_cast<double>(dash::core::kMinEqGainDb),
            static_cast<double>(dash::core::kMaxEqGainDb),
            0.1);
        slider.setValue(0.0, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.setTextValueSuffix(" DB");
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 22);
    }
} // namespace

// ==============================================================================
// 1. Pantalla Central HUD: Matriz de 6 Bandas Paramétricas (EqualizerHudView)
// ==============================================================================
class MainComponent::EqualizerHudView final : public juce::Component
{
public:
    std::function<void(size_t, float)> onBandChanged;

    EqualizerHudView()
    {
        matrixHeader_.setText(
            "// TACTICAL FREQUENCY MATRIX - 6-BAND BIQUAD [ESTÉREO]",
            juce::dontSendNotification);
        matrixHeader_.setJustificationType(juce::Justification::centredLeft);
        matrixHeader_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(13.5f).withStyle("Bold")));
        matrixHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyanGlow);
        addAndMakeVisible(matrixHeader_);

        for (size_t i = 0; i < kNumBands; ++i)
        {
            bandTitles_[i].setText(
                juce::String(dash::core::ParametricEQProcessor::kBandNames[i]).toUpperCase(),
                juce::dontSendNotification);
            bandTitles_[i].setJustificationType(juce::Justification::centred);
            bandTitles_[i].setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.5f).withStyle("Bold")));
            bandTitles_[i].setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
            addAndMakeVisible(bandTitles_[i]);

            bandFreqLabels_[i].setText(kBandFreqLabels[i], juce::dontSendNotification);
            bandFreqLabels_[i].setJustificationType(juce::Justification::centred);
            bandFreqLabels_[i].setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.5f).withStyle("Bold")));
            bandFreqLabels_[i].setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyan);
            addAndMakeVisible(bandFreqLabels_[i]);

            styleHudFader(bandSliders_[i]);
            bandSliders_[i].onValueChange = [this, i] {
                if (onBandChanged)
                    onBandChanged(i, static_cast<float>(bandSliders_[i].getValue()));
            };
            addAndMakeVisible(bandSliders_[i]);
        }
    }

    void setBandValues(const std::array<float, dash::core::ParametricEQProcessor::kMaxBands>& gainsDb)
    {
        for (size_t i = 0; i < kNumBands; ++i)
            bandSliders_[i].setValue(static_cast<double>(gainsDb[i]), juce::dontSendNotification);
    }

    void setBypassedVisualState(bool bypassed)
    {
        const float alpha = bypassed ? 0.35f : 1.0f;
        for (auto& s : bandSliders_)
        {
            s.setEnabled(!bypassed);
            s.setAlpha(alpha);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Marco cibernético central estilo HUD Gaming (como la imagen de referencia)
        DashLookAndFeel::drawHudPanel(
            g,
            bounds,
            16.0f,
            true,  // Brackets en las esquinas
            true,  // Scanlines de fondo
            DashLookAndFeel::kBgCard,
            DashLookAndFeel::kNeonCrimson,
            1.6f);

        // Alas decorativas en el borde superior del marco
        g.setColour(DashLookAndFeel::kNeonCrimsonGlow);
        const float midX = bounds.getCentreX();
        g.drawLine(midX - 90.0f, bounds.getY() + 2.0f, midX + 90.0f, bounds.getY() + 2.0f, 2.0f);
        g.drawLine(midX - 50.0f, bounds.getY() + 5.0f, midX + 50.0f, bounds.getY() + 5.0f, 1.2f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16, 14);

        matrixHeader_.setBounds(area.removeFromTop(24));
        area.removeFromTop(8);

        const int stripWidth = area.getWidth() / static_cast<int>(kNumBands);

        for (size_t i = 0; i < kNumBands; ++i)
        {
            auto strip = (i + 1 == kNumBands)
                ? area
                : area.removeFromLeft(stripWidth);

            auto stripInner = strip.reduced(4, 0);
            bandTitles_[i].setBounds(stripInner.removeFromTop(20));
            bandFreqLabels_[i].setBounds(stripInner.removeFromTop(18));
            stripInner.removeFromTop(6);
            stripInner.removeFromBottom(4);
            bandSliders_[i].setBounds(stripInner);
        }
    }

private:
    static constexpr size_t kNumBands = dash::core::ParametricEQProcessor::kMaxBands;

    juce::Label matrixHeader_;
    std::array<juce::Label,  kNumBands> bandTitles_;
    std::array<juce::Label,  kNumBands> bandFreqLabels_;
    std::array<juce::Slider, kNumBands> bandSliders_;
};

// ==============================================================================
// 2. Pantalla Central HUD: Ajustes de Sistema y Equalizer APO (SettingsHudView)
// ==============================================================================
class MainComponent::SettingsHudView final : public juce::Component
{
public:
    std::function<void(int)> onOutputDeviceSelected;
    std::function<void()>    onAutoConfigApoClicked;
    std::function<void()>    onRestoreApoClicked;

    SettingsHudView()
    {
        headerTitle_.setText("// AJUSTES DE SISTEMA Y CONTROLADOR EQUALIZER APO", juce::dontSendNotification);
        headerTitle_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(13.5f).withStyle("Bold")));
        headerTitle_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyanGlow);
        addAndMakeVisible(headerTitle_);

        // Dispositivo de Salida
        deviceCardTitle_.setText("// DISPOSITIVO DE SALIDA WASAPI (ESTÉREO 2.0)", juce::dontSendNotification);
        deviceCardTitle_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.5f).withStyle("Bold")));
        deviceCardTitle_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(deviceCardTitle_);

        outputDeviceBox_.setTextWhenNothingSelected("SALIDA PREDETERMINADA DE WINDOWS");
        outputDeviceBox_.onChange = [this] {
            if (onOutputDeviceSelected)
                onOutputDeviceSelected(outputDeviceBox_.getSelectedItemIndex());
        };
        addAndMakeVisible(outputDeviceBox_);

        // Motor Equalizer APO
        apoCardTitle_.setText("// INTEGRACIÓN DIRECTA CON EQUALIZER APO", juce::dontSendNotification);
        apoCardTitle_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.5f).withStyle("Bold")));
        apoCardTitle_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(apoCardTitle_);

        autoConfigButton_.setButtonText("// AUTO-CONFIGURAR EQUALIZER APO");
        autoConfigButton_.onClick = [this] {
            if (onAutoConfigApoClicked)
                onAutoConfigApoClicked();
        };
        addAndMakeVisible(autoConfigButton_);

        restoreApoButton_.setButtonText("// RESTAURAR APO ORIGINAL (.BAK)");
        restoreApoButton_.onClick = [this] {
            if (onRestoreApoClicked)
                onRestoreApoClicked();
        };
        addAndMakeVisible(restoreApoButton_);

        statusLabel_.setText(
            "Escribe las 6 bandas tácticas en config.txt o restaura la configuración original desde config.txt.bak.",
            juce::dontSendNotification);
        statusLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.5f)));
        statusLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(statusLabel_);
    }

    void setDeviceList(const juce::StringArray& names, int selectedIndex)
    {
        outputDeviceBox_.clear(juce::dontSendNotification);
        for (int i = 0; i < names.size(); ++i)
            outputDeviceBox_.addItem(names[i].toUpperCase(), i + 1);

        if (selectedIndex >= 0 && selectedIndex < names.size())
            outputDeviceBox_.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
    }

    void setStatus(const juce::String& text, bool isError)
    {
        statusLabel_.setText(text.toUpperCase(), juce::dontSendNotification);
        statusLabel_.setColour(
            juce::Label::textColourId,
            isError ? DashLookAndFeel::kNeonCrimsonGlow : DashLookAndFeel::kNeonEmerald);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        DashLookAndFeel::drawHudPanel(
            g,
            bounds,
            16.0f,
            true,
            true,
            DashLookAndFeel::kBgCard,
            DashLookAndFeel::kRubyBorder,
            1.4f);

        if (!deviceCardBounds_.isEmpty())
        {
            DashLookAndFeel::drawHudPanel(
                g,
                deviceCardBounds_.toFloat(),
                8.0f,
                false,
                false,
                DashLookAndFeel::kBgCardElevated,
                DashLookAndFeel::kRubyBorder,
                1.0f);
        }

        if (!apoCardBounds_.isEmpty())
        {
            DashLookAndFeel::drawHudPanel(
                g,
                apoCardBounds_.toFloat(),
                8.0f,
                false,
                false,
                DashLookAndFeel::kBgCardElevated,
                DashLookAndFeel::kRubyBorder,
                1.0f);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20, 16);
        headerTitle_.setBounds(area.removeFromTop(24));
        area.removeFromTop(12);

        // Tarjeta de Dispositivo
        deviceCardBounds_ = area.removeFromTop(96);
        auto dInner = deviceCardBounds_.reduced(16, 12);
        deviceCardTitle_.setBounds(dInner.removeFromTop(20));
        dInner.removeFromTop(8);
        outputDeviceBox_.setBounds(dInner.removeFromTop(34));

        area.removeFromTop(14);

        // Tarjeta de Equalizer APO
        apoCardBounds_ = area.removeFromTop(144);
        auto aInner = apoCardBounds_.reduced(16, 12);
        apoCardTitle_.setBounds(aInner.removeFromTop(20));
        aInner.removeFromTop(10);

        auto btnRow = aInner.removeFromTop(38);
        const int gap = 12;
        const int btnW = (btnRow.getWidth() - gap) / 2;
        autoConfigButton_.setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(gap);
        restoreApoButton_.setBounds(btnRow);

        aInner.removeFromTop(8);
        statusLabel_.setBounds(aInner.removeFromTop(22));
    }

private:
    juce::Label headerTitle_;

    juce::Rectangle<int> deviceCardBounds_;
    juce::Label          deviceCardTitle_;
    juce::ComboBox       outputDeviceBox_;

    juce::Rectangle<int> apoCardBounds_;
    juce::Label          apoCardTitle_;
    juce::TextButton     autoConfigButton_;
    juce::TextButton     restoreApoButton_;
    juce::Label          statusLabel_;
};

// ==============================================================================
// 3. Pantalla de Bloqueo / Onboarding Cyberpunk (ApoOnboardingOverlay)
// ==============================================================================
class MainComponent::ApoOnboardingOverlay final : public juce::Component
{
public:
    std::function<void()> onVerifyRequested;

    ApoOnboardingOverlay()
    {
        titleLabel_.setText(
            "// CRITICAL ALERT: EQUALIZER APO NO DETECTADO",
            juce::dontSendNotification);
        titleLabel_.setJustificationType(juce::Justification::centred);
        titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(17.0f).withStyle("Bold")));
        titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCrimsonGlow);
        addAndMakeVisible(titleLabel_);

        descLabel_.setText(
            "ESTE ECUALIZADOR TÁCTICO REQUIERE EL CONTROLADOR DE EQUALIZER APO PARA PROCESAR EL AUDIO NATIVO DEL SISTEMA CON CERO LATENCIA.",
            juce::dontSendNotification);
        descLabel_.setJustificationType(juce::Justification::centred);
        descLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.5f)));
        descLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
        addAndMakeVisible(descLabel_);

        stepsHeader_.setText(
            "// PROTOCOLO DE DESPLIEGUE EN 3 PASOS:",
            juce::dontSendNotification);
        stepsHeader_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(13.0f).withStyle("Bold")));
        stepsHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyanGlow);
        addAndMakeVisible(stepsHeader_);

        step1Label_.setText("1. DESCARGA E INSTALA EQUALIZER APO (PROYECTO OPEN SOURCE GRATUITO).", juce::dontSendNotification);
        step1Label_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.0f)));
        step1Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(step1Label_);

        step2Label_.setText("2. EN EL CONFIGURATOR DE APO, MARCA TUS AURICULARES O ALTAVOCES COMO DISPOSITIVO DE REPRODUCCIÓN.", juce::dontSendNotification);
        step2Label_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.0f)));
        step2Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(step2Label_);

        step3Label_.setText("3. REINICIA TU ORDENADOR PARA QUE WINDOWS ACTIVE EL CONTROLADOR DE APO.", juce::dontSendNotification);
        step3Label_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.0f)));
        step3Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
        addAndMakeVisible(step3Label_);

        downloadButton_.setButtonText("// DESCARGAR APO (WEB OFICIAL)");
        downloadButton_.onClick = [] {
            juce::URL("https://sourceforge.net/projects/equalizerapo/").launchInDefaultBrowser();
        };
        addAndMakeVisible(downloadButton_);

        verifyButton_.setButtonText("// VERIFICAR INSTALACIÓN DE APO");
        verifyButton_.onClick = [this] {
            if (onVerifyRequested)
                onVerifyRequested();
        };
        addAndMakeVisible(verifyButton_);

        statusLabel_.setText({}, juce::dontSendNotification);
        statusLabel_.setJustificationType(juce::Justification::centred);
        statusLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.0f).withStyle("Bold")));
        addAndMakeVisible(statusLabel_);
    }

    void setStatusMessage(const juce::String& message, bool isError)
    {
        statusLabel_.setText(message.toUpperCase(), juce::dontSendNotification);
        statusLabel_.setColour(
            juce::Label::textColourId,
            isError ? DashLookAndFeel::kNeonCrimsonGlow : DashLookAndFeel::kNeonEmerald);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xf2070204)); // Fondo oscuro obsidiana translúcido

        if (!cardBounds_.isEmpty())
        {
            DashLookAndFeel::drawHudPanel(
                g,
                cardBounds_.toFloat(),
                14.0f,
                true,
                true,
                DashLookAndFeel::kBgCard,
                DashLookAndFeel::kNeonCrimson,
                1.8f);
        }

        if (!tutorialBounds_.isEmpty())
        {
            DashLookAndFeel::drawHudPanel(
                g,
                tutorialBounds_.toFloat(),
                8.0f,
                false,
                false,
                DashLookAndFeel::kBgCardElevated,
                DashLookAndFeel::kRubyBorder,
                1.0f);
        }
    }

    void resized() override
    {
        const int cardW = juce::jlimit(440, 680, getWidth() - 48);
        const int cardH = juce::jlimit(360, 460, getHeight() - 40);

        cardBounds_ = getLocalBounds().withSizeKeepingCentre(cardW, cardH);
        auto inner = cardBounds_.reduced(24, 20);

        titleLabel_.setBounds(inner.removeFromTop(26));
        inner.removeFromTop(8);
        descLabel_.setBounds(inner.removeFromTop(36));
        inner.removeFromTop(12);

        tutorialBounds_ = inner.removeFromTop(134);
        auto tutInner = tutorialBounds_.reduced(16, 12);
        stepsHeader_.setBounds(tutInner.removeFromTop(20));
        tutInner.removeFromTop(6);
        step1Label_.setBounds(tutInner.removeFromTop(22));
        step2Label_.setBounds(tutInner.removeFromTop(34));
        step3Label_.setBounds(tutInner.removeFromTop(22));

        inner.removeFromTop(16);

        const int btnGap = 14;
        const int btnW = (inner.getWidth() - btnGap) / 2;
        auto btnArea = inner.removeFromTop(36);
        downloadButton_.setBounds(btnArea.removeFromLeft(btnW));
        btnArea.removeFromLeft(btnGap);
        verifyButton_.setBounds(btnArea);

        inner.removeFromTop(8);
        statusLabel_.setBounds(inner.removeFromTop(22));
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
// 4. MainComponent Principal
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

    loadPersistedSettings();

    audioGraph_ = std::make_unique<dash::core::AudioGraph>();
    audioGraph_->prepare(kDefaultSampleRate, kDefaultBufferSize, 2);
    audioGraph_->updateParameters(graphParams_);

    audioBackend_ = std::make_unique<dash::audio::JuceWasapiAudioBackend>();
    audioBackend_->initialize();

    // 2. Construir barra superior de telemetría (Top Bar)
    appTitleLabel_.setText("DASH // COMPETITIVE AUDIO [TACTICAL HUD]", juce::dontSendNotification);
    appTitleLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(13.5f).withStyle("Bold")));
    appTitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(appTitleLabel_);

    cpuChipLabel_.setText("CPU: 0.0 %", juce::dontSendNotification);
    cpuChipLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.5f).withStyle("Bold")));
    cpuChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonEmerald);
    addAndMakeVisible(cpuChipLabel_);

    latencyChipLabel_.setText("LAT: 0.0 MS", juce::dontSendNotification);
    latencyChipLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.5f).withStyle("Bold")));
    latencyChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyanGlow);
    addAndMakeVisible(latencyChipLabel_);

    apoChipLabel_.setText("APO: CHECKING", juce::dontSendNotification);
    apoChipLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.5f).withStyle("Bold")));
    apoChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonEmerald);
    addAndMakeVisible(apoChipLabel_);

    masterBypassToggle_.setButtonText("BYPASS GLOBAL");
    masterBypassToggle_.setToggleState(graphParams_.bypassAll, juce::dontSendNotification);
    masterBypassToggle_.onClick = [this] {
        const bool bypassed = masterBypassToggle_.getToggleState();
        graphParams_.bypassAll = bypassed;
        if (audioBackend_)
            audioBackend_->setBypass(bypassed);

        if (equalizerHudView_)
            equalizerHudView_->setBypassedVisualState(bypassed);

        syncDspParameters();
        scheduleDebouncedApoSync();
        savePersistedSettings();
    };
    addAndMakeVisible(masterBypassToggle_);

    // 3. Construir Sidebar Táctico Izquierdo
    navHeaderLabel_.setText("// NAVEGACIÓN HUD", juce::dontSendNotification);
    navHeaderLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.0f).withStyle("Bold")));
    navHeaderLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCrimsonGlow);
    addAndMakeVisible(navHeaderLabel_);

    navEqButton_.setButtonText("[ 01 // MATRIZ EQ ]");
    navEqButton_.onClick = [this] { setView(ActiveView::Equalizer); };
    addAndMakeVisible(navEqButton_);

    navSettingsButton_.setButtonText("[ 02 // AJUSTES ]");
    navSettingsButton_.onClick = [this] { setView(ActiveView::Settings); };
    addAndMakeVisible(navSettingsButton_);

    preampTitleLabel_.setText("// PREAMP MASTER", juce::dontSendNotification);
    preampTitleLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(12.0f).withStyle("Bold")));
    preampTitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(preampTitleLabel_);

    preampSubLabel_.setText("GANANCIA ENTRADA", juce::dontSendNotification);
    preampSubLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(10.5f)));
    preampSubLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyan);
    addAndMakeVisible(preampSubLabel_);

    styleHudFader(preampSlider_);
    preampSlider_.setValue(static_cast<double>(graphParams_.preampDb), juce::dontSendNotification);
    preampSlider_.onValueChange = [this] {
        graphParams_.preampDb = juce::jlimit(
            dash::core::kMinEqGainDb,
            dash::core::kMaxEqGainDb,
            static_cast<float>(preampSlider_.getValue()));
        syncDspParameters();
        scheduleDebouncedApoSync();
        scheduleDebouncedSettingsSave();
    };
    addAndMakeVisible(preampSlider_);

    resetButton_.setButtonText("[ RESET MATRIX ]");
    resetButton_.onClick = [this] { resetAllEqAndPreamp(); };
    addAndMakeVisible(resetButton_);

    deviceHeaderLabel_.setText("// AUDIO ENDPOINT", juce::dontSendNotification);
    deviceHeaderLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(11.0f).withStyle("Bold")));
    deviceHeaderLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCyan);
    addAndMakeVisible(deviceHeaderLabel_);

    deviceActiveLabel_.setText("PREDETERMINADO", juce::dontSendNotification);
    deviceActiveLabel_.setFont(juce::Font(juce::FontOptions().withName("Consolas").withHeight(10.5f)));
    deviceActiveLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(deviceActiveLabel_);

    // 4. Construir Vistas Centrales HUD
    equalizerHudView_ = std::make_unique<EqualizerHudView>();
    equalizerHudView_->setBandValues(graphParams_.bandGainsDb);
    equalizerHudView_->setBypassedVisualState(graphParams_.bypassAll);
    equalizerHudView_->onBandChanged = [this](size_t bandIdx, float gainDb) {
        if (bandIdx < graphParams_.bandGainsDb.size())
        {
            graphParams_.bandGainsDb[bandIdx] = juce::jlimit(
                dash::core::kMinEqGainDb,
                dash::core::kMaxEqGainDb,
                gainDb);
            syncDspParameters();
            scheduleDebouncedApoSync();
            scheduleDebouncedSettingsSave();
        }
    };
    addAndMakeVisible(equalizerHudView_.get());

    settingsHudView_ = std::make_unique<SettingsHudView>();
    settingsHudView_->onOutputDeviceSelected = [this](int index) {
        if (index >= 0 && static_cast<size_t>(index) < outputEndpoints_.size())
        {
            const auto& ep = outputEndpoints_[static_cast<size_t>(index)];
            selectedDeviceName_   = ep.friendlyName;
            selectedEndpointGuid_ = wideToNarrowAscii(ep.endpointGuid);

            deviceActiveLabel_.setText(juce::String(ep.friendlyName.c_str()).toUpperCase(), juce::dontSendNotification);

            if (!ep.id.empty() && ep.id.front() == L'{')
                policyManager_.setDefaultEndpoint(ep.id);

            restartAudioStream();
            scheduleDebouncedApoSync();
        }
    };

    settingsHudView_->onAutoConfigApoClicked = [this] {
        std::string err;
        if (executeApoSync(&err))
        {
            settingsHudView_->setStatus("¡APO AUTO-CONFIGURADO CON ÉXITO CON LAS 6 BANDAS!", false);
        }
        else
        {
            const juce::String msg = err.empty()
                ? "NO SE PUDO ESCRIBIR CONFIG.TXT. VERIFICA LA INSTALACIÓN DE APO."
                : juce::String(err.c_str());
            settingsHudView_->setStatus(msg, true);
        }
    };

    settingsHudView_->onRestoreApoClicked = [this] {
        needsApoSync_    = false;
        debounceCounter_ = 0;

        if (!dash::audio::ApoConfigManager::isApoInstalled())
        {
            const juce::String notInstalledMsg = "EQUALIZER APO NO ESTÁ INSTALADO EN EL SISTEMA.";
            settingsHudView_->setStatus(notInstalledMsg, true);
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "EQUALIZER APO NO DETECTADO",
                notInstalledMsg,
                "OK",
                this);
            return;
        }

        std::string err;
        const auto res = dash::audio::ApoConfigManager::restoreOriginalConfig(&err);
        if (res == dash::audio::ApoConfigResult::Success)
        {
            settingsHudView_->setStatus("¡APO ORIGINAL RESTAURADO CON ÉXITO DESDE CONFIG.TXT.BAK!", false);
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "APO ORIGINAL RESTAURADO",
                "Se restauró exitosamente tu archivo config.txt original a partir de config.txt.bak.\nEqualizer APO ha recargado automáticamente la configuración.",
                "OK",
                this);
        }
        else
        {
            const juce::String failMsg = err.empty()
                ? "NO SE ENCONTRÓ CONFIG.TXT.BAK O NO SE PUDO RESTAURAR CONFIG.TXT."
                : juce::String(err.c_str());
            settingsHudView_->setStatus(failMsg, true);
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "NO SE PUDO RESTAURAR APO",
                failMsg,
                "OK",
                this);
        }
    };
    addChildComponent(settingsHudView_.get());

    // 5. Overlay Onboarding si no se detecta APO
    apoOverlay_ = std::make_unique<ApoOnboardingOverlay>();
    apoOverlay_->onVerifyRequested = [this] {
        checkApoInstallation(true);
    };
    addChildComponent(apoOverlay_.get());

    // 6. Detección de dispositivos e inicialización de audio
    populateOutputDevices();
    restartAudioStream();

    // 7. Verificar presencia de APO y bloquear si es necesario
    checkApoInstallation(false);

    setView(ActiveView::Equalizer);

    setSize(960, 580);
    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    savePersistedSettings();

    if (audioBackend_)
        audioBackend_->shutdown();

    if (dash::audio::ApoConfigManager::isApoInstalled())
    {
        const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
        if (dash::audio::ApoConfigManager::hasManagedSection(apoConfigPath))
            dash::audio::ApoConfigManager::removeManagedSection(apoConfigPath);
    }

    setLookAndFeel(nullptr);
}

void MainComponent::setView(ActiveView view)
{
    currentView_ = view;
    updateNavigationButtons();

    if (equalizerHudView_)
        equalizerHudView_->setVisible(currentView_ == ActiveView::Equalizer);

    if (settingsHudView_)
        settingsHudView_->setVisible(currentView_ == ActiveView::Settings);

    resized();
}

void MainComponent::updateNavigationButtons()
{
    const bool isEq = (currentView_ == ActiveView::Equalizer);
    navEqButton_.setColour(
        juce::TextButton::buttonColourId,
        isEq ? DashLookAndFeel::kNeonCrimson.withAlpha(0.28f) : DashLookAndFeel::kBgCardElevated);
    navEqButton_.setColour(
        juce::TextButton::textColourOffId,
        isEq ? DashLookAndFeel::kNeonCyanGlow : DashLookAndFeel::kTextMuted);

    navSettingsButton_.setColour(
        juce::TextButton::buttonColourId,
        !isEq ? DashLookAndFeel::kNeonCrimson.withAlpha(0.28f) : DashLookAndFeel::kBgCardElevated);
    navSettingsButton_.setColour(
        juce::TextButton::textColourOffId,
        !isEq ? DashLookAndFeel::kNeonCyanGlow : DashLookAndFeel::kTextMuted);
}

void MainComponent::checkApoInstallation(bool userInitiated)
{
    const bool apoInstalled = dash::audio::ApoConfigManager::isApoInstalled();

    if (apoInstalled)
    {
        if (apoOverlay_)
            apoOverlay_->setVisible(false);

        apoChipLabel_.setText("APO: HOOKED", juce::dontSendNotification);
        apoChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonEmerald);

        executeApoSync();
    }
    else
    {
        apoChipLabel_.setText("APO: OFFLINE", juce::dontSendNotification);
        apoChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCrimsonGlow);

        if (apoOverlay_)
        {
            apoOverlay_->setVisible(true);
            apoOverlay_->toFront(true);
            if (userInitiated)
            {
                apoOverlay_->setStatusMessage("APO AÚN NO FUE DETECTADO. ASEGÚRATE DE INSTALARLO Y REINICIAR.", true);
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
    g.fillAll(DashLookAndFeel::kBgObsidian);

    // Fondo y borde divisor de la barra superior
    if (!topBarBounds_.isEmpty())
    {
        g.setColour(DashLookAndFeel::kBgDark);
        g.fillRect(topBarBounds_);

        g.setColour(DashLookAndFeel::kNeonCrimson.withAlpha(0.85f));
        g.drawHorizontalLine(topBarBounds_.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));

        g.setColour(DashLookAndFeel::kNeonCrimsonGlow.withAlpha(0.35f));
        g.drawHorizontalLine(topBarBounds_.getBottom() - 2, 0.0f, static_cast<float>(getWidth()));
    }

    // Fondo del Sidebar Táctico Izquierdo
    if (!sidebarBounds_.isEmpty())
    {
        DashLookAndFeel::drawHudPanel(
            g,
            sidebarBounds_.toFloat(),
            10.0f,
            true,
            true,
            DashLookAndFeel::kBgDark,
            DashLookAndFeel::kRubyBorder,
            1.2f);
    }

    // Sub-panel Preamp dentro del sidebar
    if (!preampCardBounds_.isEmpty())
    {
        DashLookAndFeel::drawHudPanel(
            g,
            preampCardBounds_.toFloat(),
            8.0f,
            false,
            false,
            DashLookAndFeel::kBgCardElevated,
            DashLookAndFeel::kRubyBorder,
            1.0f);
    }

    // Sub-panel Endpoint dentro del sidebar
    if (!deviceCardBounds_.isEmpty())
    {
        DashLookAndFeel::drawHudPanel(
            g,
            deviceCardBounds_.toFloat(),
            6.0f,
            false,
            false,
            DashLookAndFeel::kBgCardElevated,
            DashLookAndFeel::kRubyBorder,
            1.0f);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // 1. Barra Superior (Top Bar de Telemetría)
    topBarBounds_ = area.removeFromTop(44);
    auto tbInner = topBarBounds_.reduced(14, 6);

    appTitleLabel_.setBounds(tbInner.removeFromLeft(340));

    masterBypassToggle_.setBounds(tbInner.removeFromRight(150));
    tbInner.removeFromRight(12);

    apoChipLabel_.setBounds(tbInner.removeFromRight(110));
    tbInner.removeFromRight(8);

    latencyChipLabel_.setBounds(tbInner.removeFromRight(100));
    tbInner.removeFromRight(8);

    cpuChipLabel_.setBounds(tbInner.removeFromRight(95));

    // 2. Sidebar Lateral Táctico (Izquierda)
    const int sidebarW = juce::jlimit(210, 240, area.getWidth() / 4);
    sidebarBounds_ = area.removeFromLeft(sidebarW).reduced(6, 6);
    auto sb = sidebarBounds_.reduced(10, 10);

    navHeaderLabel_.setBounds(sb.removeFromTop(18));
    sb.removeFromTop(4);
    navEqButton_.setBounds(sb.removeFromTop(32));
    sb.removeFromTop(6);
    navSettingsButton_.setBounds(sb.removeFromTop(32));
    sb.removeFromTop(12);

    deviceCardBounds_ = sb.removeFromBottom(58);
    auto devInner = deviceCardBounds_.reduced(8, 6);
    deviceHeaderLabel_.setBounds(devInner.removeFromTop(18));
    deviceActiveLabel_.setBounds(devInner.removeFromTop(20));

    sb.removeFromBottom(10);

    preampCardBounds_ = sb;
    auto preInner = preampCardBounds_.reduced(8, 8);
    preampTitleLabel_.setBounds(preInner.removeFromTop(18));
    preampSubLabel_.setBounds(preInner.removeFromTop(16));
    preInner.removeFromTop(4);

    auto resetArea = preInner.removeFromBottom(30);
    preInner.removeFromBottom(6);
    preampSlider_.setBounds(preInner);
    resetButton_.setBounds(resetArea.reduced(2, 0));

    // 3. Área Central HUD
    centralHudBounds_ = area.reduced(6, 6);

    if (equalizerHudView_)
        equalizerHudView_->setBounds(centralHudBounds_);

    if (settingsHudView_)
        settingsHudView_->setBounds(centralHudBounds_);

    // 4. Overlay Onboarding si está activo
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

    if (needsSettingsSave_)
    {
        if (++settingsSaveDebounceCounter_ >= 4)
        {
            savePersistedSettings();
        }
    }

    if (audioBackend_)
    {
        const auto state = audioBackend_->getState();
        const bool active = (state == dash::audio::AudioBackendState::Running ||
                             state == dash::audio::AudioBackendState::Bypassed);

        const float cpuPercent = active ? audioBackend_->getCpuUsage() : 0.0f;
        float latencyMs        = active ? audioBackend_->getCurrentLatencyMs() : 0.0f;
        if (active && latencyMs <= 0.0f)
            latencyMs = static_cast<float>((kDefaultBufferSize * 1000.0) / kDefaultSampleRate);

        updateTelemetryVisuals(cpuPercent, latencyMs, dash::audio::ApoConfigManager::isApoInstalled());
    }
}

void MainComponent::updateTelemetryVisuals(float cpuPercent, float latencyMs, bool apoActive)
{
    cpuChipLabel_.setText("CPU: " + juce::String(cpuPercent, 1) + " %", juce::dontSendNotification);
    latencyChipLabel_.setText("LAT: " + juce::String(latencyMs, 1) + " MS", juce::dontSendNotification);

    if (apoActive)
    {
        apoChipLabel_.setText("APO: HOOKED", juce::dontSendNotification);
        apoChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonEmerald);
    }
    else
    {
        apoChipLabel_.setText("APO: OFFLINE", juce::dontSendNotification);
        apoChipLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kNeonCrimsonGlow);
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
        deviceActiveLabel_.setText(juce::String(chosen.friendlyName.c_str()).toUpperCase(), juce::dontSendNotification);
    }

    if (settingsHudView_)
        settingsHudView_->setDeviceList(names, outputEndpoints_.empty() ? -1 : defaultIdx);
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

    preampSlider_.setValue(0.0, juce::dontSendNotification);

    if (equalizerHudView_)
        equalizerHudView_->setBandValues(graphParams_.bandGainsDb);

    syncDspParameters();
    scheduleDebouncedApoSync();
    savePersistedSettings();
}

void MainComponent::loadPersistedSettings()
{
    dash::audio::AppPersistedSettings loaded;
    if (dash::audio::AppSettingsStore::load(loaded))
    {
        graphParams_.preampDb    = loaded.preampDb;
        graphParams_.bandGainsDb = loaded.bandGainsDb;
        graphParams_.bypassAll   = loaded.bypass;
    }
}

void MainComponent::savePersistedSettings()
{
    needsSettingsSave_           = false;
    settingsSaveDebounceCounter_ = 0;

    dash::audio::AppPersistedSettings snapshot;
    snapshot.preampDb    = graphParams_.preampDb;
    snapshot.bandGainsDb = graphParams_.bandGainsDb;
    snapshot.bypass      = graphParams_.bypassAll;

    dash::audio::AppSettingsStore::save(snapshot);
}

void MainComponent::scheduleDebouncedSettingsSave()
{
    needsSettingsSave_           = true;
    settingsSaveDebounceCounter_ = 0;
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
