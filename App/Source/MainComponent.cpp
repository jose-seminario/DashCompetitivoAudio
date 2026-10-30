#include "MainComponent.h"

#include <cwctype>
#include <algorithm>
#include <thread>

namespace dash::ui
{

namespace
{
    juce::String u8(const char* utf8Text)
    {
        return juce::String::fromUTF8(utf8Text);
    }

    bool isVirtualCableEndpointName(std::wstring name)
    {
        std::transform(name.begin(), name.end(), name.begin(),
                       [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return name.find(L"cable input") != std::wstring::npos
            || name.find(L"vb-audio") != std::wstring::npos
            || name.find(L"voicemeeter") != std::wstring::npos;
    }
} // namespace

MainComponent::MainComponent()
    : apoConfigPath_(dash::audio::ApoConfigInjector::getDefaultApoConfigPath())
{
    setSize(1040, 790);

    // =========================================================================
    // 1. Paleta Oscura Profesional (#1E1E1E)
    // =========================================================================
    auto& lf = getLookAndFeel();
    lf.setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
    lf.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d2d30));
    lf.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3f3f46));
    lf.setColour(juce::ComboBox::textColourId, juce::Colour(0xfff4f4f5));
    lf.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffa1a1aa));
    lf.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff252528));
    lf.setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff4f4f5));
    lf.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3f3f46));

    // =========================================================================
    // 2. Encabezado Centrado
    // =========================================================================
    titleLabel_.setText(u8("Dash Competitivo Audio"), juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f4f5));
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setText(
        u8("Orquestador Táctico en Tiempo Real • Edición Warzone Pro (C++20 / WASAPI / APO)"),
        juce::dontSendNotification);
    subtitleLabel_.setFont(juce::FontOptions(13.0f));
    subtitleLabel_.setJustificationType(juce::Justification::centred);
    subtitleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
    addAndMakeVisible(subtitleLabel_);

    // =========================================================================
    // 3. Sección de Perfiles y Conmutación COM
    // =========================================================================
    profileSectionHeader_.setText(u8("ORQUESTADOR DE PERFILES Y RUTA DE AUDIO"), juce::dontSendNotification);
    profileSectionHeader_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    profileSectionHeader_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(profileSectionHeader_);

    profileSelectLabel_.setText(u8("Perfil:"), juce::dontSendNotification);
    profileSelectLabel_.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    profileSelectLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffe4e4e7));
    addAndMakeVisible(profileSelectLabel_);

    profileSelector_.addItem("Campaign", 1);
    profileSelector_.addItem("Eclipse", 2);
    profileSelector_.setSelectedId(1, juce::dontSendNotification);
    profileSelector_.onChange = [this]() {
        applyPresetValuesToSliders(profileSelector_.getSelectedId(), true);
    };
    addAndMakeVisible(profileSelector_);

    activateProfileBtn_.setButtonText(u8("Activar Perfil"));
    activateProfileBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff10b981));
    activateProfileBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff059669));
    activateProfileBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    activateProfileBtn_.onClick = [this]() { activateTacticalProfile(); };
    addAndMakeVisible(activateProfileBtn_);

    deactivateProfileBtn_.setButtonText(u8("Desactivar / Restaurar"));
    deactivateProfileBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff27272a));
    deactivateProfileBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3f3f46));
    deactivateProfileBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd4d4d8));
    deactivateProfileBtn_.onClick = [this]() { deactivateAndRestoreAudio(); };
    addAndMakeVisible(deactivateProfileBtn_);

    routeInfoLabel_.setText(u8("Estado Ruta: Original del Sistema"), juce::dontSendNotification);
    routeInfoLabel_.setFont(juce::FontOptions(12.5f));
    routeInfoLabel_.setJustificationType(juce::Justification::centredRight);
    routeInfoLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
    addAndMakeVisible(routeInfoLabel_);

    // =========================================================================
    // 4. Perillas de Compresor (Fila 1) y Personalización Extrema Warzone (Fila 2)
    // =========================================================================
    compressorSectionHeader_.setText(
        u8("COMPRESOR DINÁMICO HÍBRIDO (ITU-R BS.1770 - LOUDNESS COMPRESSOR)"),
        juce::dontSendNotification);
    compressorSectionHeader_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    compressorSectionHeader_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(compressorSectionHeader_);

    warzoneSectionHeader_.setText(
        u8("PERSONALIZACIÓN EXTREMA WARZONE (SUPRESOR DE DISPAROS • EQ PASOS/GRAVES • AMBIENTE)"),
        juce::dontSendNotification);
    warzoneSectionHeader_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    warzoneSectionHeader_.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    addAndMakeVisible(warzoneSectionHeader_);

    // Fila 1: Línea base de nuestro motor DSP para el perfil Campaign
    initRotarySlider(amountSlider_,  amountLabel_,  "Intensidad (Amount)", 0.0, 1.0, 0.001,    0.75,     2, {},      juce::Colour(0xff0284c7));
    initRotarySlider(targetSlider_,  targetLabel_,  "Objetivo (Target)",   0.0, 1.0, 0.000001, 0.277778, 6, {},      juce::Colour(0xff0284c7));
    initRotarySlider(releaseSlider_, releaseLabel_, "Release",             0.0, 1.0, 0.001,    0.50,     2, {},      juce::Colour(0xff0284c7));

    targetLufsBadge_.setText("(-18 LUFS)", juce::dontSendNotification);
    targetLufsBadge_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    targetLufsBadge_.setJustificationType(juce::Justification::centred);
    targetLufsBadge_.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    addAndMakeVisible(targetLufsBadge_);

    // Fila 2: Perillas Tácticas Warzone
    initRotarySlider(gunSlider_,       gunLabel_,       "Supresor (Gun Crack)", 0.0,  1.0, 0.01, 0.45, 2, {},    juce::Colour(0xff10b981));
    initRotarySlider(footstepsSlider_, footstepsLabel_, "Pasos (Footsteps)",   -3.0,  6.0, 0.1,  1.6,  1, " dB", juce::Colour(0xff10b981));
    initRotarySlider(bassSlider_,      bassLabel_,      "Graves (Bass)",       -6.0,  6.0, 0.1,  1.5,  1, " dB", juce::Colour(0xff10b981));
    initRotarySlider(ambienceSlider_,  ambienceLabel_,  "Ambiente (Ambience)",  0.0,  1.0, 0.01, 1.0,  2, {},    juce::Colour(0xff10b981));

    // =========================================================================
    // 5. Panel "Configuración del Sistema" (System Checker + Guía Paso a Paso)
    // =========================================================================
    sysConfigSectionHeader_.setText(u8("CONFIGURACIÓN DEL SISTEMA (VERIFICADOR DE DEPENDENCIAS)"), juce::dontSendNotification);
    sysConfigSectionHeader_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    sysConfigSectionHeader_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(sysConfigSectionHeader_);

    depStatusLabel_.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    addAndMakeVisible(depStatusLabel_);

    installDepsBtn_.setButtonText(u8("Instalar Dependencias"));
    installDepsBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2563eb));
    installDepsBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1d4ed8));
    installDepsBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    installDepsBtn_.onClick = [this]() { onInstallDependenciesClicked(); };
    addAndMakeVisible(installDepsBtn_);

    recheckDepsBtn_.setButtonText(u8("Reverificar"));
    recheckDepsBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff27272a));
    recheckDepsBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe4e4e7));
    recheckDepsBtn_.onClick = [this]() { refreshSystemDependencies(); };
    addAndMakeVisible(recheckDepsBtn_);

    runAsAdminBtn_.setButtonText(u8("Reiniciar como Admin"));
    runAsAdminBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd97706));
    runAsAdminBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    runAsAdminBtn_.onClick = [this]() {
        if (dash::audio::SystemDependencyChecker::relaunchAsAdministrator())
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
    };
    addAndMakeVisible(runAsAdminBtn_);

    installGuideEditor_.setMultiLine(true);
    installGuideEditor_.setReadOnly(true);
    installGuideEditor_.setScrollbarsShown(true);
    installGuideEditor_.setCaretVisible(false);
    installGuideEditor_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff18181b));
    installGuideEditor_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3f3f46));
    installGuideEditor_.setColour(juce::TextEditor::textColourId, juce::Colour(0xffd4d4d8));
    installGuideEditor_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    addAndMakeVisible(installGuideEditor_);

    // =========================================================================
    // 6. Barra de Estado Inferior
    // =========================================================================
    statusLabel_.setFont(juce::FontOptions(12.5f));
    statusLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff18181b));
    statusLabel_.setColour(juce::Label::outlineColourId, juce::Colour(0xff27272a));
    addAndMakeVisible(statusLabel_);

    // Capturar el endpoint predeterminado inicial del usuario para poder restaurarlo al desactivar
    const auto initialEndpoints = policyManager_.enumerateActiveRenderEndpoints();
    for (const auto& ep : initialEndpoints)
    {
        if (ep.isDefaultConsole && !isVirtualCableEndpointName(ep.friendlyName))
        {
            originalEndpointId_   = ep.id;
            originalEndpointName_ = ep.friendlyName;
            break;
        }
    }

    refreshSystemDependencies();
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::initRotarySlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& titleUtf8,
    double minVal,
    double maxVal,
    double step,
    double defaultVal,
    int decimalPlaces,
    const juce::String& suffix,
    juce::Colour accentColour)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 20);
    slider.setRange(minVal, maxVal, step);
    slider.setValue(defaultVal, juce::dontSendNotification);
    slider.setNumDecimalPlacesToDisplay(decimalPlaces);

    if (suffix.isNotEmpty())
        slider.setTextValueSuffix(suffix);

    slider.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff27272a));
    slider.setColour(juce::Slider::thumbColourId, accentColour.brighter(0.35f));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff18181b));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3f3f46));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff4f4f5));

    slider.onValueChange = [this]() {
        if (!isUpdatingPresetSliders_)
        {
            const double lufs = -28.0 + targetSlider_.getValue() * 36.0;
            targetLufsBadge_.setText("(" + juce::String(lufs, 1) + " LUFS)", juce::dontSendNotification);
            scheduleDebouncedInjection();
        }
    };

    addAndMakeVisible(slider);

    label.setText(u8(titleUtf8.toRawUTF8()), juce::dontSendNotification);
    label.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffe4e4e7));
    addAndMakeVisible(label);
}

