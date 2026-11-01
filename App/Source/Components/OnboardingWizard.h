#pragma once

#include <JuceHeader.h>
#include <functional>
#include <string>
#include "../Core/SystemDependencyChecker.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

/**
 * Asistente de Primera Vez (Onboarding Wizard).
 *
 * Se presenta a pantalla completa cuando SystemDependencyChecker detecta
 * componentes faltantes (Equalizer APO, VB-Audio Cable, etc.).
 * Guía al usuario para instalar automáticamente o descargar manualmente
 * las dependencias requeridas para el motor acústico táctico.
 */
class OnboardingWizard final : public juce::Component
{
public:
    OnboardingWizard();
    ~OnboardingWizard() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Actualiza el listado de componentes y dependencias detectadas.
     */
    void updateStatus(const dash::audio::DependencyStatus& status);

    /**
     * Alterna la interfaz hacia el estado de instalación activa con mensaje de progreso.
     */
    void setInstallingState(bool isInstalling, const juce::String& statusMessage = {});

    // Callbacks de interacción hacia MainComponent
    std::function<void()> onAutoInstallRequested;
    std::function<void()> onManualDownloadRequested;
    std::function<void()> onRecheckRequested;
    std::function<void()> onDismissWizard;

private:
    dash::audio::DependencyStatus currentStatus_;
    bool isInstalling_ = false;

    // Encabezado
    juce::Label badgeLabel_;
    juce::Label titleLabel_;
    juce::Label subtitleLabel_;

    // Tarjeta de dependencias faltantes
    juce::Label missingHeaderLabel_;
    juce::TextEditor missingListEditor_;

    // Mensaje de advertencia de privilegios
    juce::Label adminWarningLabel_;

    // Botones de acción principal
    juce::TextButton autoInstallBtn_;
    juce::TextButton manualDownloadBtn_;
    juce::TextButton recheckBtn_;
    juce::TextButton skipBtn_;

    // Estado / progreso de instalación
    juce::Label progressLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnboardingWizard)
};

} // namespace dash::ui
