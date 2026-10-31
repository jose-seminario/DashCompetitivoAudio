#include "MainComponent.h"

#include <cmath>

namespace dash::ui
{

MainComponent::MainComponent()
    : tabs_(juce::TabbedButtonBar::TabsAtTop)
{
    setLookAndFeel(&dashLookAndFeel_);
    setSize(1080, 800);

    // 1. Encabezado Superior
    headerTitleLabel_.setText("DASH COMPETITIVO AUDIO", juce::dontSendNotification);
    headerTitleLabel_.setFont(juce::FontOptions(17.0f).withStyle("Bold"));
    headerTitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(headerTitleLabel_);

    headerSubtitleLabel_.setText("Tactical Audio DSP Engine & Spatial 3D Platform", juce::dontSendNotification);
    headerSubtitleLabel_.setFont(juce::FontOptions(11.5f));
    headerSubtitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(headerSubtitleLabel_);

    // 2. Creación e inicialización de paneles modulares
    dashboardPanel_     = std::make_unique<DashboardComponent>();
    profilePanel_       = std::make_unique<ProfilePanel>();
    audioEnginePanel_   = std::make_unique<AudioEnginePanel>();
    dynamicsPanel_      = std::make_unique<DynamicsPanel>();
    tacticalPanel_      = std::make_unique<TacticalProcessingPanel>();
    equalizerPanel_     = std::make_unique<EqualizerPanel>();
    spatialPanel_       = std::make_unique<SpatialAudioPanel>();
    headphonePanel_     = std::make_unique<HeadphonePanel>();
    benchmarkPanel_     = std::make_unique<BenchmarkPanel>();
    systemStatusPanel_  = std::make_unique<SystemStatusPanel>();

    // 3. Configuración de pestañas de navegación
    tabs_.setTabBarDepth(34);
    tabs_.setColour(juce::TabbedComponent::backgroundColourId, DashLookAndFeel::kBgDark);
    tabs_.setColour(juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);

    const auto tabBg = DashLookAndFeel::kBgCardElevated;
    tabs_.addTab("DASHBOARD", tabBg, dashboardPanel_.get(), false);
    tabs_.addTab("PERFILES", tabBg, profilePanel_.get(), false);
    tabs_.addTab("MOTOR & WASAPI", tabBg, audioEnginePanel_.get(), false);
    tabs_.addTab("DINÁMICA BS.1770", tabBg, dynamicsPanel_.get(), false);
    tabs_.addTab("DISPAROS & RUIDO", tabBg, tacticalPanel_.get(), false);
    tabs_.addTab("ECUALIZADOR", tabBg, equalizerPanel_.get(), false);
    tabs_.addTab("AUDIO ESPACIAL 3D", tabBg, spatialPanel_.get(), false);
    tabs_.addTab("AURICULARES", tabBg, headphonePanel_.get(), false);
    tabs_.addTab("BENCHMARK", tabBg, benchmarkPanel_.get(), false);
    tabs_.addTab("SISTEMA", tabBg, systemStatusPanel_.get(), false);
    addAndMakeVisible(tabs_);

    // 4. Barra de estado inferior
    addAndMakeVisible(statusBar_);

    // 5. Inicialización del motor DSP y Backend de audio WASAPI
    audioGraph_ = std::make_unique<dash::core::AudioGraph>();
    audioBackend_ = std::make_unique<dash::audio::JuceWasapiAudioBackend>();
    std::string initBackendErr;
    audioBackend_->initialize(&initBackendErr);

    // 6. Cableado de callbacks entre componentes
    dashboardPanel_->onActivateClicked = [this] { activateTacticalMode(); };
    dashboardPanel_->onDeactivateClicked = [this] { deactivateAndRestoreAudio(); };
    dashboardPanel_->onQuickProfileSelected = [this](const juce::String& name) {
        if (name.containsIgnoreCase("Balanced"))
            loadProfile("warzone-balanced-default");
        else if (name.containsIgnoreCase("Immersive"))
            loadProfile("warzone-immersive-default");
        else if (name.containsIgnoreCase("Footsteps"))
            loadProfile("warzone-footsteps-focus");
        else
            loadProfile("warzone-competitive-default");
    };

    dashboardPanel_->onQuickControlsChanged = [this](float gun, float stepsDb, float ambGuard, bool spatialEnabled) {
        currentProfile_.dspParams.transient.gun = juce::jlimit(0.0f, 1.0f, gun);
        currentProfile_.dspParams.equalizer.footstepsDb = stepsDb;
        currentProfile_.dspParams.ambience.ambGuar = juce::jlimit(0.0f, 1.0f, ambGuard);
        currentProfile_.enableSpatial = spatialEnabled;
        scheduleDebouncedInjection();
    };

    audioEnginePanel_->onFormatConfigChanged = [this](double sampleRate, int /*bufferSize*/, int numChannels) {
        currentProfile_.audio.sampleRate = sampleRate;
        juce::String layoutStr = juce::String(numChannels) + " canales";
        if (numChannels == 2) layoutStr = "2.0 Estéreo (2 canales)";
        else if (numChannels == 6) layoutStr = "5.1 Surround (6 canales)";
        else if (numChannels == 8) layoutStr = "7.1 Surround (8 canales)";
        else if (numChannels == 12) layoutStr = "7.1.4 Inmersivo (12 canales)";
        else if (numChannels == 16) layoutStr = "9.1.6 Espacial (16 canales)";
        dashboardPanel_->setFormatInfo(sampleRate, layoutStr);
        scheduleDebouncedInjection();
    };

    profilePanel_->onProfileSelected = [this](const std::string& profileId) {
        loadProfile(profileId);
    };

    profilePanel_->onProfileSaved = [this](const dash::profile::AudioProfile& p) {
        currentProfile_ = p;
        scheduleDebouncedInjection();
        statusBar_.setStatusMessage("Perfil '" + juce::String(p.name) + "' guardado correctamente.");
    };

    dynamicsPanel_->onParametersChanged = [this](const dash::core::DynamicsParameters& p) {
        currentProfile_.dspParams.dynamics = p;
        scheduleDebouncedInjection();
    };

    tacticalPanel_->onParametersChanged = [this](
        const dash::core::TransientParameters& t,
        const dash::core::AmbienceParameters& a,
        const dash::core::LimiterParameters& l) {
        currentProfile_.dspParams.transient = t;
        currentProfile_.dspParams.ambience  = a;
        currentProfile_.dspParams.limiter   = l;
        scheduleDebouncedInjection();
    };

    equalizerPanel_->onParametersChanged = [this](
        const dash::core::EQParameters& eq,
        bool useP,
        const std::array<dash::core::ParametricBandParameters, dash::core::ParametricEQProcessor::kMaxBands>& bands) {
        currentProfile_.dspParams.equalizer = eq;
        currentProfile_.dspParams.useParametricEq = useP;
        currentProfile_.dspParams.parametricBands = bands;
        scheduleDebouncedInjection();
    };

    spatialPanel_->onParametersChanged = [this](const dash::spatial::SpatialProcessorParameters& s) {
        currentProfile_.spatialParams = s;
        scheduleDebouncedInjection();
    };

    // 6. Cargar perfil inicial por defecto
    loadProfile("warzone-competitive-default");

    // Iniciar temporizador de telemetría a 30 Hz
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (isProfileActive_)
    {
        deactivateAndRestoreAudio();
    }
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawHorizontalLine(44, 0.0f, (float)getWidth());
}

void MainComponent::resized()
{
    headerTitleLabel_.setBounds(20, 6, 320, 20);
    headerSubtitleLabel_.setBounds(20, 24, 380, 16);

    tabs_.setBounds(0, 44, getWidth(), getHeight() - 76);
    statusBar_.setBounds(0, getHeight() - 32, getWidth(), 32);
}

void MainComponent::timerCallback()
{
    // Manejo de inyección aplazada (debounced)
    if (needsInjection_)
    {
        if (++debounceCounter_ >= 3) // ~100 ms a 30 FPS
        {
            needsInjection_ = false;
            debounceCounter_ = 0;
            executeConfigInjection();
        }
    }

    // Telemetría en tiempo real del motor DSP y Backend WASAPI
    if (isProfileActive_ && audioBackend_ && audioBackend_->getState() == dash::audio::AudioBackendState::Running)
    {
        const float latMs = audioBackend_->getCurrentLatencyMs();
        const float cpu = audioBackend_->getCpuUsage();
        dashboardPanel_->setLatencyMs(latMs > 0.0f ? latMs : 5.3f);
        dashboardPanel_->setCpuPercent(cpu);

        // Actualizar estados de bypass de los nodos en el Dashboard
        // Nodos: 0: Input, 1: Headroom, 2: Dynamics, 3: Transient, 4: Ambience, 5: Spatial, 6: EQ, 7: Limiter, 8: Output
        dashboardPanel_->setNodeBypassState(1, currentProfile_.dspParams.bypassAll);
        dashboardPanel_->setNodeBypassState(2, currentProfile_.dspParams.bypassAll || currentProfile_.dspParams.dynamics.bypass);
        dashboardPanel_->setNodeBypassState(3, currentProfile_.dspParams.bypassAll || currentProfile_.dspParams.transient.bypass);
        dashboardPanel_->setNodeBypassState(4, currentProfile_.dspParams.bypassAll || currentProfile_.dspParams.ambience.bypass);
        dashboardPanel_->setNodeBypassState(5, currentProfile_.dspParams.bypassAll || !currentProfile_.enableSpatial || currentProfile_.spatialParams.bypass);
        dashboardPanel_->setNodeBypassState(6, currentProfile_.dspParams.bypassAll || currentProfile_.dspParams.equalizer.bypass);
        dashboardPanel_->setNodeBypassState(7, currentProfile_.dspParams.bypassAll || currentProfile_.dspParams.limiter.bypass);
    }
}

void MainComponent::loadProfile(const std::string& profileId)
{
    std::string err;
    if (profileRepo_.loadProfileById(profileId, currentProfile_, &err))
    {
        syncAllPanelsWithProfile();
        statusBar_.setActiveProfileName(juce::String(currentProfile_.name));
        statusBar_.setStatusMessage("Perfil '" + juce::String(currentProfile_.name) + "' cargado.");
        scheduleDebouncedInjection();
    }
    else
    {
        // Si no se encuentra en disco, iniciar nominal
        currentProfile_.resetToDefaults();
        syncAllPanelsWithProfile();
    }
}

void MainComponent::syncAllPanelsWithProfile()
{
    dashboardPanel_->updateQuickControls(
        currentProfile_.dspParams.transient.gun,
        currentProfile_.dspParams.equalizer.footstepsDb,
        currentProfile_.dspParams.ambience.ambGuar,
        currentProfile_.enableSpatial
    );

    profilePanel_->setLoadedProfile(currentProfile_);
    dynamicsPanel_->setParameters(currentProfile_.dspParams.dynamics);
    tacticalPanel_->setParameters(
        currentProfile_.dspParams.transient,
        currentProfile_.dspParams.ambience,
        currentProfile_.dspParams.limiter
    );
    equalizerPanel_->setParameters(
        currentProfile_.dspParams.equalizer,
        currentProfile_.dspParams.useParametricEq,
        currentProfile_.dspParams.parametricBands
    );
    spatialPanel_->setParameters(currentProfile_.spatialParams);

    currentProfile_.dspParams.spatial = currentProfile_.spatialParams;
    currentProfile_.dspParams.spatial.bypass = !currentProfile_.enableSpatial || currentProfile_.spatialParams.bypass;

    if (audioGraph_)
    {
        audioGraph_->updateParameters(currentProfile_.dspParams);
    }
}

void MainComponent::scheduleDebouncedInjection()
{
    needsInjection_ = true;
    debounceCounter_ = 0;
}

void MainComponent::executeConfigInjection()
{
    currentProfile_.dspParams.spatial = currentProfile_.spatialParams;
    currentProfile_.dspParams.spatial.bypass = !currentProfile_.enableSpatial || currentProfile_.spatialParams.bypass;

    if (audioGraph_)
    {
        audioGraph_->updateParameters(currentProfile_.dspParams);
    }

    if (!isProfileActive_)
        return;

    dash::audio::ApoChainConfig config;
    config.profileName = currentProfile_.name;
    config.preampDb    = static_cast<double>(currentProfile_.dspParams.headroomDb);

    // 1. Supresor de Transitorios
    config.enableTransientSuppressor = !currentProfile_.dspParams.bypassAll && !currentProfile_.dspParams.transient.bypass;
    config.gunVolume    = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.transient.gun));
    config.gunSteps     = static_cast<int>(std::round(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.transient.steps)));
    config.gunFrontOnly = currentProfile_.dspParams.transient.front ? 1 : 0;
    config.gunRestore   = static_cast<int>(std::round(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.transient.restore)));
    config.gunBypass    = currentProfile_.dspParams.transient.bypass ? 1 : 0;

    // 2. Compresor Dinámico Híbrido BS.1770
    config.enableLoudnessCompressor = !currentProfile_.dspParams.bypassAll && !currentProfile_.dspParams.dynamics.bypass;
    config.compAmount  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.dynamics.amount));
    config.compTarget  = juce::jlimit(0.0, 1.0, (static_cast<double>(currentProfile_.dspParams.dynamics.targetLufs) + 28.0) / 36.0);
    config.compRelease = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.dynamics.releaseNorm));
    config.compDetRel  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.dynamics.detRelNorm));
    config.compLimRel  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.dynamics.limRelNorm));
    config.compScLow   = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.dynamics.scLowNorm));
    config.compScHigh  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.dynamics.scHighNorm));

    // 3. Protector Ambiental Multibanda
    config.enableAmbienceGuard = !currentProfile_.dspParams.bypassAll && !currentProfile_.dspParams.ambience.bypass;
    config.ambGuard   = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.ambience.ambGuar));
    config.ambDistant = currentProfile_.dspParams.ambience.distant ? 1 : 0;
    config.ambTarget  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.ambience.target));
    config.ambRelease = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.ambience.release));
    config.ambDetRel  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.ambience.detRel));
    config.ambScLow   = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.ambience.scLow));
    config.ambScHigh  = static_cast<double>(juce::jlimit(0.0f, 1.0f, currentProfile_.dspParams.ambience.scHigh));
    config.ambBandMod = currentProfile_.dspParams.ambience.bandMod ? 1 : 0;

    // 4. Ecualizador Gráfico de 31 Bandas y HeSuVi
    config.enableGraphicEq = !currentProfile_.dspParams.bypassAll && !currentProfile_.dspParams.equalizer.bypass;
    config.footstepsEqDb   = static_cast<double>(currentProfile_.dspParams.equalizer.footstepsDb);
    config.bassEqDb        = static_cast<double>(currentProfile_.dspParams.equalizer.bassDb);
    config.enableHeSuVi    = !currentProfile_.dspParams.bypassAll && currentProfile_.enableSpatial && !currentProfile_.spatialParams.bypass;

    std::string err;
    const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
    const std::string directives = dash::audio::ApoConfigInjector::buildConfigContent(config, apoConfigPath);
    if (dash::audio::ApoConfigManager::injectManagedSection(apoConfigPath, directives, &err) == dash::audio::ApoConfigResult::Success)
    {
        statusBar_.setStatusMessage("Configuración Equalizer APO sincronizada.");
    }
    else
    {
        statusBar_.setStatusMessage("Aviso APO: " + juce::String(err), true);
    }
}