void MainComponent::applyPresetValuesToSliders(int profileId, bool triggerInjection)
{
    isUpdatingPresetSliders_ = true;

    if (profileId == 1)
    {
        // Perfil "Campaign": Punto de partida estándar de nuestro motor para audio competitivo equilibrado
        amountSlider_.setValue(0.75, juce::dontSendNotification);
        targetSlider_.setValue(0.277778, juce::dontSendNotification);
        releaseSlider_.setValue(0.50, juce::dontSendNotification);
        gunSlider_.setValue(0.45, juce::dontSendNotification);
        footstepsSlider_.setValue(1.6, juce::dontSendNotification);
        bassSlider_.setValue(1.5, juce::dontSendNotification);
        ambienceSlider_.setValue(1.0, juce::dontSendNotification);
        targetLufsBadge_.setText("(-18.0 LUFS)", juce::dontSendNotification);
    }
    else
    {
        // Perfil "Eclipse": Enfoque competitivo agresivo para pasos lejanos y supresión intensa
        amountSlider_.setValue(0.85, juce::dontSendNotification);
        targetSlider_.setValue(0.333333, juce::dontSendNotification);
        releaseSlider_.setValue(0.40, juce::dontSendNotification);
        gunSlider_.setValue(0.35, juce::dontSendNotification);
        footstepsSlider_.setValue(3.4, juce::dontSendNotification);
        bassSlider_.setValue(0.2, juce::dontSendNotification);
        ambienceSlider_.setValue(1.0, juce::dontSendNotification);
        targetLufsBadge_.setText("(-16.0 LUFS)", juce::dontSendNotification);
    }

    isUpdatingPresetSliders_ = false;

    if (triggerInjection)
        scheduleDebouncedInjection();
}

