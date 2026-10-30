#include <JuceHeader.h>
#include "MainComponent.h"

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
                  juce::Colour(0xff1e1e1e),
                  DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new dash::ui::MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(780, 580, 2560, 1440);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
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
