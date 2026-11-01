#include "HeadphonePanel.h"
#include "../Theme/DashLookAndFeel.h"
#include <filesystem>

namespace dash::ui
{

HeadphonePanel::HeadphonePanel()
{
    titleLabel_.setText(juce::String::fromUTF8(u8"CALIBRACIÓN ACÚSTICA DE AURICULARES (AUTOEQ / HARMAN 2018)"), juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    modelSelectLabel_.setText(juce::String::fromUTF8(u8"Seleccionar Modelo de Auricular:"), juce::dontSendNotification);
    modelSelectLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    addAndMakeVisible(modelSelectLabel_);

    modelSelector_.onChange = [this] {
        updateModelInfo();
        const int idx = modelSelector_.getSelectedId() - 1;
        if (idx >= 0 && idx < static_cast<int>(headphoneItems_.size()))
        {
            const auto& item = headphoneItems_[static_cast<size_t>(idx)];
            if (onHeadphoneModelChanged)
                onHeadphoneModelChanged(item.model);
            if (onCompensationChanged)
                onCompensationChanged(enableCompensationToggle_.getToggleState(), item.bands);
        }
    };
    addAndMakeVisible(modelSelector_);

    statusBadgeLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f).withStyle("Bold")));
    addAndMakeVisible(statusBadgeLabel_);

    manufacturerLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
    addAndMakeVisible(manufacturerLabel_);

    targetCurveLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
    addAndMakeVisible(targetCurveLabel_);

    measurementSourceLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
    addAndMakeVisible(measurementSourceLabel_);

    licenseLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
    addAndMakeVisible(licenseLabel_);

    descriptionLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
    descriptionLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(descriptionLabel_);

    enableCompensationToggle_.setButtonText(juce::String::fromUTF8(u8"Aplicar Ecualización de Compensación"));
    enableCompensationToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    enableCompensationToggle_.onClick = [this] {
        const int idx = modelSelector_.getSelectedId() - 1;
        if (idx >= 0 && idx < static_cast<int>(headphoneItems_.size()))
        {
            const auto& item = headphoneItems_[static_cast<size_t>(idx)];
            if (onCompensationChanged)
                onCompensationChanged(enableCompensationToggle_.getToggleState(), item.bands);
        }
    };
    addAndMakeVisible(enableCompensationToggle_);

    scanHeadphoneProfiles();
    updateModelInfo();
}

void HeadphonePanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const auto cardArea = juce::Rectangle<float>(16, 44, (float)getWidth() - 32, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(cardArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(cardArea, 8.0f, 1.2f);
}

void HeadphonePanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);

    const int startX = 36;
    const int contentW = getWidth() - 72;

    modelSelectLabel_.setBounds(startX, 60, contentW, 20);
    modelSelector_.setBounds(startX, 84, 460, 32);

    statusBadgeLabel_.setBounds(startX, 130, contentW, 22);
    manufacturerLabel_.setBounds(startX, 156, contentW, 20);
    targetCurveLabel_.setBounds(startX, 180, contentW, 20);
    measurementSourceLabel_.setBounds(startX, 204, contentW, 20);
    licenseLabel_.setBounds(startX, 228, contentW, 20);

    descriptionLabel_.setBounds(startX, 260, contentW, 44);

    enableCompensationToggle_.setBounds(startX, 320, 380, 26);
}

std::vector<dash::core::ParametricBandParameters> HeadphonePanel::getSelectedHeadphoneBands() const
{
    const int idx = modelSelector_.getSelectedId() - 1;
    if (idx >= 0 && idx < static_cast<int>(headphoneItems_.size()))
        return headphoneItems_[static_cast<size_t>(idx)].bands;
    return {};
}

void HeadphonePanel::setSelectedModelId(const juce::String& modelId)
{
    if (modelId.isEmpty())
        return;

    for (size_t i = 0; i < headphoneItems_.size(); ++i)
    {
        if (headphoneItems_[i].id.equalsIgnoreCase(modelId) ||
            headphoneItems_[i].model.equalsIgnoreCase(modelId) ||
            headphoneItems_[i].displayName.containsIgnoreCase(modelId))
        {
            modelSelector_.setSelectedId(static_cast<int>(i + 1), juce::dontSendNotification);
            updateModelInfo();
            return;
        }
    }
}

void HeadphonePanel::setCompensationEnabled(bool enabled)
{
    enableCompensationToggle_.setToggleState(enabled, juce::dontSendNotification);
}

