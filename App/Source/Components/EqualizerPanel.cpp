#include "EqualizerPanel.h"
#include "../Theme/DashLookAndFeel.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace dash::ui
{

namespace
{
    float computeBiquadMagnitudeDb(
        float freqHz,
        dash::core::ParametricFilterType type,
        float bandFreqHz,
        float gainDb,
        float q,
        double sampleRate) noexcept
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        const float nyquist = static_cast<float>(sampleRate * 0.49);
        const float f0 = std::clamp(bandFreqHz, 20.0f, nyquist);
        const float safeQ = std::max(0.1f, q);

        const float w0 = 2.0f * kPi * f0 / static_cast<float>(sampleRate);
        const float cosW = std::cos(w0);
        const float sinW = std::sin(w0);
        const float alpha = sinW / (2.0f * safeQ);
        const float A = std::pow(10.0f, gainDb / 40.0f); // sqrt(gain lineal)

        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        switch (type)
        {
            case dash::core::ParametricFilterType::Peaking:
            {
                b0 = 1.0f + alpha * A;
                b1 = -2.0f * cosW;
                b2 = 1.0f - alpha * A;
                a0 = 1.0f + alpha / A;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha / A;
                break;
            }
            case dash::core::ParametricFilterType::LowShelf:
            {
                const float sqrtA = std::sqrt(A);
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cosW + 2.0f * sqrtA * alpha);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha);
                a0 = (A + 1.0f) + (A - 1.0f) * cosW + 2.0f * sqrtA * alpha;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosW);
                a2 = (A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha;
                break;
            }
            case dash::core::ParametricFilterType::HighShelf:
            {
                const float sqrtA = std::sqrt(A);
                b0 = A * ((A + 1.0f) + (A - 1.0f) * cosW + 2.0f * sqrtA * alpha);
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha);
                a0 = (A + 1.0f) - (A - 1.0f) * cosW + 2.0f * sqrtA * alpha;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosW);
                a2 = (A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha;
                break;
            }
            case dash::core::ParametricFilterType::HighPass:
            {
                b0 = (1.0f + cosW) * 0.5f;
                b1 = -(1.0f + cosW);
                b2 = (1.0f + cosW) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha;
                break;
            }
            case dash::core::ParametricFilterType::LowPass:
            {
                b0 = (1.0f - cosW) * 0.5f;
                b1 = 1.0f - cosW;
                b2 = (1.0f - cosW) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha;
                break;
            }
            case dash::core::ParametricFilterType::Notch:
            {
                b0 = 1.0f;
                b1 = -2.0f * cosW;
                b2 = 1.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha;
                break;
            }
        }

        const float omega = 2.0f * kPi * freqHz / static_cast<float>(sampleRate);
        const float cos1 = std::cos(omega);
        const float sin1 = std::sin(omega);
        const float cos2 = std::cos(2.0f * omega);
        const float sin2 = std::sin(2.0f * omega);

        const float numReal = b0 + b1 * cos1 + b2 * cos2;
        const float numImag = -(b1 * sin1 + b2 * sin2);

        const float denReal = a0 + a1 * cos1 + a2 * cos2;
        const float denImag = -(a1 * sin1 + a2 * sin2);

        const float numMagSq = numReal * numReal + numImag * numImag;
        const float denMagSq = denReal * denReal + denImag * denImag;

        if (denMagSq < 1.0e-12f)
            return 0.0f;

        const float magSq = std::max(1.0e-10f, numMagSq / denMagSq);
        return 10.0f * std::log10(magSq);
    }
} // namespace