void MainComponent::scheduleDebouncedInjection()
{
    startTimer(kDebounceMs);
}

void MainComponent::timerCallback()
{
    stopTimer();
    rebuildAndInjectConfig(false);
}

void MainComponent::rebuildAndInjectConfig(bool forcePermissionPopup)
{
    apoConfig_.profileName     = profileSelector_.getText().toStdString();
    apoConfig_.compAmount      = amountSlider_.getValue();
    apoConfig_.compTarget      = targetSlider_.getValue();
    apoConfig_.compRelease     = releaseSlider_.getValue();
    apoConfig_.gunVolume       = gunSlider_.getValue();
    apoConfig_.footstepsEqDb   = footstepsSlider_.getValue();
    apoConfig_.bassEqDb        = bassSlider_.getValue();
    apoConfig_.ambGuard        = ambienceSlider_.getValue();

    // Si hay un Cable Virtual detectado y el perfil está activo, asociamos su GUID o aplicamos globalmente
    apoConfig_.targetEndpointGuid.clear();
    if (isProfileActive_ && !depStatus_.detectedCableGuid.empty())
        apoConfig_.targetEndpointGuid = juce::String(depStatus_.detectedCableGuid.c_str()).toStdString();

    std::string err;
    bool permissionDenied = false;

    if (dash::audio::ApoConfigInjector::writeConfigAtomically(
            apoConfigPath_, apoConfig_, &err, &permissionDenied))
    {
        setStatusMessage(
            u8("\xE2\x9C\x93 Config.txt inyectado atómicamente [") + profileSelector_.getText()
            + " | Gun " + juce::String(apoConfig_.gunVolume, 2)
            + " | Pasos " + juce::String(apoConfig_.footstepsEqDb, 1) + " dB"
            + " | Graves " + juce::String(apoConfig_.bassEqDb, 1) + " dB]",
            false);
    }
    else
    {
        if (permissionDenied)
            handlePermissionDeniedAlert(forcePermissionPopup);
        else
            setStatusMessage("Error al inyectar config.txt: " + u8(err.c_str()), true);
    }
}

