#include <JuceHeader.h>
#include "MainComponent.h"

namespace
{

/**
 * Genera el icono vectorial minimalista de la calavera táctica en color cian (#06B6D4) y blanco.
 * No requiere archivos de imagen externos y escala con máxima nitidez.
 */
juce::Image createSkullIcon(int size = 128)
{
    juce::Image img(juce::Image::ARGB, size, size, true);
    juce::Graphics g(img);

    const float s = static_cast<float>(size);
    g.addTransform(juce::AffineTransform::scale(s, s));

    // 1. Cráneo superior abovedado y mandíbula táctica
    juce::Path skull;
    skull.startNewSubPath(0.20f, 0.48f);
    skull.cubicTo(0.20f, 0.14f, 0.80f, 0.14f, 0.80f, 0.48f);
    // Pómulo derecho y transición a mandíbula
    skull.lineTo(0.74f, 0.62f);
    skull.lineTo(0.68f, 0.62f);
    // Mandíbula
    skull.lineTo(0.65f, 0.84f);
    skull.lineTo(0.35f, 0.84f);
    // Mandíbula izquierda y pómulo izquierdo
    skull.lineTo(0.32f, 0.62f);
    skull.lineTo(0.26f, 0.62f);
    skull.closeSubPath();

    // Relleno cian táctico (#06B6D4)
    g.setColour(juce::Colour(0xff06b6d4));
    g.fillPath(skull);

    // Contorno blanco translúcido elegante
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.strokePath(skull, juce::PathStrokeType(0.015f));

    // 2. Cuencas oculares angulares
    juce::Path leftEye;
    leftEye.startNewSubPath(0.30f, 0.42f);
    leftEye.lineTo(0.44f, 0.45f);
    leftEye.lineTo(0.41f, 0.58f);
    leftEye.lineTo(0.32f, 0.54f);
    leftEye.closeSubPath();

    juce::Path rightEye;
    rightEye.startNewSubPath(0.70f, 0.42f);
    rightEye.lineTo(0.56f, 0.45f);
    rightEye.lineTo(0.59f, 0.58f);
    rightEye.lineTo(0.68f, 0.54f);
    rightEye.closeSubPath();

    g.setColour(juce::Colour(0xff121214)); // Fondo oscuro del cráneo
    g.fillPath(leftEye);
    g.fillPath(rightEye);

    // 3. Cavidad nasal triangular estilizada
    juce::Path nose;
    nose.startNewSubPath(0.50f, 0.58f);
    nose.lineTo(0.46f, 0.68f);
    nose.lineTo(0.54f, 0.68f);
    nose.closeSubPath();
    g.fillPath(nose);

    // 4. Ranuras de dientes en la mandíbula
    g.setColour(juce::Colour(0xff121214));
    g.drawLine(0.42f, 0.74f, 0.42f, 0.84f, 0.035f);
    g.drawLine(0.50f, 0.74f, 0.50f, 0.84f, 0.035f);
    g.drawLine(0.58f, 0.74f, 0.58f, 0.84f, 0.035f);

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
                  juce::Colour(0xff1e1e1e),
                  DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new dash::ui::MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(920, 720, 2560, 1440);
            centreWithSize(getWidth(), getHeight());

            // Icono vectorial minimalista de la aplicación
            const auto skullIcon = createSkullIcon(128);
            setIcon(skullIcon);

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