EqualizerPanel::EqualizerPanel()
{
    titleLabel_.setText(juce::String::fromUTF8(u8"ECUALIZADOR ACÚSTICO (31 BANDAS ISO + PARAMÉTRICO DE 10 BANDAS)"), juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    bypassToggle_.setButtonText("Bypass EQ");
    bypassToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    bypassToggle_.onClick = [this] { notifyChange(); };
    addAndMakeVisible(bypassToggle_);

    parametricModeToggle_.setButtonText(juce::String::fromUTF8(u8"Activar Modo Paramétrico (PEQ)"));
    parametricModeToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kAccentCyan);
    parametricModeToggle_.onClick = [this] {
        useParametric_ = parametricModeToggle_.getToggleState();
        notifyChange();
        repaint();
    };
    addAndMakeVisible(parametricModeToggle_);

    // 1. Controles rápidos de pasos y graves (Rango de pasos ampliado a [-6, +12] dB)
    footstepsSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    footstepsSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    footstepsSlider_.setRange(-6.0, 12.0, 0.1);
    footstepsSlider_.setValue(1.6, juce::dontSendNotification);
    footstepsSlider_.setTextValueSuffix(" dB");
    footstepsSlider_.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    footstepsSlider_.onValueChange = [this] { notifyChange(); };
    addAndMakeVisible(footstepsSlider_);

    footstepsLabel_.setText(juce::String::fromUTF8(u8"Realce Pasos"), juce::dontSendNotification);
    footstepsLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    footstepsLabel_.setJustificationType(juce::Justification::centred);
    footstepsLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(footstepsLabel_);

    bassSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    bassSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    bassSlider_.setRange(-6.0, 6.0, 0.1);
    bassSlider_.setValue(1.5, juce::dontSendNotification);
    bassSlider_.setTextValueSuffix(" dB");
    bassSlider_.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    bassSlider_.onValueChange = [this] { notifyChange(); };
    addAndMakeVisible(bassSlider_);

    bassLabel_.setText(juce::String::fromUTF8(u8"Control Graves"), juce::dontSendNotification);
    bassLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    bassLabel_.setJustificationType(juce::Justification::centred);
    bassLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(bassLabel_);

    preampSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    preampSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    preampSlider_.setRange(-12.0, 12.0, 0.1);
    preampSlider_.setValue(0.0, juce::dontSendNotification);
    preampSlider_.setTextValueSuffix(" dB");
    preampSlider_.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    preampSlider_.onValueChange = [this] { notifyChange(); };
    addAndMakeVisible(preampSlider_);

    preampLabel_.setText(juce::String::fromUTF8(u8"Preamplificador"), juce::dontSendNotification);
    preampLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    preampLabel_.setJustificationType(juce::Justification::centred);
    preampLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(preampLabel_);

    // 2. Editor de bandas paramétricas
    bandSelectLabel_.setText(juce::String::fromUTF8(u8"Banda:"), juce::dontSendNotification);
    bandSelectLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
    addAndMakeVisible(bandSelectLabel_);

    for (int i = 0; i < 10; ++i)
        bandSelector_.addItem(juce::String::fromUTF8(u8"Banda ") + juce::String(i + 1), i + 1);
    bandSelector_.setSelectedId(1, juce::dontSendNotification);
    bandSelector_.onChange = [this] {
        selectedBandIndex_ = bandSelector_.getSelectedId() - 1;
        updateCurrentBandControls();
    };
    addAndMakeVisible(bandSelector_);

    bandEnableToggle_.setButtonText(juce::String::fromUTF8(u8"Habilitada"));
    bandEnableToggle_.setToggleState(true, juce::dontSendNotification);
    bandEnableToggle_.onClick = [this] {
        if (selectedBandIndex_ >= 0 && selectedBandIndex_ < 10)
        {
            bands_[static_cast<size_t>(selectedBandIndex_)].enabled = bandEnableToggle_.getToggleState();
            notifyChange();
        }
    };
    addAndMakeVisible(bandEnableToggle_);

    filterTypeLabel_.setText(juce::String::fromUTF8(u8"Tipo:"), juce::dontSendNotification);
    filterTypeLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
    addAndMakeVisible(filterTypeLabel_);

    filterTypeSelector_.addItem(juce::String::fromUTF8(u8"Campana (Peaking)"), 1);
    filterTypeSelector_.addItem(juce::String::fromUTF8(u8"Estante Graves (Low Shelf)"), 2);
    filterTypeSelector_.addItem(juce::String::fromUTF8(u8"Estante Agudos (High Shelf)"), 3);
    filterTypeSelector_.addItem(juce::String::fromUTF8(u8"Pasa-Altos (High Pass)"), 4);
    filterTypeSelector_.addItem(juce::String::fromUTF8(u8"Pasa-Bajos (Low Pass)"), 5);
    filterTypeSelector_.addItem(juce::String::fromUTF8(u8"Trampa (Notch)"), 6);
    filterTypeSelector_.setSelectedId(1, juce::dontSendNotification);
    filterTypeSelector_.onChange = [this] {
        if (selectedBandIndex_ >= 0 && selectedBandIndex_ < 10)
        {
            const int id = filterTypeSelector_.getSelectedId();
            auto& b = bands_[static_cast<size_t>(selectedBandIndex_)];
            switch (id)
            {
                case 1: b.type = dash::core::ParametricFilterType::Peaking; break;
                case 2: b.type = dash::core::ParametricFilterType::LowShelf; break;
                case 3: b.type = dash::core::ParametricFilterType::HighShelf; break;
                case 4: b.type = dash::core::ParametricFilterType::HighPass; break;
                case 5: b.type = dash::core::ParametricFilterType::LowPass; break;
                case 6: b.type = dash::core::ParametricFilterType::Notch; break;
            }
            notifyChange();
        }
    };
    addAndMakeVisible(filterTypeSelector_);

    // Frecuencia
    freqSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    freqSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    freqSlider_.setRange(20.0, 20000.0, 1.0);
    freqSlider_.setSkewFactorFromMidPoint(1000.0);
    freqSlider_.setValue(1000.0, juce::dontSendNotification);
    freqSlider_.setTextValueSuffix(" Hz");
    freqSlider_.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    freqSlider_.onValueChange = [this] {
        if (selectedBandIndex_ >= 0 && selectedBandIndex_ < 10)
        {
            bands_[static_cast<size_t>(selectedBandIndex_)].frequencyHz = static_cast<float>(freqSlider_.getValue());
            notifyChange();
        }
    };
    addAndMakeVisible(freqSlider_);

    freqLabel_.setText(juce::String::fromUTF8(u8"Frecuencia"), juce::dontSendNotification);
    freqLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    freqLabel_.setJustificationType(juce::Justification::centred);
    freqLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(freqLabel_);

    // Ganancia
    gainSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    gainSlider_.setRange(-18.0, 18.0, 0.1);
    gainSlider_.setValue(0.0, juce::dontSendNotification);
    gainSlider_.setTextValueSuffix(" dB");
    gainSlider_.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    gainSlider_.onValueChange = [this] {
        if (selectedBandIndex_ >= 0 && selectedBandIndex_ < 10)
        {
            bands_[static_cast<size_t>(selectedBandIndex_)].gainDb = static_cast<float>(gainSlider_.getValue());
            notifyChange();
        }
    };
    addAndMakeVisible(gainSlider_);

    gainLabel_.setText(juce::String::fromUTF8(u8"Ganancia"), juce::dontSendNotification);
    gainLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    gainLabel_.setJustificationType(juce::Justification::centred);
    gainLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(gainLabel_);

    // Factor Q
    qSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    qSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    qSlider_.setRange(0.1, 10.0, 0.05);
    qSlider_.setValue(0.707, juce::dontSendNotification);
    qSlider_.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    qSlider_.onValueChange = [this] {
        if (selectedBandIndex_ >= 0 && selectedBandIndex_ < 10)
        {
            bands_[static_cast<size_t>(selectedBandIndex_)].q = static_cast<float>(qSlider_.getValue());
            notifyChange();
        }
    };
    addAndMakeVisible(qSlider_);

    qLabel_.setText(juce::String::fromUTF8(u8"Factor Q"), juce::dontSendNotification);
    qLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    qLabel_.setJustificationType(juce::Justification::centred);
    qLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(qLabel_);

    updateCurrentBandControls();
}