void MainComponent::activateTacticalProfile()
{
    stopTimer();
    depStatus_ = dash::audio::SystemDependencyChecker::inspectSystem(policyManager_);

    // Guardar el dispositivo físico actual antes de conmutar al Cable Input
    const auto endpoints = policyManager_.enumerateActiveRenderEndpoints();
    for (const auto& ep : endpoints)
    {
        if (ep.isDefaultConsole && !isVirtualCableEndpointName(ep.friendlyName))
        {
            originalEndpointId_   = ep.id;
            originalEndpointName_ = ep.friendlyName;
            break;
        }
    }

    juce::String switchedDeviceMsg = u8("Dispositivo actual conservado");
    if (!depStatus_.detectedCableEndpointId.empty())
    {
        std::string comErr;
        if (policyManager_.setDefaultEndpoint(depStatus_.detectedCableEndpointId, &comErr))
        {
            isProfileActive_ = true;
            switchedDeviceMsg = "Audio -> " + juce::String(depStatus_.detectedCableName.c_str());
            routeInfoLabel_.setText(switchedDeviceMsg, juce::dontSendNotification);
            routeInfoLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
        }
        else
        {
            setStatusMessage("Error COM al conmutar a Cable Input: " + u8(comErr.c_str()), true);
        }
    }
    else
    {
        isProfileActive_ = true;
        routeInfoLabel_.setText(u8("Sin Cable Input (Inyección sobre salida actual)"), juce::dontSendNotification);
        routeInfoLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfffbbf24));
    }

    rebuildAndInjectConfig(true);
}