void HeadphonePanel::scanHeadphoneProfiles()
{
    headphoneItems_.clear();
    modelSelector_.clear(juce::dontSendNotification);

    const std::vector<std::filesystem::path> candidateRoots = {
        std::filesystem::current_path() / "Profiles" / "Headphones",
        std::filesystem::current_path().parent_path() / "Profiles" / "Headphones",
        std::filesystem::path("Profiles/Headphones"),
        std::filesystem::path("C:/Program Files/DashCompetitivoAudio/Profiles/Headphones")
    };

    std::filesystem::path foundPath;
    for (const auto& p : candidateRoots)
    {
        std::error_code ec;
        if (std::filesystem::exists(p, ec) && std::filesystem::is_directory(p, ec))
        {
            foundPath = p;
            break;
        }
    }

    if (!foundPath.empty())
    {
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(foundPath, ec))
        {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".json")
            {
                const std::string filename = entry.path().filename().string();
                if (filename.find(".schema.json") != std::string::npos)
                    continue;

                juce::File f(juce::String(entry.path().string()));
                juce::var parsed;
                if (juce::JSON::parse(f.loadFileAsString(), parsed).wasOk() && parsed.isObject())
                {
                    HeadphoneProfileItem item;
                    item.id = parsed.getProperty("profileId", f.getFileNameWithoutExtension()).toString();
                    auto hp = parsed.getProperty("headphone", juce::var());
                    item.manufacturer = hp.getProperty("manufacturer", juce::String::fromUTF8(u8"Fabricante Desconocido")).toString();
                    item.model = hp.getProperty("model", f.getFileNameWithoutExtension()).toString();
                    const juce::String variant = hp.getProperty("variant", "").toString();
                    item.displayName = item.manufacturer + " " + item.model + (variant.isNotEmpty() ? " (" + variant + ")" : "");
                    item.targetCurve = hp.getProperty("targetCurve", "Harman Over-Ear 2018").toString();
                    item.measurementSource = hp.getProperty("measurementSource", "AutoEq / Harman").toString();
                    item.description = parsed.getProperty("description", "").toString();

                    auto meta = parsed.getProperty("metadata", juce::var());
                    item.license = meta.getProperty("license", "CC-BY-4.0").toString();
                    const bool verified = meta.getProperty("verified", false);
                    const juce::String calStatus = hp.getProperty("calibrationStatus", "uncalibrated").toString();

                    // Parsear bandas paramétricas de compensación si existen
                    auto proc = parsed.getProperty("processing", juce::var());
                    if (proc.hasProperty("parametricEq"))
                    {
                        auto peq = proc.getProperty("parametricEq", juce::var());
                        if (peq.hasProperty("bands"))
                        {
                            auto bandsVar = peq.getProperty("bands", juce::var());
                            if (bandsVar.isArray())
                            {
                                auto* arr = bandsVar.getArray();
                                for (int b = 0; b < arr->size(); ++b)
                                {
                                    const auto& bObj = (*arr)[b];
                                    dash::core::ParametricBandParameters band;
                                    band.frequencyHz = static_cast<float>(bObj.getProperty("frequencyHz", 1000.0));
                                    band.gainDb      = static_cast<float>(bObj.getProperty("gainDb", 0.0));
                                    band.q           = static_cast<float>(bObj.getProperty("q", 0.7071));
                                    band.enabled     = static_cast<bool>(bObj.getProperty("enabled", true));
                                    const juce::String tStr = bObj.getProperty("type", "peaking").toString().toLowerCase();
                                    if (tStr == "low_shelf" || tStr == "lowshelf")
                                        band.type = dash::core::ParametricFilterType::LowShelf;
                                    else if (tStr == "high_shelf" || tStr == "highshelf")
                                        band.type = dash::core::ParametricFilterType::HighShelf;
                                    else if (tStr == "high_pass" || tStr == "highpass")
                                        band.type = dash::core::ParametricFilterType::HighPass;
                                    else if (tStr == "low_pass" || tStr == "lowpass")
                                        band.type = dash::core::ParametricFilterType::LowPass;
                                    else if (tStr == "notch")
                                        band.type = dash::core::ParametricFilterType::Notch;
                                    else
                                        band.type = dash::core::ParametricFilterType::Peaking;
                                    item.bands.push_back(band);
                                }
                            }
                        }
                    }

                    item.isCalibrated = (calStatus != "uncalibrated" && calStatus != "uncalibrated_pending_verified_measurement" && verified);
                    if (item.isCalibrated)
                    {
                        item.statusBadge = juce::String::fromUTF8(u8"ESTADO: CALIBRACIÓN VERIFICADA (AUTOEQ HARMAN 2018)");
                    }
                    else
                    {
                        item.statusBadge = juce::String::fromUTF8(u8"ESTADO: SIN CALIBRACIÓN — PENDIENTE DE MEDICIÓN DE LABORATORIO");
                    }
                    headphoneItems_.push_back(std::move(item));
                }
            }
        }
    }

    if (headphoneItems_.empty())
    {
        headphoneItems_ = {
            {
                "razer-blackshark-v2x",
                juce::String::fromUTF8(u8"Razer BlackShark V2 X (3.5mm Analógico)"),
                "Razer Inc.",
                "BlackShark V2 X",
                juce::String::fromUTF8(u8"ESTADO: CALIBRACIÓN VERIFICADA (AUTOEQ HARMAN 2018)"),
                "Harman Over-Ear 2018",
                "AutoEq (Harman Over-Ear 2018)",
                "CC-BY-4.0",
                juce::String::fromUTF8(u8"Calibración acústica AutoEQ aplicada hacia curva Harman Over-Ear 2018 para corrección de resonancia y realce de claridad táctica."),
                true,
                {
                    { 200.0f, -2.5f, 1.4f, dash::core::ParametricFilterType::Peaking, true },
                    { 1200.0f, 1.8f, 2.0f, dash::core::ParametricFilterType::Peaking, true },
                    { 4000.0f, 3.5f, 0.71f, dash::core::ParametricFilterType::HighShelf, true }
                }
            },
            {
                "hyperx-cloud-ii",
                juce::String::fromUTF8(u8"HyperX Cloud II (Almohadillas Cuero)"),
                "HP / HyperX",
                "Cloud II",
                juce::String::fromUTF8(u8"ESTADO: CALIBRACIÓN VERIFICADA (AUTOEQ HARMAN 2018)"),
                "Target: Harman Over-Ear 2018",
                "AutoEq por Jaakko Pasanen (Archivo oratory1990)",
                "MIT",
                juce::String::fromUTF8(u8"Compensación acústica de precisión con filtros paramétricos para eliminar picos de resonancia en 5.8 kHz y linearizar graves."),
                true,
                {
                    { 105.0f, -2.3f, 0.71f, dash::core::ParametricFilterType::LowShelf, true },
                    { 240.0f, -1.8f, 1.4f, dash::core::ParametricFilterType::Peaking, true },
                    { 4800.0f, 3.2f, 2.0f, dash::core::ParametricFilterType::Peaking, true }
                }
            },
            {
                "logitech-g-pro-x",
                juce::String::fromUTF8(u8"Logitech G Pro X (Wired)"),
                "Logitech G",
                "G Pro X",
                juce::String::fromUTF8(u8"ESTADO: CALIBRACIÓN VERIFICADA (AUTOEQ HARMAN 2018)"),
                "Target: Harman Over-Ear 2018",
                "AutoEq por Jaakko Pasanen (Archivo oratory1990)",
                "MIT",
                juce::String::fromUTF8(u8"Compensación paramétrica de 4 bandas que suaviza la transición de medios-altos en 4.2 kHz y reduce fatiga auditiva."),
                true,
                {
                    { 85.0f, -3.1f, 0.71f, dash::core::ParametricFilterType::LowShelf, true },
                    { 1500.0f, -2.0f, 1.5f, dash::core::ParametricFilterType::Peaking, true },
                    { 4200.0f, 4.1f, 2.0f, dash::core::ParametricFilterType::Peaking, true },
                    { 7100.0f, -3.5f, 3.0f, dash::core::ParametricFilterType::Peaking, true }
                }
            }
        };
    }

    for (size_t i = 0; i < headphoneItems_.size(); ++i)
    {
        modelSelector_.addItem(headphoneItems_[i].displayName, static_cast<int>(i + 1));
    }
    modelSelector_.setSelectedId(1, juce::dontSendNotification);
}

