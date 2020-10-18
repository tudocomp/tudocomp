tudocomp
========

The Technical University of DOrtmund COMPression Framework
(*tudocomp*) is a lossless compression framework with the aim to support and
facilitate the implementation of novel compression algorithms. It already
comprises a range of standard data compression and encoding algorithms. These
can be mixed and parameterized with the following uses in mind:

* Baseline implementations of well-known compression schemes.
* Detailed benchmarking and comparison of compression and encoding algorithms.
* Easy integration of new algorithm implementations.

The framework offers a solid and extensible base for new implementations. Its
design is focused on modularity and interchangeability for finding the optimal
compression strategy for a given input while minimizing the performance
overhead.

# Dependencies

[CMake](https://cmake.org/) (3.0.2 or later) and
[Python 3](https://www.python.org/) are required to build tudocomp. Our code
is compatible to gcc (5.4 or later) and clang (3.4 or later).

The dependency packages of tudocomp do not require to be installed on the build
system. Our CMake scripts provide tools to automatically download, build and
deploy them locally or allow pointing to any existing installation. More
information can be found in the documentation.

[Google Logging (glog)](https://github.com/google/glog) (0.3.5 or later) is
required for all modules, which again requires
[gflags](https://github.com/gflags/gflags) (2.2.2 or later). Furthermore,
[Google Test](https://github.com/google/googletest) (1.8.1 or later)
is needed for running the tests.

## Soft dependencies

These soft dependencies are required by some algorithm implementations, but
tudocomp also functions without them:
* [SDSL](https://github.com/simongog/sdsl-lite) (2.1.1 or later).
* [STXXL](http://stxxl.org/) (`master` development branch only).

In case one of these dependencies cannot be resolved, the features that use
them will be excluded from the build process.

## Windows

There is no explicit support for Windows. However, it is possible to
deploy tudocomp in the
[Windows Subsystem for Linux](https://msdn.microsoft.com/en-us/commandline/wsl/about)
with little (if any) limitations.

# License

The framework is published under the
[Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)

