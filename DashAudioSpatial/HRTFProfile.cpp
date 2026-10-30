#include "HRTFProfile.h"

namespace dash::spatial
{

bool HRTFProfile::isValid() const noexcept
{
    return !id.empty() && irLengthTaps > 16 && irLengthTaps <= 2048 && sampleRate >= 8000.0;
}

std::string HRTFProfile::getSummary() const
{
    return name + " [" + id + "] (" + std::to_string(static_cast<int>(sampleRate)) + " Hz, "
        + std::to_string(irLengthTaps) + " taps) - " + license;
}

} // namespace dash::spatial
