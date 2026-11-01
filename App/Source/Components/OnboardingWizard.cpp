#include "OnboardingWizard.h"

namespace dash::ui
{

OnboardingWizard::OnboardingWizard()
{
    setOpaque(true);

    // 1. Encabezado de Bienvenida
    badgeLabel_.setText(juce::String::fromUTF8("ASISTENTE DE CONFIGURACI\xc3\x93N INICIAL"), juce::dontSendNotification);
    badgeLabel_.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    badgeLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(badgeLabel_);

    titleLabel_.setText(juce::String::fromUTF8("\xc2\xa1""BIENVENIDO A DASH COMPETITIVO AUDIO!"), juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setText(juce::String::fromUTF8("Para obtener audio t\xc3\xa1" "ctico de ultra baja latencia y pasos amplificados en Warzone,\n"
                                                 "necesitamos verificar que tu sistema cuenta con los componentes necesarios."),
                           juce::dontSendNotification);
    subtitleLabel_.setFont(juce::FontOptions(13.0f));
    subtitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(subtitleLabel_);

    // 2. Tarjeta y editor de componentes faltantes
    missingHeaderLabel_.setText(juce::String::fromUTF8("ESTADO DE COMPONENTES DEL SISTEMA"), juce::dontSendNotification);
    missingHeaderLabel_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    missingHeaderLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentBlue);
    addAndMakeVisible(missingHeaderLabel_);

    missingListEditor_.setMultiLine(true);
    missingListEditor_.setReadOnly(true);
    missingListEditor_.setColour(juce::TextEditor::backgroundColourId, DashLookAndFeel::kBgCardElevated);
    missingListEditor_.setColour(juce::TextEditor::textColourId, DashLookAndFeel::kTextMain);
    missingListEditor_.setColour(juce::TextEditor::outlineColourId, DashLookAndFeel::kBorderSubtle);
    missingListEditor_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(missingListEditor_);

    // 3. Advertencia de privilegios de Administrador
    adminWarningLabel_.setText(juce::String::fromUTF8("\xe2\x9a\xa0\xef\xb8\x8f Se requieren permisos de administrador para instalar los drivers de audio de Windows."),
                               juce::dontSendNotification);
    adminWarningLabel_.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    adminWarningLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kWarningAmber);
    addAndMakeVisible(adminWarningLabel_);

    // 4. Botón Principal: "Instalar Automáticamente"
    autoInstallBtn_.setButtonText(juce::String::fromUTF8("INSTALAR AUTOM\xc3\x81" "TICAMENTE"));
    autoInstallBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kSuccessEmerald);
    autoInstallBtn_.onClick = [this] {
        if (onAutoInstallRequested)
            onAutoInstallRequested();
    };
    addAndMakeVisible(autoInstallBtn_);

    // 5. Botón Secundario: "Descargar Manualmente (Abrir navegador)"
    manualDownloadBtn_.setButtonText(juce::String::fromUTF8("Descargar Manualmente (Abrir navegador)"));
    manualDownloadBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    manualDownloadBtn_.onClick = [this] {
        if (onManualDownloadRequested)
            onManualDownloadRequested();
    };
    addAndMakeVisible(manualDownloadBtn_);

    // 6. Botones de Soporte: Comprobar y Omitir
    recheckBtn_.setButtonText(juce::String::fromUTF8("Volver a Comprobar"));
    recheckBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
    recheckBtn_.onClick = [this] {
        if (onRecheckRequested)
            onRecheckRequested();
    };
    addAndMakeVisible(recheckBtn_);

    skipBtn_.setButtonText(juce::String::fromUTF8("Continuar a la Aplicaci\xc3\xb3n"));
    skipBtn_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    skipBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kTextMuted);
    skipBtn_.onClick = [this] {
        if (onDismissWizard)
            onDismissWizard();
    };
    addAndMakeVisible(skipBtn_);

    // 7. Etiqueta de progreso
    progressLabel_.setFont(juce::FontOptions(12.5f));
    progressLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    progressLabel_.setJustificationType(juce::Justification::centred);
    progressLabel_.setVisible(false);
    addAndMakeVisible(progressLabel_);
}

void OnboardingWizard::paint(juce::Graphics& g)
{
    // Fondo oscuro semitransparente o sólido
    g.fillAll(DashLookAndFeel::kBgDark);

    // Contenedor modal central
    const auto area = getLocalBounds().toFloat();
    const float cardW = std::min(760.0f, area.getWidth() - 48.0f);
    const float cardH = std::min(620.0f, area.getHeight() - 48.0f);
    const auto cardBounds = juce::Rectangle<float>(
        (area.getWidth() - cardW) * 0.5f,
        (area.getHeight() - cardH) * 0.5f,
        cardW, cardH
    );

    // Sombra suave y tarjeta principal
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(cardBounds, 12.0f);

    g.setColour(DashLookAndFeel::kBorderFocus);
    g.drawRoundedRectangle(cardBounds, 12.0f, 1.5f);
}

