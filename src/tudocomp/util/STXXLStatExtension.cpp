#include <tudocomp/util/STXXLStatExtension.hpp>

#ifdef STXXL_FOUND

stxxl::stats* tdc::STXXLStatExtension::s_stats = stxxl::stats::get_instance();

#endif
