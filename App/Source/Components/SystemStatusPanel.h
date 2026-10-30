#pragma once

#include <JuceHeader.h>
#include "../Core/SystemDependencyChecker.h"

namespace dash::ui
{

/**
 * Panel de Diagnóstico del Sistema y Dependencias (SystemStatusPanel).
 *
 * Verifica la presencia de Equalizer APO, permisos de escritura NTFS en config.txt,
 * elevación de privilegios de administrador y cable de audio virtual.
 */
class SystemStatusPanel final : public juce::Component
{
public:
    SystemStatusPanel();
    ~SystemStatusPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshStatus();

private:
    dash::audio::SystemDependencyChecker checker_;
    dash::audio::DependencyStatus status_;

    juce::Label titleLabel_;
    juce::Label overallStatusLabel_;

    juce::TextButton recheckBtn_;
    juce::TextButton runAsAdminBtn_;

    juce::TextEditor detailsEditor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemStatusPanel)
};

} // namespace dash::ui