void EqualizerPanel::updateCurrentBandControls()
{
    if (selectedBandIndex_ >= 0 && selectedBandIndex_ < 10)
    {
        const auto& b = bands_[static_cast<size_t>(selectedBandIndex_)];
        bandEnableToggle_.setToggleState(b.enabled, juce::dontSendNotification);
        freqSlider_.setValue(b.frequencyHz, juce::dontSendNotification);
        gainSlider_.setValue(b.gainDb, juce::dontSendNotification);
        qSlider_.setValue(b.q, juce::dontSendNotification);

        int typeId = 1;
        switch (b.type)
        {
            case dash::core::ParametricFilterType::Peaking:   typeId = 1; break;
            case dash::core::ParametricFilterType::LowShelf:  typeId = 2; break;
            case dash::core::ParametricFilterType::HighShelf: typeId = 3; break;
            case dash::core::ParametricFilterType::HighPass:  typeId = 4; break;
            case dash::core::ParametricFilterType::LowPass:   typeId = 5; break;
            case dash::core::ParametricFilterType::Notch:     typeId = 6; break;
        }
        filterTypeSelector_.setSelectedId(typeId, juce::dontSendNotification);
    }
}

void EqualizerPanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const float cardW = (float)getWidth() - 32.0f;

    // 1. Tarjeta Gráfica Superior (Curva de Frecuencia)
    const auto curveArea = juce::Rectangle<float>(16, 44, cardW, 190);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(curveArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(curveArea, 8.0f, 1.2f);

    drawFrequencyResponseCurve(g, curveArea.reduced(24.0f, 16.0f));

    // 2. Tarjeta Inferior de Controles
    const auto ctrlArea = juce::Rectangle<float>(16, 244, cardW, (float)getHeight() - 258);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(ctrlArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(ctrlArea, 8.0f, 1.2f);
}

void EqualizerPanel::drawFrequencyResponseCurve(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    g.setColour(DashLookAndFeel::kBgCardElevated);
    g.fillRect(area);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRect(area, 1.0f);

    // Eje de frecuencias logarítmico (20, 100, 1k, 10k, 20k Hz)
    static const std::array<float, 5> gridFreqs = { 20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f };
    const float minLog = std::log10(20.0f);
    const float maxLog = std::log10(20000.0f);

    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(9.5f)));
    for (float f : gridFreqs)
    {
        const float normX = (std::log10(f) - minLog) / (maxLog - minLog);
        const float x = area.getX() + normX * area.getWidth();
        g.setColour(DashLookAndFeel::kBorderSubtle.withAlpha(0.5f));
        g.drawVerticalLine((int)x, area.getY(), area.getBottom());

        g.setColour(DashLookAndFeel::kTextMuted);
        juce::String label = (f >= 1000.0f) ? (juce::String((int)(f / 1000.0f)) + "k") : juce::String((int)f);
        g.drawText(label, (int)x - 15, (int)area.getBottom() - 14, 30, 12, juce::Justification::centred);
    }

    // Línea 0 dB central
    const float midY = area.getCentreY();
    g.setColour(DashLookAndFeel::kTextMuted.withAlpha(0.6f));
    g.drawHorizontalLine((int)midY, area.getX(), area.getRight());

    // Curva combinada calculada
    const auto gains31 = dash::core::EQProcessor::computeBandGainsDb(footstepsSlider_.getValue(), bassSlider_.getValue());

    juce::Path curve;
    bool started = false;

    const int numPoints = 120;
    for (int i = 0; i < numPoints; ++i)
    {
        const float normX = (float)i / (float)(numPoints - 1);
        const float freq = std::pow(10.0f, minLog + normX * (maxLog - minLog));

        // Calcular ganancia aproximada interpolada de las 31 bandas ISO
        float totalDb = 0.0f;
        for (size_t b = 0; b < dash::core::EQProcessor::kNumBands; ++b)
        {
            const float bandFreq = dash::core::EQProcessor::kIsoFrequencies[b];
            const float octDiff = std::abs(std::log2(freq / bandFreq));
            if (octDiff < 1.0f)
            {
                const float w = 1.0f - octDiff;
                totalDb += (float)gains31[b] * w * 0.4f;
            }
        }

        // Sumar bandas paramétricas calculando su respuesta en frecuencia compleja real (Cookbook RBJ)
        if (useParametric_)
        {
            for (const auto& band : bands_)
            {
                if (band.enabled)
                {
                    totalDb += computeBiquadMagnitudeDb(freq, band.type, band.frequencyHz, band.gainDb, band.q, 48000.0);
                }
            }
        }

        const float normY = (totalDb + 18.0f) / 36.0f;
        const float px = area.getX() + normX * area.getWidth();
        const float py = area.getBottom() - juce::jlimit(0.0f, 1.0f, normY) * area.getHeight();

        if (!started)
        {
            curve.startNewSubPath(px, py);
            started = true;
        }
        else
        {
            curve.lineTo(px, py);
        }
    }

    g.setColour(bypassToggle_.getToggleState() ? DashLookAndFeel::kTextMuted : DashLookAndFeel::kAccentCyan);
    g.strokePath(curve, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void EqualizerPanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);
    bypassToggle_.setBounds(getWidth() - 280, 12, 100, 24);
    parametricModeToggle_.setBounds(getWidth() - 170, 12, 160, 24);

    // Fila rápida (Izquierda)
    const int startX = 36;
    const int dialY = 265;
    const int dialW = 100;
    const int dialH = 85;

    footstepsLabel_.setBounds(startX, dialY, dialW, 16);
    footstepsSlider_.setBounds(startX, dialY + 16, dialW, dialH);

    bassLabel_.setBounds(startX + 115, dialY, dialW, 16);
    bassSlider_.setBounds(startX + 115, dialY + 16, dialW, dialH);

    preampLabel_.setBounds(startX + 230, dialY, dialW, 16);
    preampSlider_.setBounds(startX + 230, dialY + 16, dialW, dialH);

    // Controles Paramétricos (Derecha)
    const int peqX = startX + 370;

    bandSelectLabel_.setBounds(peqX, dialY, 50, 20);
    bandSelector_.setBounds(peqX + 55, dialY, 110, 24);
    bandEnableToggle_.setBounds(peqX + 175, dialY, 100, 24);

    filterTypeLabel_.setBounds(peqX + 285, dialY, 40, 20);
    filterTypeSelector_.setBounds(peqX + 330, dialY, 160, 24);

    const int peqDialY = dialY + 34;
    freqLabel_.setBounds(peqX, peqDialY, dialW, 16);
    freqSlider_.setBounds(peqX, peqDialY + 16, dialW, dialH);

    gainLabel_.setBounds(peqX + 115, peqDialY, dialW, 16);
    gainSlider_.setBounds(peqX + 115, peqDialY + 16, dialW, dialH);

    qLabel_.setBounds(peqX + 230, peqDialY, dialW, 16);
    qSlider_.setBounds(peqX + 230, peqDialY + 16, dialW, dialH);
}

