#include "SystemStatusPanel.h"
#include "../Theme/DashLookAndFeel.h"

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace dash::ui
{

SystemStatusPanel::SystemStatusPanel()
{
    titleLabel_.setText("ESTADO DEL SISTEMA, PRIVILEGIOS Y DEPENDENCIAS WINDOWS", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    overallStatusLabel_.setFont(juce::FontOptions(12.5f).withStyle("Bold"));
    addAndMakeVisible(overallStatusLabel_);

    recheckBtn_.setButtonText("VOLVER A COMPROBAR");
    recheckBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
    recheckBtn_.onClick = [this] { refreshStatus(); };
    addAndMakeVisible(recheckBtn_);

    runAsAdminBtn_.setButtonText(juce::String::fromUTF8("ELEVACI\xc3\x93N UAC (ADMINISTRADOR)"));
    runAsAdminBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kWarningAmber);
    runAsAdminBtn_.onClick = [] {
#ifdef _WIN32
        ShellExecuteW(nullptr, L"runas", juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName().toWideCharPointer(), nullptr, nullptr, SW_SHOWNORMAL);
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
#endif
    };
    addAndMakeVisible(runAsAdminBtn_);

    detailsEditor_.setMultiLine(true);
    detailsEditor_.setReadOnly(true);
    detailsEditor_.setColour(juce::TextEditor::backgroundColourId, DashLookAndFeel::kBgCardElevated);
    detailsEditor_.setColour(juce::TextEditor::textColourId, DashLookAndFeel::kTextMain);
    detailsEditor_.setColour(juce::TextEditor::outlineColourId, DashLookAndFeel::kBorderSubtle);
    detailsEditor_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(detailsEditor_);

    refreshStatus();
}

void SystemStatusPanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const auto cardArea = juce::Rectangle<float>(16, 44, (float)getWidth() - 32, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(cardArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(cardArea, 8.0f, 1.2f);
}

void SystemStatusPanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);

    const int startX = 36;
    const int contentW = getWidth() - 72;

    overallStatusLabel_.setBounds(startX, 60, contentW - 380, 24);
    recheckBtn_.setBounds(startX + contentW - 370, 56, 175, 30);
    runAsAdminBtn_.setBounds(startX + contentW - 185, 56, 185, 30);

    detailsEditor_.setBounds(startX, 100, contentW, getHeight() - 175);
}

void SystemStatusPanel::refreshStatus()
{
    status_ = dash::audio::SystemDependencyChecker::inspectSystem(policyManager_);

    std::error_code ec;
    const bool hasVstDir = !status_.apoDllPath.empty()
        && std::filesystem::exists(status_.apoDllPath.parent_path() / "VSTPlugins", ec);

    juce::String report;
    report << "=== REPORTE DE DEPENDENCIAS DEL SISTEMA ===\n\n";

    report << "1. Equalizer APO Instalado: " << (status_.hasEqualizerApo ? "[OK] Detectado" : "[FALTA] No encontrado") << "\n";
    if (status_.hasEqualizerApo)
        report << "   Ruta DLL: " << juce::String(status_.apoDllPath.string()) << "\n\n";
    else
        report << "\n";

    report << "2. Directorio de Plugins VST (EqualizerAPO\\VSTPlugins): "
           << (hasVstDir ? "[OK] Presente" : "[ADVERTENCIA] Requiere despliegue") << "\n\n";

    report << "3. Motor Binaural HeSuVi: "
           << (status_.hasHeSuVi ? "[OK] Detectado" : "[OPCIONAL] No detectado") << "\n";
    if (status_.hasHeSuVi)
        report << "   Ruta HeSuVi: " << juce::String(status_.hesuviDirPath.string()) << "\n\n";
    else
        report << "\n";

    report << "4. Cable de Audio Virtual (VB-Audio / Hi-Fi): "
           << (status_.hasVbAudioCable ? "[OK] Detectado" : "[OPCIONAL] No detectado (usando modo directo)") << "\n";
    if (status_.hasVbAudioCable && !status_.detectedCableName.empty())
        report << "   Dispositivo: " << juce::String(status_.detectedCableName.c_str()) << "\n\n";
    else
        report << "\n";

    report << "5. Privilegios de Administrador del Proceso: "
           << (status_.isRunningAsAdmin ? "[OK] Proceso elevado (acceso de escritura en Program Files)" : "[INFO] Usuario estándar (requiere elevación UAC para config.txt)") << "\n";

    detailsEditor_.setText(report);

    const bool allOk = status_.hasEqualizerApo && status_.isRunningAsAdmin;
    overallStatusLabel_.setText(allOk ? juce::String::fromUTF8("ESTADO GENERAL: LISTO PARA PROCESAMIENTO") : juce::String::fromUTF8("ESTADO GENERAL: ACCI\xc3\x93N REQUERIDA"), juce::dontSendNotification);
    overallStatusLabel_.setColour(juce::Label::textColourId, allOk ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kWarningAmber);

    runAsAdminBtn_.setVisible(!status_.isRunningAsAdmin);
}

} // namespace dash::ui