void OnboardingWizard::resized()
{
    const auto area = getLocalBounds();
    const int cardW = std::min(760, area.getWidth() - 48);
    const int cardH = std::min(620, area.getHeight() - 48);
    const int startX = (area.getWidth() - cardW) / 2;
    const int startY = (area.getHeight() - cardH) / 2;

    const int padX = 36;
    const int contentW = cardW - (padX * 2);

    int curY = startY + 28;

    badgeLabel_.setBounds(startX + padX, curY, contentW, 16);
    curY += 20;

    titleLabel_.setBounds(startX + padX, curY, contentW, 30);
    curY += 34;

    subtitleLabel_.setBounds(startX + padX, curY, contentW, 38);
    curY += 46;

    missingHeaderLabel_.setBounds(startX + padX, curY, contentW, 20);
    curY += 24;

    missingListEditor_.setBounds(startX + padX, curY, contentW, 140);
    curY += 150;

    adminWarningLabel_.setBounds(startX + padX, curY, contentW, 22);
    curY += 30;

    progressLabel_.setBounds(startX + padX, curY, contentW, 22);
    curY += 26;

    // Botón principal de instalación automática
    autoInstallBtn_.setBounds(startX + padX, curY, contentW, 42);
    curY += 50;

    // Botones secundarios
    const int halfW = (contentW - 12) / 2;
    manualDownloadBtn_.setBounds(startX + padX, curY, halfW, 34);
    recheckBtn_.setBounds(startX + padX + halfW + 12, curY, halfW, 34);
    curY += 42;

    skipBtn_.setBounds(startX + padX + (contentW - 220) / 2, curY, 220, 24);
}

void OnboardingWizard::updateStatus(const dash::audio::DependencyStatus& status)
{
    currentStatus_ = status;

    juce::String listText;
    int missingCount = 0;

    if (!status.hasEqualizerApo)
    {
        listText << juce::String::fromUTF8("\xe2\x80\xa2 [FALTA] Equalizer APO: No detectado\n"
                                           "  Motor de procesamiento de efectos de audio para Windows (requerido para plugins t\xc3\xa1" "cticos).\n\n");
        missingCount++;
    }
    else
    {
        listText << juce::String::fromUTF8("\xe2\x80\xa2 [OK] Equalizer APO: Instalado y detectado correctamente.\n\n");
    }

    if (!status.hasVbAudioCable)
    {
        listText << juce::String::fromUTF8("\xe2\x80\xa2 [FALTA] VB-Cable: No detectado\n"
                                           "  Driver de cable de audio virtual para enrutar sonido multicanal con latencia cero.\n\n");
        missingCount++;
    }
    else
    {
        listText << juce::String::fromUTF8("\xe2\x80\xa2 [OK] VB-Cable: Instalado (")
                 << juce::String(status.detectedCableName.c_str())
                 << juce::String::fromUTF8(").\n\n");
    }

    if (!status.hasHeSuVi)
    {
        listText << juce::String::fromUTF8("\xe2\x80\xa2 [OPCIONAL] HeSuVi 7.1 Virtualization: No detectado\n"
                                           "  (Opcional: Se incluye soporte binaural anal\xc3\xad" "tico 3D nativo en la app).\n");
    }
    else
    {
        listText << juce::String::fromUTF8("\xe2\x80\xa2 [OK] HeSuVi 7.1 Virtualization: Presente.\n");
    }

    missingListEditor_.setText(listText);

    if (missingCount == 0)
    {
        autoInstallBtn_.setButtonText(juce::String::fromUTF8("\xc2\xa1""TODO LISTO! ENTRAR A LA APLICACI\xc3\x93N"));
        autoInstallBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
        autoInstallBtn_.onClick = [this] {
            if (onDismissWizard)
                onDismissWizard();
        };
        adminWarningLabel_.setText(juce::String::fromUTF8("\xe2\x9c\x93 Todas las dependencias cr\xc3\xad" "ticas est\xc3\xa1" "n instaladas en tu equipo."), juce::dontSendNotification);
        adminWarningLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kSuccessEmerald);
    }
    else
    {
        autoInstallBtn_.setButtonText(juce::String::fromUTF8("INSTALAR AUTOM\xc3\x81" "TICAMENTE"));
        autoInstallBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kSuccessEmerald);
        autoInstallBtn_.onClick = [this] {
            if (onAutoInstallRequested)
                onAutoInstallRequested();
        };
        adminWarningLabel_.setText(juce::String::fromUTF8("\xe2\x9a\xa0\xef\xb8\x8f Se requieren permisos de administrador para instalar los drivers de audio de Windows."), juce::dontSendNotification);
        adminWarningLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kWarningAmber);
    }
}

void OnboardingWizard::setInstallingState(bool isInstalling, const juce::String& statusMessage)
{
    isInstalling_ = isInstalling;

    autoInstallBtn_.setEnabled(!isInstalling);
    manualDownloadBtn_.setEnabled(!isInstalling);
    recheckBtn_.setEnabled(!isInstalling);
    skipBtn_.setEnabled(!isInstalling);

    if (isInstalling)
    {
        autoInstallBtn_.setButtonText(juce::String::fromUTF8("INSTALANDO COMPONENTES..."));
        progressLabel_.setText(statusMessage.isNotEmpty() ? statusMessage : juce::String::fromUTF8("Descargando e iniciando instaladores oficiales..."), juce::dontSendNotification);
        progressLabel_.setVisible(true);
    }
    else
    {
        progressLabel_.setVisible(statusMessage.isNotEmpty());
        if (statusMessage.isNotEmpty())
            progressLabel_.setText(statusMessage, juce::dontSendNotification);
    }
}

} // namespace dash::ui
