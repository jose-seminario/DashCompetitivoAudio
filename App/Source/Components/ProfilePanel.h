#pragma once

#include <JuceHeader.h>
#include "../Profiles/ProfileRepository.h"
#include <functional>

namespace dash::ui
{

/**
 * Panel de Gestión y Catálogo de Perfiles (ProfilePanel).
 *
 * Permite explorar, cargar, guardar, importar y exportar perfiles JSON
 * utilizando el repositorio desacoplado dash::profile::ProfileRepository.
 */
class ProfilePanel final : public juce::Component
{
public:
    ProfilePanel();
    ~ProfilePanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshProfileList();

    std::function<void(const std::string& profileId)> onProfileSelected;
    std::function<void(const dash::profile::AudioProfile& profile)> onProfileSaved;

    void setLoadedProfile(const dash::profile::AudioProfile& profile);

private:
    void importProfileFile();
    void exportProfileFile();
    void saveCurrentProfile();
    void deleteSelectedProfile();

    dash::profile::ProfileRepository repo_;
    dash::profile::AudioProfile currentProfile_;

    juce::Label titleLabel_;
    juce::ListBox profileListBox_;
    juce::TextButton loadBtn_;
    juce::TextButton saveBtn_;
    juce::TextButton importBtn_;
    juce::TextButton exportBtn_;
    juce::TextButton deleteBtn_;

    // Campos de visualización y edición de metadatos
    juce::Label idLabel_;
    juce::TextEditor nameEditor_;
    juce::TextEditor descEditor_;
    juce::Label gameLabel_;
    juce::Label headphoneLabel_;
    juce::Label authorLabel_;
    juce::Label licenseLabel_;
    juce::Label verifiedBadge_;

    // Modelo para ListBox
    class ProfileListModel : public juce::ListBoxModel
    {
    public:
        std::vector<dash::profile::ProfileSummary> items;
        int getNumRows() override { return static_cast<int>(items.size()); }
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    };

    ProfileListModel listModel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfilePanel)
};

} // namespace dash::ui
