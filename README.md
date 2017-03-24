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
design is focused on modularity and interchangeability.
This way, the user can combine algorithms to find the optimal compression
strategy for a given input. The framework gives this opportunity while creating
as little performance overhead as possible.

# Dependencies

*tudocomp* is built using [CMake](https://cmake.org) (3.0.2 or later).
It is written in `C++11` with GNU extensions and has been tested with the `gcc`
compiler family (version 4.9.2 or later) and `clang` (version 3.5.2 or later).
The build process requires a [Python](https://www.python.org/) interpreter
(3 or later) to be installed on the system (`py` scripts are invoked directly).

It has the following external dependencies:

* [SDSL](https://github.com/simongog/sdsl-lite)
  (2.1 or later).
* [Gflags](https://gflags.github.io/gflags) (2.1.2 or later).
* [Google Logging (glog)](https://github.com/google/glog) (0.34 or later).

Additionally, the tests require
[Google Test](https://github.com/google/googletest) (1.7.0 or later).

The CMake build process will either find the external dependencies on the build
system if they have been properly installed, or automatically download and build
them from their official repositories in case they cannot be found.

For building the documentation, the following tools are required:

* [LaTeX](http://www.latex-project.org) (specifically the `pdflatex` component)
* [Doxygen](http://doxygen.org) (1.8 or later).
* [Pandoc](http://pandoc.org) (1.19 or later).

## Windows Support

We highly recommend users of Windows 10 or later to use the
[Bash on Ubuntu on Windows](https://msdn.microsoft.com/en-us/commandline/wsl/about).

That being said, *tudocomp* has no explicit support for Windows. However, the
project can be built in a [Cygwin](https://www.cygwin.com/) environment with a
limited feature set. Cygwin does not allow overrides of `malloc`, therefore the
heap allocation counter cannot work and statistics tracking becomes largely
nonfunctional.

# License

The framework is published under the
[Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)

