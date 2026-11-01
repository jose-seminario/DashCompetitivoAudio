#include "AudioEnginePanel.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

AudioEnginePanel::AudioEnginePanel()
{
    titleLabel_.setText(juce::String::fromUTF8(u8"MOTOR DE AUDIO Y DISPOSITIVOS DE REPRODUCCIÓN (WASAPI / APO)"), juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    deviceSelectLabel_.setText(juce::String::fromUTF8(u8"Dispositivo de Reproducción (Endpoint):"), juce::dontSendNotification);
    deviceSelectLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    addAndMakeVisible(deviceSelectLabel_);

    addAndMakeVisible(deviceSelector_);

    refreshDevicesBtn_.setButtonText(juce::String::fromUTF8(u8"ACTUALIZAR DISPOSITIVOS"));
    refreshDevicesBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    refreshDevicesBtn_.onClick = [this] { refreshDevices(); };
    addAndMakeVisible(refreshDevicesBtn_);

    setAsDefaultBtn_.setButtonText(juce::String::fromUTF8(u8"ESTABLECER COMO PREDETERMINADO EN WINDOWS"));
    setAsDefaultBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
    setAsDefaultBtn_.onClick = [this] {
        const int idx = deviceSelector_.getSelectedId() - 1;
        if (idx >= 0 && idx < static_cast<int>(endpoints_.size()))
        {
            policyManager_.setDefaultEndpoint(endpoints_[static_cast<size_t>(idx)].id);
            if (onEndpointChanged)
                onEndpointChanged(endpoints_[static_cast<size_t>(idx)].id);
            refreshDevices();
        }
    };
    addAndMakeVisible(setAsDefaultBtn_);

    // Sample Rate
    sampleRateLabel_.setText(juce::String::fromUTF8(u8"Frecuencia de Muestreo:"), juce::dontSendNotification);
    sampleRateLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    addAndMakeVisible(sampleRateLabel_);

    sampleRateSelector_.addItem(juce::String::fromUTF8(u8"44100 Hz (Estándar CD)"), 1);
    sampleRateSelector_.addItem(juce::String::fromUTF8(u8"48000 Hz (Recomendado Juegos)"), 2);
    sampleRateSelector_.addItem(juce::String::fromUTF8(u8"96000 Hz (Alta Fidelidad)"), 3);
    sampleRateSelector_.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(sampleRateSelector_);

    // Buffer Size
    bufferSizeLabel_.setText(juce::String::fromUTF8(u8"Tamaño de Búfer (Latencia):"), juce::dontSendNotification);
    bufferSizeLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    addAndMakeVisible(bufferSizeLabel_);

    bufferSizeSelector_.addItem(juce::String::fromUTF8(u8"128 muestras (2.6 ms - Baja Latencia Extrema)"), 1);
    bufferSizeSelector_.addItem(juce::String::fromUTF8(u8"256 muestras (5.3 ms - Óptimo Competitivo)"), 2);
    bufferSizeSelector_.addItem(juce::String::fromUTF8(u8"512 muestras (10.6 ms - Balanceado)"), 3);
    bufferSizeSelector_.addItem(juce::String::fromUTF8(u8"1024 muestras (21.3 ms - Modo Seguro)"), 4);
    bufferSizeSelector_.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(bufferSizeSelector_);

    // Channel Layout
    layoutLabel_.setText(juce::String::fromUTF8(u8"Layout de Entrada del Motor:"), juce::dontSendNotification);
    layoutLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    addAndMakeVisible(layoutLabel_);

    layoutSelector_.addItem(juce::String::fromUTF8(u8"2.0 Estéreo (2 canales)"), 1);
    layoutSelector_.addItem(juce::String::fromUTF8(u8"5.1 Surround (6 canales)"), 2);
    layoutSelector_.addItem(juce::String::fromUTF8(u8"7.1 Surround (8 canales - Warzone)"), 3);
    layoutSelector_.addItem(juce::String::fromUTF8(u8"7.1.4 Inmersivo Atmos (12 canales)"), 4);
    layoutSelector_.addItem(juce::String::fromUTF8(u8"9.1.6 Espacial Completo (16 canales)"), 5);
    layoutSelector_.setSelectedId(3, juce::dontSendNotification);
    addAndMakeVisible(layoutSelector_);

    auto notifyFormatConfig = [this] {
        if (!onFormatConfigChanged)
            return;

        double sr = 48000.0;
        switch (sampleRateSelector_.getSelectedId())
        {
            case 1: sr = 44100.0; break;
            case 2: sr = 48000.0; break;
            case 3: sr = 96000.0; break;
            default: break;
        }

        int bs = 256;
        switch (bufferSizeSelector_.getSelectedId())
        {
            case 1: bs = 128; break;
            case 2: bs = 256; break;
            case 3: bs = 512; break;
            case 4: bs = 1024; break;
            default: break;
        }

        int ch = 8;
        switch (layoutSelector_.getSelectedId())
        {
            case 1: ch = 2; break;
            case 2: ch = 6; break;
            case 3: ch = 8; break;
            case 4: ch = 12; break;
            case 5: ch = 16; break;
            default: break;
        }

        onFormatConfigChanged(sr, bs, ch);
    };

    sampleRateSelector_.onChange = notifyFormatConfig;
    bufferSizeSelector_.onChange = notifyFormatConfig;
    layoutSelector_.onChange = notifyFormatConfig;

    infoTextLabel_.setText(juce::String::fromUTF8(u8"Nota: El motor DashCompetitivoAudio procesa hasta 16 canales nativos en tiempo real con SIMD AVX2. Para Call of Duty: Warzone, se recomienda configurar Windows en 7.1 Surround con 48 kHz y 256 muestras de búfer."), juce::dontSendNotification);
    infoTextLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
    infoTextLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(infoTextLabel_);

    refreshDevices();
}

void AudioEnginePanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const auto cardArea = juce::Rectangle<float>(16, 44, (float)getWidth() - 32, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(cardArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(cardArea, 8.0f, 1.2f);
}

void AudioEnginePanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);

    const int startX = 36;
    const int contentW = getWidth() - 72;

    deviceSelectLabel_.setBounds(startX, 60, contentW, 20);
    deviceSelector_.setBounds(startX, 84, contentW - 200, 32);
    refreshDevicesBtn_.setBounds(startX + contentW - 190, 84, 190, 32);

    setAsDefaultBtn_.setBounds(startX, 124, 380, 34);

    const int colW = (contentW - 32) / 3;
    const int row2Y = 180;

    sampleRateLabel_.setBounds(startX, row2Y, colW, 20);
    sampleRateSelector_.setBounds(startX, row2Y + 24, colW, 30);

    bufferSizeLabel_.setBounds(startX + colW + 16, row2Y, colW, 20);
    bufferSizeSelector_.setBounds(startX + colW + 16, row2Y + 24, colW, 30);

    layoutLabel_.setBounds(startX + (colW + 16) * 2, row2Y, colW, 20);
    layoutSelector_.setBounds(startX + (colW + 16) * 2, row2Y + 24, colW, 30);

    infoTextLabel_.setBounds(startX, 260, contentW, 40);
}

void AudioEnginePanel::refreshDevices()
{
    deviceSelector_.clear();
    endpoints_ = policyManager_.enumerateActiveRenderEndpoints();

    int defaultIndex = 1;
    for (size_t i = 0; i < endpoints_.size(); ++i)
    {
        juce::String name(endpoints_[i].friendlyName.c_str());
        if (endpoints_[i].isDefaultConsole)
        {
            name += juce::String::fromUTF8(u8" [PREDETERMINADO CONSOLA/JUEGOS]");
            defaultIndex = static_cast<int>(i) + 1;
        }
        deviceSelector_.addItem(name, static_cast<int>(i) + 1);
    }

    if (!endpoints_.empty())
        deviceSelector_.setSelectedId(defaultIndex, juce::dontSendNotification);
}

} // namespace dash::ui
