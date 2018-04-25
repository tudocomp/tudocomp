#pragma once

#include <tudocomp/compressors/lzss/Factor.hpp>

namespace tdc {
namespace lzss {

/// \brief Contains various statistics about a factorization.
struct FactorizationStats {
    // general facts
    size_t num_factors, num_runs, num_replaced, num_unreplaced;

    // factor lengths
    size_t len_min, len_max, len_med;
    float len_avg;

    // references
    bool bidirectioal;
    size_t dist_min, dist_max, dist_med;
    float dist_avg;

    // literal runs
    size_t run_max, run_med;
    float run_avg;

    inline FactorizationStats(FactorBuffer& factors, const size_t n) {
        CHECK(factors.is_sorted())
            << "factors need to be sorted before they can be analyzed";

        const size_t m = factors.size();

        num_factors = m;

        len_min = factors.shortest_factor();
        len_max = factors.longest_factor();

        std::vector<size_t> lens; lens.reserve(m);
        len_avg = 0.0f;

        std::vector<size_t> runs; runs.reserve(m);
        run_max = 0;
        run_avg = 0.0f;

        std::vector<size_t> dists; dists.reserve(m);
        dist_min = SIZE_MAX;
        dist_max = 0;
        dist_avg = 0.0f;

        bidirectioal = false;

        num_replaced = 0;
        num_unreplaced = 0;
        num_runs = 0;

        auto register_run = [&](size_t run){
            num_unreplaced += run;
            run_avg += float(run);
            runs.emplace_back(run);
            run_max = std::max(run_max, run);
        };

        size_t p = 0;
        for(auto& f : factors) {
            const size_t fpos = f.pos;
            const size_t fsrc = f.src;
            const size_t flen = f.len;

            size_t dist;
            if(fsrc < fpos) {
                dist = fpos - fsrc;
            } else {
                bidirectioal = true;
                dist = fsrc - fpos;
            }

            dist_min = std::min(dist_min, dist);
            dist_max = std::max(dist_max, dist);
            dist_avg += float(dist);
            dists.emplace_back(dist);

            num_replaced += flen;
            len_avg += float(flen);
            lens.emplace_back(flen);

            if(p < f.pos) {
                register_run(fpos - p);
            }

            p = fpos + flen;
        }

        if(p < n) {
            register_run(n - p);
        }

        // compute averages
        len_avg /= float(m);
        dist_avg /= float(m);

        num_runs = runs.size();
        run_avg /= float(num_runs);

        // compute medians
        std::sort(lens.begin(), lens.end());
        len_med = lens[lens.size() / 2];

        std::sort(runs.begin(), runs.end());
        run_med = runs[runs.size() / 2];

        std::sort(dists.begin(), dists.end());
        dist_med = dists[dists.size() / 2];
    }

    /// logs the statistics
    inline void log() const {
        StatPhase::log("num_factors", num_factors);
        StatPhase::log("num_runs", num_runs);
        StatPhase::log("num_replaced", num_replaced);
        StatPhase::log("num_unreplaced", num_unreplaced);
        StatPhase::log("len_min", len_min);
        StatPhase::log("len_max", len_max);
        StatPhase::log("len_avg", len_avg);
        StatPhase::log("len_med", len_med);
        StatPhase::log("run_max", run_max);
        StatPhase::log("run_avg", run_avg);
        StatPhase::log("run_med", run_med);
        StatPhase::log("dist_min", dist_min);
        StatPhase::log("dist_max", dist_max);
        StatPhase::log("dist_avg", dist_avg);
        StatPhase::log("dist_med", dist_med);
        StatPhase::log("bidirectioal", bidirectioal);
    }
};

}} //ns
