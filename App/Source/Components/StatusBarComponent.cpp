#include "StatusBarComponent.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

StatusBarComponent::StatusBarComponent()
{
    engineStatusLabel_.setText("MOTOR: DETENIDO", juce::dontSendNotification);
    engineStatusLabel_.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
    engineStatusLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(engineStatusLabel_);

    activeProfileLabel_.setText("PERFIL: NINGUNO", juce::dontSendNotification);
    activeProfileLabel_.setFont(juce::FontOptions(11.5f));
    activeProfileLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(activeProfileLabel_);

    formatInfoLabel_.setText("48.0 kHz | 7.1 Surround", juce::dontSendNotification);
    formatInfoLabel_.setFont(juce::FontOptions(11.5f));
    formatInfoLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(formatInfoLabel_);

    statusMessageLabel_.setText(juce::String::fromUTF8("Sistema en espera. Verifica dependencias."), juce::dontSendNotification);
    statusMessageLabel_.setFont(juce::FontOptions(11.5f));
    statusMessageLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(statusMessageLabel_);
}

void StatusBarComponent::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgCard);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());

    // Indicador LED de clipping y estado del motor
    const int ledX = 14;
    const int ledY = getHeight() / 2 - 4;
    g.setColour(isRunning_ ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kWarningAmber);
    g.fillEllipse((float)ledX, (float)ledY, 8.0f, 8.0f);

    if (hasClipped_)
    {
        const int clipX = getWidth() - 120;
        g.setColour(DashLookAndFeel::kDangerCrimson);
        g.fillRoundedRectangle((float)clipX, (float)ledY - 2, 70.0f, 14.0f, 3.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("CLIP 0dBFS", clipX, ledY - 2, 70, 14, juce::Justification::centred);
    }
}

void StatusBarComponent::resized()
{
    auto area = getLocalBounds().reduced(28, 2);

    engineStatusLabel_.setBounds(area.removeFromLeft(140));
    area.removeFromLeft(12);

    activeProfileLabel_.setBounds(area.removeFromLeft(180));
    area.removeFromLeft(12);

    formatInfoLabel_.setBounds(area.removeFromLeft(170));
    area.removeFromLeft(16);

    statusMessageLabel_.setBounds(area.removeFromLeft(area.getWidth() - 130));
}

void StatusBarComponent::setEngineStatus(const juce::String& statusText, bool isRunning)
{
    isRunning_ = isRunning;
    engineStatusLabel_.setText(statusText, juce::dontSendNotification);
    engineStatusLabel_.setColour(juce::Label::textColourId,
                                 isRunning ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kWarningAmber);
    repaint();
}

void StatusBarComponent::setActiveProfileName(const juce::String& profileName)
{
    activeProfileLabel_.setText("PERFIL: " + profileName.toUpperCase(), juce::dontSendNotification);
}

void StatusBarComponent::setAudioFormatInfo(double sampleRate, const juce::String& layoutName)
{
    const juce::String text = juce::String(sampleRate / 1000.0, 1) + " kHz | " + layoutName;
    formatInfoLabel_.setText(text, juce::dontSendNotification);
}

void StatusBarComponent::setClippingDetected(bool hasClipped)
{
    if (hasClipped_ != hasClipped)
    {
        hasClipped_ = hasClipped;
        repaint();
    }
}

void StatusBarComponent::setStatusMessage(const juce::String& message, bool isError)
{
    statusMessageLabel_.setText(message, juce::dontSendNotification);
    statusMessageLabel_.setColour(juce::Label::textColourId,
                                  isError ? DashLookAndFeel::kDangerCrimson : DashLookAndFeel::kTextMuted);
}

} // namespace dash::ui