void MainComponent::activateTacticalMode()
{
    isProfileActive_ = true;
    dashboardPanel_->setEngineRunning(true);
    statusBar_.setEngineStatus("MOTOR: ACTIVO [WASAPI / C++ DSP]", true);

    // Guardar dispositivo actual antes de conmutar
    auto activeDevices = policyManager_.enumerateActiveRenderEndpoints();
    for (const auto& dev : activeDevices)
    {
        if (dev.isDefaultConsole)
        {
            originalEndpointId_ = dev.id;
            dashboardPanel_->setDeviceName(juce::String(dev.friendlyName.c_str()));
            break;
        }
    }

    // 1. Sincronizar parámetros con el grafo de audio
    currentProfile_.dspParams.spatial = currentProfile_.spatialParams;
    currentProfile_.dspParams.spatial.bypass = !currentProfile_.enableSpatial || currentProfile_.spatialParams.bypass;

    const double sr = currentProfile_.audio.sampleRate > 0.0 ? currentProfile_.audio.sampleRate : 48000.0;
    const int ch = currentProfile_.audio.numChannels > 0 ? currentProfile_.audio.numChannels : 2;

    if (audioGraph_)
    {
        audioGraph_->prepare(sr, 512, ch);
        audioGraph_->updateParameters(currentProfile_.dspParams);
    }

    // 2. Iniciar backend WASAPI Full-Duplex
    if (audioBackend_)
    {
        dash::audio::AudioBackendConfig cfg;
        cfg.sampleRate = sr;
        cfg.bufferSize = 512;
        cfg.numChannels = ch;
        cfg.numInputChannels = ch;

        std::string startErr;
        const bool started = audioBackend_->start(
            cfg,
            [this](float* const* channels, int numChannels, int numSamples) {
                if (audioGraph_)
                {
                    audioGraph_->process(channels, numChannels, numSamples);
                }
            },
            [this](dash::audio::AudioBackendState state, const std::string& msg) {
                if (state == dash::audio::AudioBackendState::Error)
                {
                    juce::MessageManager::callAsync([this, msg] {
                        statusBar_.setStatusMessage("Aviso WASAPI: " + juce::String(msg), true);
                    });
                }
            },
            &startErr);

        if (!started)
        {
            statusBar_.setStatusMessage("Aviso WASAPI: " + juce::String(startErr), true);
        }
    }

    executeConfigInjection();
    statusBar_.setStatusMessage("Modo táctico activado con éxito.");
}

void MainComponent::deactivateAndRestoreAudio()
{
    isProfileActive_ = false;
    dashboardPanel_->setEngineRunning(false);
    statusBar_.setEngineStatus("MOTOR: DETENIDO", false);

    if (audioBackend_)
    {
        audioBackend_->stop();
    }

    std::string err;
    const auto apoConfigPath = dash::audio::ApoConfigInjector::getDefaultApoConfigPath();
    const auto remRes = dash::audio::ApoConfigManager::removeManagedSection(apoConfigPath, &err);

    if (!originalEndpointId_.empty())
    {
        policyManager_.setDefaultEndpoint(originalEndpointId_);
    }

    if (remRes == dash::audio::ApoConfigResult::Success || remRes == dash::audio::ApoConfigResult::FileNotFound)
    {
        statusBar_.setStatusMessage("Audio original de Windows restaurado.");
    }
    else
    {
        statusBar_.setStatusMessage("Aviso al restaurar APO: " + juce::String(err), true);
    }
}

} // namespace dash::ui
