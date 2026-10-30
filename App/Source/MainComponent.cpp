#include "MainComponent.h"

namespace dash::ui
{

MainComponent::MainComponent()
{
    setSize(900, 680);

    // Paleta oscura táctica de bajo consumo visual
    getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff11151c));
    getLookAndFeel().setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1b222d));
    getLookAndFeel().setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff141922));
    getLookAndFeel().setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2c3647));
    getLookAndFeel().setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f6feb));
    getLookAndFeel().setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    // Encabezado
    titleLabel_.setText("DASH COMPETITIVO AUDIO", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffe6edf3));
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setText("Orquestador Nativo C++20: IPolicyConfig COM + Inyector Atómico Equalizer APO",
                           juce::dontSendNotification);
    subtitleLabel_.setFont(juce::FontOptions(13.5f));
    subtitleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(subtitleLabel_);

    // Sección 1: Orquestador COM
    endpointGroup_.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff30363d));
    endpointGroup_.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff58a6ff));
    addAndMakeVisible(endpointGroup_);

    endpointCombo_.onChange = [this]() {
        syncConfigFromControls();
        updateConfigPreview();
    };
    addAndMakeVisible(endpointCombo_);

    refreshEndpointsBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    refreshEndpointsBtn_.onClick = [this]() { refreshEndpoints(); };
    addAndMakeVisible(refreshEndpointsBtn_);

    setDefaultEndpointBtn_.onClick = [this]() { applySelectedEndpoint(); };
    addAndMakeVisible(setDefaultEndpointBtn_);

    // Sección 2: Cadena Equalizer APO
    apoGroup_.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff30363d));
    apoGroup_.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff58a6ff));
    addAndMakeVisible(apoGroup_);

    auto configureToggle = [this](juce::ToggleButton& btn, bool initialState) {
        btn.setToggleState(initialState, juce::dontSendNotification);
        btn.onClick = [this]() {
            syncConfigFromControls();
            updateConfigPreview();
        };
        addAndMakeVisible(btn);
    };

    configureToggle(bindSelectedDeviceToggle_, false);
    configureToggle(transientToggle_, apoConfig_.enableTransientSuppressor);
    configureToggle(compressorToggle_, apoConfig_.enableLoudnessCompressor);
    configureToggle(ambienceToggle_, apoConfig_.enableAmbienceGuard);
    configureToggle(graphicEqToggle_, apoConfig_.enableGraphicEq);
    configureToggle(hesuviToggle_, apoConfig_.enableHeSuVi);

    addAndMakeVisible(configPathLabel_);

    configPathEditor_.setText(
        juce::String(dash::audio::ApoConfigInjector::getDefaultApoConfigPath().wstring()),
        juce::dontSendNotification);
    addAndMakeVisible(configPathEditor_);

    injectConfigBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff238636));
    injectConfigBtn_.onClick = [this]() { injectApoConfig(); };
    addAndMakeVisible(injectConfigBtn_);

    previewLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(previewLabel_);

    configPreviewEditor_.setMultiLine(true);
    configPreviewEditor_.setReadOnly(true);
    configPreviewEditor_.setScrollbarsShown(true);
    configPreviewEditor_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    addAndMakeVisible(configPreviewEditor_);

    statusLabel_.setFont(juce::FontOptions(13.0f));
    statusLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff0d1117));
    statusLabel_.setColour(juce::Label::outlineColourId, juce::Colour(0xff30363d));
    addAndMakeVisible(statusLabel_);

    refreshEndpoints();
    syncConfigFromControls();
    updateConfigPreview();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d1117));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(18);

    auto header = area.removeFromTop(54);
    titleLabel_.setBounds(header.removeFromTop(28));
    subtitleLabel_.setBounds(header);

    area.removeFromTop(8);

    // Bloque 1: Endpoints COM
    auto endpointBounds = area.removeFromTop(108);
    endpointGroup_.setBounds(endpointBounds);
    auto epInner = endpointBounds.reduced(16, 26);

    auto epTopRow = epInner.removeFromTop(28);
    refreshEndpointsBtn_.setBounds(epTopRow.removeFromRight(120));
    epTopRow.removeFromRight(8);
    endpointCombo_.setBounds(epTopRow);

    epInner.removeFromTop(8);
    setDefaultEndpointBtn_.setBounds(epInner.removeFromTop(30));

    area.removeFromTop(12);

    // Barra inferior de estado
    statusLabel_.setBounds(area.removeFromBottom(32));
    area.removeFromBottom(10);

    // Bloque 2: Configuración Equalizer APO
    apoGroup_.setBounds(area);
    auto apoInner = area.reduced(16, 26);

    auto toggleArea = apoInner.removeFromTop(90);
    auto leftCol = toggleArea.removeFromLeft(toggleArea.getWidth() / 2);
    auto rightCol = toggleArea;

    bindSelectedDeviceToggle_.setBounds(leftCol.removeFromTop(28));
    transientToggle_.setBounds(leftCol.removeFromTop(28));
    compressorToggle_.setBounds(leftCol.removeFromTop(28));

    ambienceToggle_.setBounds(rightCol.removeFromTop(28));
    graphicEqToggle_.setBounds(rightCol.removeFromTop(28));
    hesuviToggle_.setBounds(rightCol.removeFromTop(28));

    apoInner.removeFromTop(10);

    auto pathRow = apoInner.removeFromTop(30);
    configPathLabel_.setBounds(pathRow.removeFromLeft(110));
    injectConfigBtn_.setBounds(pathRow.removeFromRight(230));
    pathRow.removeFromRight(8);
    configPathEditor_.setBounds(pathRow);

    apoInner.removeFromTop(10);
    previewLabel_.setBounds(apoInner.removeFromTop(22));
    apoInner.removeFromTop(4);
    configPreviewEditor_.setBounds(apoInner);
}

