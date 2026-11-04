#include <JuceHeader.h>
#include "MainComponent.h"

namespace
{

juce::Image createSkullIcon(int size = 128)
{
    juce::Image img(juce::Image::ARGB, size, size, true);
    juce::Graphics g(img);

    const float s = static_cast<float>(size);
    g.addTransform(juce::AffineTransform::scale(s, s));

    g.setColour(juce::Colour(0xff0e1117));
    g.fillRoundedRectangle(0.04f, 0.04f, 0.92f, 0.92f, 0.22f);

    g.setColour(juce::Colour(0xff06b6d4));
    g.drawRoundedRectangle(0.04f, 0.04f, 0.92f, 0.92f, 0.22f, 0.035f);

    juce::Path skull;
    skull.startNewSubPath(0.24f, 0.46f);
    skull.cubicTo(0.22f, 0.16f, 0.78f, 0.16f, 0.76f, 0.46f);
    skull.lineTo(0.72f, 0.58f);
    skull.lineTo(0.66f, 0.58f);
    skull.lineTo(0.64f, 0.80f);
    skull.lineTo(0.36f, 0.80f);
    skull.lineTo(0.34f, 0.58f);
    skull.lineTo(0.28f, 0.58f);
    skull.closeSubPath();

    juce::ColourGradient grad(
        juce::Colour(0xff22d3ee), 0.5f, 0.18f,
        juce::Colour(0xff06b6d4), 0.5f, 0.80f,
        false);
    g.setGradientFill(grad);
    g.fillPath(skull);

    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.strokePath(skull, juce::PathStrokeType(0.02f));

    juce::Path leftEye;
    leftEye.startNewSubPath(0.32f, 0.40f);
    leftEye.lineTo(0.45f, 0.45f);
    leftEye.lineTo(0.42f, 0.56f);
    leftEye.lineTo(0.33f, 0.50f);
    leftEye.closeSubPath();

    juce::Path rightEye;
    rightEye.startNewSubPath(0.68f, 0.40f);
    rightEye.lineTo(0.55f, 0.45f);
    rightEye.lineTo(0.58f, 0.56f);
    rightEye.lineTo(0.67f, 0.50f);
    rightEye.closeSubPath();

    g.setColour(juce::Colour(0xff0e1117));
    g.fillPath(leftEye);
    g.fillPath(rightEye);

    juce::Path nose;
    nose.startNewSubPath(0.50f, 0.56f);
    nose.lineTo(0.46f, 0.65f);
    nose.lineTo(0.54f, 0.65f);
    nose.closeSubPath();
    g.fillPath(nose);

    g.setColour(juce::Colour(0xff0e1117));
    g.drawLine(0.42f, 0.70f, 0.42f, 0.80f, 0.035f);
    g.drawLine(0.50f, 0.70f, 0.50f, 0.80f, 0.035f);
    g.drawLine(0.58f, 0.70f, 0.58f, 0.80f, 0.035f);

    return img;
}

} // namespace

class DashCompetitivoAudioApplication final : public juce::JUCEApplication
{
public:
    DashCompetitivoAudioApplication() = default;

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    void initialise(const juce::String&) override
    {
        mainWindow_ = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow_.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(
                  name,
                  dash::ui::DashLookAndFeel::kBgDark,
                  DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new dash::ui::MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(780, 480, 2560, 1440);
            centreWithSize(960, 580);

            juce::Image skullIcon;
            juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
            juce::File imgFile = exeDir.getChildFile("icon.png");
            if (!imgFile.existsAsFile())
                imgFile = exeDir.getChildFile("icon.ico");
            if (!imgFile.existsAsFile())
                imgFile = juce::File::getCurrentWorkingDirectory().getChildFile("icon.png");
            if (!imgFile.existsAsFile())
                imgFile = juce::File::getCurrentWorkingDirectory().getChildFile("icon.ico");

            if (imgFile.existsAsFile())
                skullIcon = juce::ImageFileFormat::loadFrom(imgFile);

            if (skullIcon.isNull())
                skullIcon = createSkullIcon(128);

            setIcon(skullIcon);
            setVisible(true);
            setIcon(skullIcon);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow_;
};

START_JUCE_APPLICATION(DashCompetitivoAudioApplication)
