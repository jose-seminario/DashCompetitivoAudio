#include <JuceHeader.h>
#include "MainComponent.h"

namespace
{

/**
 * Genera el icono vectorial minimalista de la calavera táctica en color cian neón (#06B6D4) y oscuro.
 * No requiere archivos de imagen externos y escala con máxima nitidez en cualquier resolución de Windows.
 */
juce::Image createSkullIcon(int size = 128)
{
    juce::Image img(juce::Image::ARGB, size, size, true);
    juce::Graphics g(img);

    const float s = static_cast<float>(size);
    g.addTransform(juce::AffineTransform::scale(s, s));

    // Fondo oscuro con forma de placa/emblema táctico para contraste en cualquier barra de tareas
    g.setColour(juce::Colour(0xff0e1117));
    g.fillRoundedRectangle(0.04f, 0.04f, 0.92f, 0.92f, 0.22f);

    // Borde de neón cian (#06B6D4) con resplandor táctico
    g.setColour(juce::Colour(0xff06b6d4));
    g.drawRoundedRectangle(0.04f, 0.04f, 0.92f, 0.92f, 0.22f, 0.035f);

    // 1. Cráneo superior abovedado y mandíbula táctica
    juce::Path skull;
    skull.startNewSubPath(0.24f, 0.46f);
    skull.cubicTo(0.22f, 0.16f, 0.78f, 0.16f, 0.76f, 0.46f); // Bóveda craneal
    // Pómulo derecho
    skull.lineTo(0.72f, 0.58f);
    skull.lineTo(0.66f, 0.58f); // Hendidura temporal
    // Mandíbula derecha
    skull.lineTo(0.64f, 0.80f);
    // Mentón
    skull.lineTo(0.36f, 0.80f);
    // Mandíbula izquierda
    skull.lineTo(0.34f, 0.58f);
    // Pómulo izquierdo
    skull.lineTo(0.28f, 0.58f);
    skull.closeSubPath();

    // Relleno cian táctico (#06B6D4) con gradiente de luz
    juce::ColourGradient grad(
        juce::Colour(0xff22d3ee), 0.5f, 0.18f,
        juce::Colour(0xff06b6d4), 0.5f, 0.80f,
        false);
    g.setGradientFill(grad);
    g.fillPath(skull);

    // Contorno de alta definición
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.strokePath(skull, juce::PathStrokeType(0.02f));

    // 2. Cuencas oculares angulares e inclinadas (estilo gamer / táctico agresivo)
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

    g.setColour(juce::Colour(0xff0e1117)); // Fondo oscuro en cuencas
    g.fillPath(leftEye);
    g.fillPath(rightEye);

    // 3. Cavidad nasal triangular estilizada
    juce::Path nose;
    nose.startNewSubPath(0.50f, 0.56f);
    nose.lineTo(0.46f, 0.65f);
    nose.lineTo(0.54f, 0.65f);
    nose.closeSubPath();
    g.fillPath(nose);

    // 4. Dientes verticales estilizados en la mandíbula
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
                  juce::Colour(0xff1e1e1e),
                  DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new dash::ui::MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(920, 720, 2560, 1440);
            centreWithSize(getWidth(), getHeight());

            // Icono vectorial minimalista de la calavera táctica
            const auto skullIcon = createSkullIcon(128);
            setIcon(skullIcon);

            // Hacer visible para crear el HWND nativo (ComponentPeer) en Windows
            setVisible(true);

            // Reasignar el icono con el peer nativo ya creado para forzar WM_SETICON en la barra de tareas y título
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
