@page build Building tudocomp

# Building tudocomp
[CMake](https://cmake.org/) (3.0.2 or later) and
[Python 3](https://www.python.org/) are required to build tudocomp.

To simply build tudocomp in its entirety, use the
`compile.sh` script in the project's root (or `compile_debug.sh` to build in
`Debug` mode). This will download and build any dependencies that could
not be found on the system automatically. For more detailed information about
the build process, read the following sections.

## Initialization
tudocomp is a CMake project and initialized by creating and initializing a
build directory as follows:
@code{.sh}
mkdir build
cd build
cmake ..
@endcode

By default, tudocomp will be setup to `Release` mode. If you wish to build
tudocomp in `Debug` mode, pass `-DCMAKE_BUILD_TYPE=Debug` to CMake.

## Building

In case dependencies are missing, you will receive warnings after calling CMake
correspondingly. See below to handle these. When all hard dependencies are
found, the build script will create targets to build tudocomp.
To do so, simply call `make` with no target:
@code{.sh}
make
@endcode

## Dependencies

There are *hard* and *soft* dependencies for tudocomp. The hard dependencies
are those absolutely required to build tudocomp, the soft dependencies only
affect a subset of features that will be disabled in case the respective
dependencies are not available. Most dependencies are available to be downloaded
and deployed automatically (see below).

Hard dependencies:
* [Google Logging (glog)](https://github.com/google/glog) (0.3.5 or later)
* [gflags](https://github.com/gflags/gflags) (2.2.2 or later)

Soft dependencies:
* [Google Test](https://github.com/google/googletest) (1.8.1 or later)
* [SDSL](https://github.com/simongog/sdsl-lite) (2.1.1 or later).
* [STXXL](http://stxxl.org/) (`master` development branch only).
* [OpenMP](https://www.openmp.org/)°
* [Judy](http://judy.sourceforge.net/)° -- some Linux repositories provide
  the `libjudy-dev` package.

° *These dependencies are not auto-deployable by the tudocomp build system
and need to be installed manually.*

### Automatic Download and Deployment

After the initial call of `cmake ..`, our build system has setup `make` targets
for the automatic download, build and deployment of the auto-deployable
dependencies. These targets are prefixed `get_`, so to download and install
glog, for example, use

@code{.sh}
make get_glog
cmake ..
@endcode

This will clone glog in the `external` subdirectory of the build directory and
build it. The subsequent invokation of CMake is necessary for the tudocomp
build system to find the newly built glog instance.

There is an individual `get_` target for each dependency, but it is also
possible to use the following to download and build *all* downloadable
dependencies:
@code{.sh}
make get_alldeps
cmake ..
@endcode

### Existing Installations

You can point to existing installations of tudocomp's dependencies by passing
the following options to CMake (or `compile.sh`).

| Option | Description |
|--------|-------------|
| `-DGFLAGS_ROOT_DIR=<dir>` | Look for gflags in `<dir>`. |
| `-DGLOG_ROOT_DIR=<dir>` | Look for glog in `<dir>`. |
| `-DGTEST_ROOT=<dir>` | Look for gtest in `<dir>`. |
| `-DSDSL_ROOT_DIR=<dir>` | Look for SDSL in `<dir>`. |
| `-DSTXXL_ROOT_DIR=<dir>` | Look for STXXL in `<dir>`. |

## Build Configuration

The following options can be passed to CMake (or `compile.sh`) in order to
configure the tudocomp build.

| Option | Description |
|--------|-------------|
| `-DCMAKE_BUILD_TYPE=<Release/Debug>` | Sets the build type to `Release` (default) or `Debug`. |
| `-DLEN_BITS=<bits>` | Sets the bit width of the @ref tdc::len_compact_t type to `<bits>`. This defaults to 32 -- use a higher bit width if you plan on working with large inputs (e.g. 40 bits for inputs up to 1 TiB). |
| `-DMALLOC_DISABLED` | Disables the `malloc` override of the statistics module, which may be required when linking third-party libraries that also override `malloc`. Note that memory tracking does not function when the override is disabled.
| `-DSTATS_DISABLED` | Disables the statistics module, including the `malloc` override. Statistics tracking is enabled by default -- disable it in order to rule out any performance impact. |
| `-DTDC_REGISTRY=<file>` | Uses `<file>` as the registry for build configuration. See @subpage registry for more information. |

## Unit Tests

See @ref test for information on running the unit tests.