void EqualizerPanel::notifyChange()
{
    graphicParams_.footstepsDb = static_cast<float>(footstepsSlider_.getValue());
    graphicParams_.bassDb      = static_cast<float>(bassSlider_.getValue());
    graphicParams_.preampDb    = static_cast<float>(preampSlider_.getValue());
    graphicParams_.bypass      = bypassToggle_.getToggleState();

    if (onParametersChanged)
        onParametersChanged(graphicParams_, useParametric_, bands_);

    repaint();
}

void EqualizerPanel::setParameters(
    const dash::core::EQParameters& graphicEq,
    bool useParametric,
    const std::array<dash::core::ParametricBandParameters, dash::core::ParametricEQProcessor::kMaxBands>& bands)
{
    graphicParams_ = graphicEq;
    useParametric_ = useParametric;
    bands_         = bands;

    footstepsSlider_.setValue(graphicEq.footstepsDb, juce::dontSendNotification);
    bassSlider_.setValue(graphicEq.bassDb, juce::dontSendNotification);
    preampSlider_.setValue(graphicEq.preampDb, juce::dontSendNotification);
    bypassToggle_.setToggleState(graphicEq.bypass, juce::dontSendNotification);
    parametricModeToggle_.setToggleState(useParametric, juce::dontSendNotification);

    updateCurrentBandControls();
    repaint();
}

} // namespace dash::ui