void MainComponent::deactivateAndRestoreAudio()
{
    stopTimer();
    isProfileActive_ = false;

    // 1. Devolver el audio al dispositivo físico original del usuario
    std::wstring restoreId   = originalEndpointId_;
    std::wstring restoreName = originalEndpointName_;

    if (restoreId.empty())
    {
        const auto endpoints = policyManager_.enumerateActiveRenderEndpoints();
        for (const auto& ep : endpoints)
        {
            if (!isVirtualCableEndpointName(ep.friendlyName))
            {
                restoreId   = ep.id;
                restoreName = ep.friendlyName;
                break;
            }
        }
    }

    if (!restoreId.empty())
    {
        std::string comErr;
        if (policyManager_.setDefaultEndpoint(restoreId, &comErr))
        {
            routeInfoLabel_.setText("Restaurado: " + juce::String(restoreName.c_str()), juce::dontSendNotification);
            routeInfoLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
        }
    }

    // 2. Escribir un config.txt vacío de forma atómica (Bypass total en Equalizer APO)
    std::string apoErr;
    bool permissionDenied = false;

    if (dash::audio::ApoConfigInjector::writeEmptyConfigAtomically(apoConfigPath_, &apoErr, &permissionDenied))
    {
        setStatusMessage("Sistema restaurado: dispositivo original activo y config.txt vaciado.", false);
    }
    else
    {
        if (permissionDenied)
            handlePermissionDeniedAlert(true);
        else
            setStatusMessage("Error al vaciar config.txt: " + u8(apoErr.c_str()), true);
    }
}

void MainComponent::refreshSystemDependencies()
{
    depStatus_ = dash::audio::SystemDependencyChecker::inspectSystem(policyManager_);

    runAsAdminBtn_.setVisible(!depStatus_.isRunningAsAdmin);

    if (!depStatus_.allDependenciesReady())
    {
        depStatusLabel_.setText(
            u8("\xE2\x9A\xA0\xEF\xB8\x8F Faltan dependencias para el audio táctico"),
            juce::dontSendNotification);
        depStatusLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffef4444)); // Rojo alerta
    }
    else
    {
        depStatusLabel_.setText(
            u8("\xE2\x9C\x93 Dependencias tácticas completas (Equalizer APO + VB-Audio + HeSuVi)"),
            juce::dontSendNotification);
        depStatusLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff10b981)); // Verde OK
    }

    const std::string guide = dash::audio::SystemDependencyChecker::buildInstallationGuideText(depStatus_);
    installGuideEditor_.setText(u8(guide.c_str()), false);

    if (!depStatus_.allDependenciesReady())
        setStatusMessage(u8("Revisa el panel 'Configuración del Sistema' para instalar las dependencias faltantes."), true);
    else
        setStatusMessage(u8("Dependencias verificadas • Perfil Campaign cargado y listo."), false);

    resized();
}

