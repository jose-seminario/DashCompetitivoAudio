#include "ProfilePanel.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

void ProfilePanel::ProfileListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(items.size()))
        return;

    const auto& item = items[static_cast<size_t>(rowNumber)];

    if (rowIsSelected)
    {
        g.setColour(DashLookAndFeel::kBorderFocus);
        g.fillRoundedRectangle(2.0f, 1.0f, (float)width - 4.0f, (float)height - 2.0f, 4.0f);
    }

    g.setColour(rowIsSelected ? juce::Colours::white : DashLookAndFeel::kTextMain);
    g.setFont(juce::FontOptions(12.5f).withStyle(item.verified ? "Bold" : "Regular"));
    g.drawText(item.name, 12, 4, width - 100, 16, juce::Justification::centredLeft);

    g.setFont(juce::FontOptions(10.5f));
    g.setColour(rowIsSelected ? juce::Colours::white.withAlpha(0.8f) : DashLookAndFeel::kTextMuted);
    g.drawText(item.headphoneModel, 12, 22, width - 100, 14, juce::Justification::centredLeft);

    if (item.verified)
    {
        g.setColour(rowIsSelected ? juce::Colours::white : DashLookAndFeel::kSuccessEmerald);
        g.drawText("[VERIFICADO]", width - 90, 8, 80, 20, juce::Justification::centredRight);
    }
}

ProfilePanel::ProfilePanel()
{
    titleLabel_.setText("CATÁLOGO Y GESTIÓN DE PERFILES (DASHAUDIODB / LOCAL)", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    profileListBox_.setModel(&listModel_);
    profileListBox_.setRowHeight(42);
    profileListBox_.setColour(juce::ListBox::backgroundColourId, DashLookAndFeel::kBgCardElevated);
    profileListBox_.setColour(juce::ListBox::outlineColourId, DashLookAndFeel::kBorderSubtle);
    addAndMakeVisible(profileListBox_);

    loadBtn_.setButtonText("CARGAR SELECCIONADO");
    loadBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
    loadBtn_.onClick = [this] {
        const int selected = profileListBox_.getSelectedRow();
        if (selected >= 0 && selected < static_cast<int>(listModel_.items.size()))
        {
            const auto& item = listModel_.items[static_cast<size_t>(selected)];
            if (repo_.loadProfileById(item.profileId, currentProfile_))
            {
                setLoadedProfile(currentProfile_);
                if (onProfileSelected)
                    onProfileSelected(item.profileId);
            }
        }
    };
    addAndMakeVisible(loadBtn_);

    saveBtn_.setButtonText("GUARDAR CAMBIOS");
    saveBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kSuccessEmerald);
    saveBtn_.onClick = [this] { saveCurrentProfile(); };
    addAndMakeVisible(saveBtn_);

    importBtn_.setButtonText("IMPORTAR ARCHIVO...");
    importBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    importBtn_.onClick = [this] { importProfileFile(); };
    addAndMakeVisible(importBtn_);

    exportBtn_.setButtonText("EXPORTAR ARCHIVO...");
    exportBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    exportBtn_.onClick = [this] { exportProfileFile(); };
    addAndMakeVisible(exportBtn_);

    deleteBtn_.setButtonText("ELIMINAR");
    deleteBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kDangerCrimson.withAlpha(0.6f));
    deleteBtn_.onClick = [this] { deleteSelectedProfile(); };
    addAndMakeVisible(deleteBtn_);

    // Inicializar campos de metadatos
    idLabel_.setText("ID: warzone-competitive-default", juce::dontSendNotification);
    idLabel_.setFont(juce::FontOptions(11.5f));
    idLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(idLabel_);

    nameEditor_.setText("Warzone Competitive Default");
    nameEditor_.setColour(juce::TextEditor::backgroundColourId, DashLookAndFeel::kBgCardElevated);
    nameEditor_.setColour(juce::TextEditor::textColourId, DashLookAndFeel::kTextMain);
    nameEditor_.setColour(juce::TextEditor::outlineColourId, DashLookAndFeel::kBorderSubtle);
    addAndMakeVisible(nameEditor_);

    descEditor_.setMultiLine(true);
    descEditor_.setReturnKeyStartsNewLine(true);
    descEditor_.setText("Perfil balanceado de alta precisión.");
    descEditor_.setColour(juce::TextEditor::backgroundColourId, DashLookAndFeel::kBgCardElevated);
    descEditor_.setColour(juce::TextEditor::textColourId, DashLookAndFeel::kTextMain);
    descEditor_.setColour(juce::TextEditor::outlineColourId, DashLookAndFeel::kBorderSubtle);
    addAndMakeVisible(descEditor_);

    gameLabel_.setText("Juego: Call of Duty: Warzone", juce::dontSendNotification);
    gameLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(gameLabel_);

    headphoneLabel_.setText("Auricular: Generic Tactical Headset", juce::dontSendNotification);
    headphoneLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(headphoneLabel_);

    authorLabel_.setText("Autor: DashAudio Engineering", juce::dontSendNotification);
    authorLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(authorLabel_);

    licenseLabel_.setText("Licencia: CC-BY-4.0", juce::dontSendNotification);
    licenseLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(licenseLabel_);

    verifiedBadge_.setText("ESTADO: VERIFICADO", juce::dontSendNotification);
    verifiedBadge_.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
    verifiedBadge_.setColour(juce::Label::textColourId, DashLookAndFeel::kSuccessEmerald);
    addAndMakeVisible(verifiedBadge_);

    // Registrar rutas de búsqueda en el repositorio
    repo_.addSearchDirectory(std::filesystem::current_path() / "Profiles");
    repo_.addSearchDirectory(std::filesystem::current_path() / "DashAudioDB");

    refreshProfileList();
}

void ProfilePanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    // Tarjeta Izquierda (Lista)
    const auto leftCard = juce::Rectangle<float>(16, 44, 380, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(leftCard, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(leftCard, 8.0f, 1.2f);

    // Tarjeta Derecha (Metadatos y Edición)
    const auto rightCard = juce::Rectangle<float>(410, 44, (float)getWidth() - 426, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(rightCard, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(rightCard, 8.0f, 1.2f);
}

void ProfilePanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);

    // Columna Izquierda
    profileListBox_.setBounds(30, 56, 352, getHeight() - 170);
    loadBtn_.setBounds(30, getHeight() - 104, 170, 32);
    deleteBtn_.setBounds(212, getHeight() - 104, 170, 32);

    importBtn_.setBounds(30, getHeight() - 64, 170, 30);
    exportBtn_.setBounds(212, getHeight() - 64, 170, 30);

    // Columna Derecha
    const int rX = 428;
    const int rW = getWidth() - 450;

    idLabel_.setBounds(rX, 56, rW, 18);
    nameEditor_.setBounds(rX, 80, rW, 30);
    descEditor_.setBounds(rX, 118, rW, 70);

    gameLabel_.setBounds(rX, 198, rW, 20);
    headphoneLabel_.setBounds(rX, 222, rW, 20);
    authorLabel_.setBounds(rX, 246, rW, 20);
    licenseLabel_.setBounds(rX, 270, rW, 20);
    verifiedBadge_.setBounds(rX, 294, rW, 20);

    saveBtn_.setBounds(rX, getHeight() - 64, 200, 34);
}

void ProfilePanel::refreshProfileList()
{
    listModel_.items = repo_.scanProfiles();
    profileListBox_.updateContent();
    profileListBox_.repaint();
}

void ProfilePanel::setLoadedProfile(const dash::profile::AudioProfile& profile)
{
    currentProfile_ = profile;
    idLabel_.setText("ID: " + juce::String(profile.profileId), juce::dontSendNotification);
    nameEditor_.setText(juce::String(profile.name));
    descEditor_.setText(juce::String(profile.description));
    gameLabel_.setText("Juego: " + juce::String(profile.game.name), juce::dontSendNotification);
    headphoneLabel_.setText("Auricular: " + juce::String(profile.headphone.manufacturer + " " + profile.headphone.model), juce::dontSendNotification);
    authorLabel_.setText("Autor: " + juce::String(profile.metadata.author), juce::dontSendNotification);
    licenseLabel_.setText("Licencia: " + juce::String(profile.metadata.license), juce::dontSendNotification);

    verifiedBadge_.setText(profile.metadata.verified ? "ESTADO: VERIFICADO" : "ESTADO: COMUNITARIO / EXPERIMENTAL", juce::dontSendNotification);
    verifiedBadge_.setColour(juce::Label::textColourId,
                             profile.metadata.verified ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kWarningAmber);
}

void ProfilePanel::saveCurrentProfile()
{
    currentProfile_.name = nameEditor_.getText().toStdString();
    currentProfile_.description = descEditor_.getText().toStdString();

    std::string err;
    if (repo_.saveUserProfile(currentProfile_, &err))
    {
        refreshProfileList();
        if (onProfileSaved)
            onProfileSaved(currentProfile_);
    }
}

void ProfilePanel::importProfileFile()
{
    // Cargar archivo seguro
    refreshProfileList();
}

void ProfilePanel::exportProfileFile()
{
    // Exportar archivo seguro
}

void ProfilePanel::deleteSelectedProfile()
{
    const int selected = profileListBox_.getSelectedRow();
    if (selected >= 0 && selected < static_cast<int>(listModel_.items.size()))
    {
        const auto& item = listModel_.items[static_cast<size_t>(selected)];
        std::string err;
        repo_.deleteUserProfile(item.profileId, &err);
        refreshProfileList();
    }
}

} // namespace dash::ui
