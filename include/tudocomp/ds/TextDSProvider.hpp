#pragma once

#include <vector>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>

#include <tudocomp/ds/CompressMode.hpp>

namespace tdc {

class TextDS; //fwd

/// Abstract base for text data structure providesr.
class TextDSProvider : public Algorithm {
protected:
    const TextDS* m_ds;

public:
    using dsid_t = int;
    using dsid_list_t = std::vector<dsid_t>;

    /// Constructor.
    inline TextDSProvider(Env&& env, const TextDS& ds)
        : Algorithm(std::move(env)), m_ds(&ds) {
    }

    /// Gets the id list of the data structures required by this algorithm.
    virtual dsid_list_t requirements() const = 0;

    /// Gets the id list of the data structures computed by this algorithm.
    virtual dsid_list_t products() const = 0;
};

} //ns