void MainComponent::onInstallDependenciesClicked()
{
    depStatus_ = dash::audio::SystemDependencyChecker::inspectSystem(policyManager_);

    // Abrir las páginas oficiales de Equalizer APO y VB-Cable si faltan (o ambas como referencia)
    const bool needApo     = !depStatus_.hasEqualizerApo;
    const bool needVbCable = !depStatus_.hasVbAudioCable;
    dash::audio::SystemDependencyChecker::openOfficialDownloadLinks(
        needApo || depStatus_.allDependenciesReady(),
        needVbCable || depStatus_.allDependenciesReady(),
        false);

    installGuideEditor_.grabKeyboardFocus();

    // Si HeSuVi no está instalado, descargar automáticamente HeSuVi_2.0.0.1.exe en segundo plano vía URLDownloadToFileW
    if (!depStatus_.hasHeSuVi && !isDownloadingHeSuVi_)
    {
        isDownloadingHeSuVi_ = true;
        installDepsBtn_.setEnabled(false);
        setStatusMessage(
            u8("Descargando automáticamente HeSuVi_2.0.0.1.exe desde SourceForge (URLDownloadToFile)..."),
            false);

        juce::Component::SafePointer<MainComponent> safeThis(this);
        std::thread([safeThis]() {
            std::filesystem::path savedPath;
            std::string msg;
            const bool ok = dash::audio::SystemDependencyChecker::downloadAndLaunchHeSuViInstaller(&savedPath, &msg);

            juce::MessageManager::callAsync([safeThis, ok, msg]() {
                if (safeThis == nullptr)
                    return;

                safeThis->isDownloadingHeSuVi_ = false;
                safeThis->installDepsBtn_.setEnabled(true);
                safeThis->refreshSystemDependencies();
                safeThis->setStatusMessage(u8(msg.c_str()), !ok);
            });
        }).detach();
        return;
    }

    refreshSystemDependencies();
    setStatusMessage(
        u8("Enlaces oficiales abiertos en tu navegador. Sigue la guía paso a paso en pantalla."),
        false);
}

void MainComponent::handlePermissionDeniedAlert(bool forcePopup)
{
    setStatusMessage(
        u8("Permiso denegado en C:\\Program Files\\EqualizerAPO\\config\\config.txt. Ejecuta la app como Administrador."),
        true);

    if (forcePopup || !hasShownAdminPopup_)
    {
        hasShownAdminPopup_ = true;
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            u8("Permisos de Administrador Requeridos"),
            u8("Dash Competitivo Audio necesita permisos de Administrador para escribir atómicamente en:\n\n"
               "C:\\Program Files\\EqualizerAPO\\config\\config.txt\n\n"
               "Haz clic en el botón 'Reiniciar como Admin' en el panel inferior o cierra la aplicación y ábrela con 'Ejecutar como administrador'."),
            this);
    }
}

void MainComponent::setStatusMessage(const juce::String& textUtf8, bool isError)
{
    statusLabel_.setColour(
        juce::Label::textColourId,
        isError ? juce::Colour(0xfff87171) : juce::Colour(0xff34d399));
    statusLabel_.setText("  " + textUtf8, juce::dontSendNotification);
}