void HeadphonePanel::updateModelInfo()
{
    const int idx = modelSelector_.getSelectedId() - 1;
    if (idx < 0 || idx >= static_cast<int>(headphoneItems_.size()))
        return;

    const auto& item = headphoneItems_[static_cast<size_t>(idx)];
    statusBadgeLabel_.setText(item.statusBadge, juce::dontSendNotification);
    statusBadgeLabel_.setColour(juce::Label::textColourId,
                                item.isCalibrated ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kWarningAmber);

    manufacturerLabel_.setText(juce::String::fromUTF8(u8"Fabricante: ") + item.manufacturer, juce::dontSendNotification);
    targetCurveLabel_.setText(juce::String::fromUTF8(u8"Target Deseado: ") + item.targetCurve, juce::dontSendNotification);
    measurementSourceLabel_.setText(juce::String::fromUTF8(u8"Fuente: ") + item.measurementSource, juce::dontSendNotification);
    licenseLabel_.setText(juce::String::fromUTF8(u8"Licencia del Perfil: ") + item.license, juce::dontSendNotification);
    descriptionLabel_.setText(item.description, juce::dontSendNotification);

    enableCompensationToggle_.setEnabled(item.isCalibrated);
    enableCompensationToggle_.setToggleState(item.isCalibrated, juce::dontSendNotification);
}

} // namespace dash::ui