void MainComponent::refreshEndpoints()
{
    std::string err;
    endpoints_ = policyManager_.enumerateActiveRenderEndpoints(&err);

    endpointCombo_.clear(juce::dontSendNotification);

    if (!err.empty())
    {
        logStatus("Error enumerando dispositivos WASAPI: " + juce::String(err), true);
        return;
    }

    int selectedIndex = 0;
    for (size_t i = 0; i < endpoints_.size(); ++i)
    {
        const auto& ep = endpoints_[i];
        juce::String label(ep.friendlyName);
        if (ep.isDefaultConsole || ep.isDefaultMultimedia)
            label << "  [Predeterminado]";

        endpointCombo_.addItem(label, static_cast<int>(i + 1));
        if (ep.isDefaultConsole)
            selectedIndex = static_cast<int>(i + 1);
    }

    if (selectedIndex == 0 && !endpoints_.empty())
        selectedIndex = 1;

    if (selectedIndex > 0)
        endpointCombo_.setSelectedId(selectedIndex, juce::dontSendNotification);

    logStatus("Endpoints activos detectados: " + juce::String(endpoints_.size()));
}

void MainComponent::applySelectedEndpoint()
{
    const int idx = endpointCombo_.getSelectedId() - 1;
    if (idx < 0 || static_cast<size_t>(idx) >= endpoints_.size())
    {
        logStatus("Selecciona un dispositivo de salida válido antes de conmutar.", true);
        return;
    }

    const auto& selected = endpoints_[static_cast<size_t>(idx)];
    std::string err;
    if (policyManager_.setDefaultEndpoint(selected.id, &err))
    {
        logStatus("Endpoint predeterminado actualizado (eConsole + eMultimedia): "
                  + juce::String(selected.friendlyName));
        refreshEndpoints();
    }
    else
    {
        logStatus("Fallo en IPolicyConfig::SetDefaultEndpoint: " + juce::String(err), true);
    }
}

void MainComponent::syncConfigFromControls()
{
    apoConfig_.enableTransientSuppressor = transientToggle_.getToggleState();
    apoConfig_.enableLoudnessCompressor  = compressorToggle_.getToggleState();
    apoConfig_.enableAmbienceGuard       = ambienceToggle_.getToggleState();
    apoConfig_.enableGraphicEq           = graphicEqToggle_.getToggleState();
    apoConfig_.enableHeSuVi              = hesuviToggle_.getToggleState();

    apoConfig_.targetEndpointGuid.clear();
    if (bindSelectedDeviceToggle_.getToggleState())
    {
        const int idx = endpointCombo_.getSelectedId() - 1;
        if (idx >= 0 && static_cast<size_t>(idx) < endpoints_.size())
        {
            apoConfig_.targetEndpointGuid =
                juce::String(endpoints_[static_cast<size_t>(idx)].endpointGuid).toStdString();
        }
    }
}

void MainComponent::updateConfigPreview()
{
    const std::string preview = dash::audio::ApoConfigInjector::buildConfigContent(apoConfig_);
    configPreviewEditor_.setText(juce::String::fromUTF8(preview.c_str()), false);
}

void MainComponent::injectApoConfig()
{
    syncConfigFromControls();

    const std::filesystem::path targetPath(configPathEditor_.getText().toWideCharPointer());
    std::string err;

    if (dash::audio::ApoConfigInjector::writeConfigAtomically(targetPath, apoConfig_, &err))
    {
        logStatus("Inyección atómica completada en: " + configPathEditor_.getText());
    }
    else
    {
        logStatus(juce::String::fromUTF8(err.c_str()), true);
    }
}

void MainComponent::logStatus(const juce::String& message, bool isError)
{
    statusLabel_.setColour(
        juce::Label::textColourId,
        isError ? juce::Colour(0xffff7b72) : juce::Colour(0xff3fb950));
    statusLabel_.setText("  " + message, juce::dontSendNotification);
}

} // namespace dash::ui