void MainComponent::paint(juce::Graphics& g)
{
    // Fondo oscuro principal #1E1E1E
    g.fillAll(juce::Colour(0xff1e1e1e));

    g.setColour(juce::Colour(0xff252528));
    g.fillRoundedRectangle(profilePanelBounds_.toFloat(), 8.0f);
    g.fillRoundedRectangle(dspPanelBounds_.toFloat(), 8.0f);
    g.fillRoundedRectangle(sysConfigPanelBounds_.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff38383c));
    g.drawRoundedRectangle(profilePanelBounds_.toFloat(), 8.0f, 1.2f);
    g.drawRoundedRectangle(dspPanelBounds_.toFloat(), 8.0f, 1.2f);
    g.drawRoundedRectangle(sysConfigPanelBounds_.toFloat(), 8.0f, 1.2f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(18, 14);

    // 1. Encabezado Centrado
    auto headerArea = area.removeFromTop(50);
    titleLabel_.setBounds(headerArea.removeFromTop(30));
    subtitleLabel_.setBounds(headerArea);

    area.removeFromTop(8);

    // 5. Barra de Estado Inferior
    statusLabel_.setBounds(area.removeFromBottom(28));
    area.removeFromBottom(8);

    // 2. Panel de Perfiles y Ruta COM
    profilePanelBounds_ = area.removeFromTop(96);
    {
        auto pInner = profilePanelBounds_.reduced(16, 10);
        auto topBar = pInner.removeFromTop(18);
        routeInfoLabel_.setBounds(topBar.removeFromRight(360));
        profileSectionHeader_.setBounds(topBar);

        pInner.removeFromTop(8);
        auto row = pInner.removeFromTop(38);
        profileSelectLabel_.setBounds(row.removeFromLeft(50));
        profileSelector_.setBounds(row.removeFromLeft(180).reduced(0, 3));
        row.removeFromLeft(16);
        activateProfileBtn_.setBounds(row.removeFromLeft(210));
        row.removeFromLeft(10);
        deactivateProfileBtn_.setBounds(row.removeFromLeft(195));
    }

    area.removeFromTop(10);

    // 3. Panel de Perillas (Fila 1: Compresor 3 perillas | Fila 2: Warzone 4 perillas)
    dspPanelBounds_ = area.removeFromTop(344);
    {
        auto dInner = dspPanelBounds_.reduced(16, 10);

        // Fila 1: Compresor (Amount, Target, Release)
        compressorSectionHeader_.setBounds(dInner.removeFromTop(18));
        auto row1 = dInner.removeFromTop(138);
        const int w1 = row1.getWidth() / 3;

        auto cAmount = row1.removeFromLeft(w1).reduced(8, 2);
        amountLabel_.setBounds(cAmount.removeFromTop(18));
        amountSlider_.setBounds(cAmount);

        auto cTarget = row1.removeFromLeft(w1).reduced(8, 2);
        targetLabel_.setBounds(cTarget.removeFromTop(16));
        targetLufsBadge_.setBounds(cTarget.removeFromTop(14));
        targetSlider_.setBounds(cTarget);

        auto cRelease = row1.reduced(8, 2);
        releaseLabel_.setBounds(cRelease.removeFromTop(18));
        releaseSlider_.setBounds(cRelease);

        dInner.removeFromTop(8);

        // Fila 2: Personalización Extrema Warzone (Gun Crack, Footsteps, Bass, Ambience)
        warzoneSectionHeader_.setBounds(dInner.removeFromTop(18));
        auto row2 = dInner;
        const int w2 = row2.getWidth() / 4;

        auto cGun = row2.removeFromLeft(w2).reduced(6, 2);
        gunLabel_.setBounds(cGun.removeFromTop(18));
        gunSlider_.setBounds(cGun);

        auto cSteps = row2.removeFromLeft(w2).reduced(6, 2);
        footstepsLabel_.setBounds(cSteps.removeFromTop(18));
        footstepsSlider_.setBounds(cSteps);

        auto cBass = row2.removeFromLeft(w2).reduced(6, 2);
        bassLabel_.setBounds(cBass.removeFromTop(18));
        bassSlider_.setBounds(cBass);

        auto cAmb = row2.reduced(6, 2);
        ambienceLabel_.setBounds(cAmb.removeFromTop(18));
        ambienceSlider_.setBounds(cAmb);
    }

    area.removeFromTop(10);

    // 4. Panel "Configuración del Sistema" (System Checker + Botón Instalar + Guía TextEditor)
    sysConfigPanelBounds_ = area;
    {
        auto sInner = sysConfigPanelBounds_.reduced(16, 10);
        sysConfigSectionHeader_.setBounds(sInner.removeFromTop(18));
        sInner.removeFromTop(4);

        auto bannerRow = sInner.removeFromTop(30);
        installDepsBtn_.setBounds(bannerRow.removeFromRight(185));
        bannerRow.removeFromRight(8);
        recheckDepsBtn_.setBounds(bannerRow.removeFromRight(105));
        if (runAsAdminBtn_.isVisible())
        {
            bannerRow.removeFromRight(8);
            runAsAdminBtn_.setBounds(bannerRow.removeFromRight(165));
        }
        depStatusLabel_.setBounds(bannerRow);

        sInner.removeFromTop(6);
        installGuideEditor_.setBounds(sInner);
    }
}

} // namespace dash::ui
