#pragma once

#include <string>

namespace dash::audio
{

/**
 * Metadatos y capacidades de un dispositivo de audio del sistema operativo.
 */
struct AudioDeviceInfo
{
    std::wstring id;                 // Identificador único del sistema (MMDevice ID / GUID)
    std::wstring friendlyName;       // Nombre legible por el usuario (ej. "Auriculares Realtek Audio")
    std::wstring endpointGuid;       // Porción GUID aislada "{xxxxxxxx-...}"
    int          preferredSampleRate = 48000;
    int          preferredChannels   = 2;
    bool         isDefault           = false;
    bool         isVirtualCable      = false;
};

} // namespace dash::audio
