#include <tudocomp_stat/PhaseData.hpp>

using tdc::PhaseData;

void (*PhaseData::s_stxxl_start)(PhaseData*) = nullptr;
void (*PhaseData::s_stxxl_read_stats)(PhaseData*) = nullptr;
void (*PhaseData::s_stxxl_stop)(PhaseData*) = nullptr;



