#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <tudocomp_stat/json.hpp>

namespace tdc {
    using json = nlohmann::json;

/// \brief Virtual interface for extensions.
///
/// When constructed, measurement shall start. During the extension's lifetime,
/// multiple calls to write may occur.
class StatPhaseExtension {
public:
    /// \brief Writes phase data.
    /// \param data the data object to write to.
    virtual void write(json& data) = 0;

    /// \brief Propagates the data of a sub phase to this phase.
    /// \param ext the corresponding extension in the sub phase.
    virtual void propagate(const StatPhaseExtension& sub) {
    }

    /// \brief Notifies the extension that tracking is being paused explicitly.
    virtual void pause() {
    }

    /// \brief Notifies the extension that paused tracking is being resumed.
    virtual void resume() {
    }
};

}

