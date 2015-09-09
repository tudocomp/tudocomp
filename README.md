The TuDoComp compression framework
==================================

This repository contains the TuDoComp compression framework.

# Dependecies

This repository depends on the following tools and C++ libraries
being installed in the system:

- Command line tools
  - cmake
  - make
  - gcc or clang (Or any other C++ compiler)
  - git
  - svn
  - wget (optional, for datasets)
  - gzip (optional, for datasets and comparision)
  - bzip, 7zip (optional, for comparisions)
  - doxygen (optional, for documentation)
- C++ libraries
  - glog (aka google log)
  - boost

Additionally, the makefile requires a internet connection because some of
the dependencies are downloaded directly, including:

- C++ libraries like gtest, docopt, sdsl-lite
- The text test corpus (optional, ca 10GB)

# Compiling and running unit tests

> NOTE: This repo is only tested on a linux system, and will most likely
> not work on windows or OS X.

This repository uses cmake for its buildsystem, for more information about
all its functionality refer to the official cmake docs.

For quickly getting this repository to compile and
run its unit tests, follow these steps in a terminal:

1. Make sure all dependencies listed above are met.
2. If not already done, clone this git repository into a local directory.
3. `cd` into that directory.
4. `mkdir ./build` to create a seperate subdirectory for the build files.
  (This can be deleted at any point to remove the build artifacts)
5. `cd ./build`
6. `cmake ..` to prepare the build files in the build directory based on
   the content of the repository itself (hence the .. to get one level up).
   This will create a makefile in the current (build) directory.

Preparations are done with those steps, and now you can run any
of the following commands in the build dir at any point
during the development process:

- `make` to just compile everything that changed, and see possible compiler
   errors.
- `make check` to also run the unit tests in addition to `make`.
- `make docs` to generate html and latex documentation.
- `make datasets` to download (large) amounts of test text files.
- `make clean` to remove every build artifact without removing the
  makefile itself, allowing a clean start.
  > WARNING: This also removes the downloaded datasets if present.
  > You can also `cd` down into the build directory of the subproject itself
  > and do a `make clean` there to only clean it, and not everything else.

Builds will be placed in a subdirectory of `build` corresponding to the source
directory. For example, if you are in `./src/build` and do a `make`, then
afterwards the executable for the commandline interface will be at
`./build/src/tudocomp_driver/tudocomp_driver`.

Documentation will end up in `./build/docs`, and datasets in `./build/datasets`.

## Compiling in release vs debug mode

Per default, the instructions given above will compile code in debug mode,
which will result in slower but better debuggable binaries
(They will include optional asserts, debugging log output, etc).

This is fine except to get usable results from benchamrks. For those, you
need to initalize the build directory with
`cmake -DCMAKE_BUILD_TYPE=Release ..` instead to make it explicit build in
optimized release mode.

## Source organisation

- `./src/`: All non-test source code.
- `./src/tudocomp`: The tudocomp library itself.
- `./src/tudocomp_driver`: The tudocomp command line interface.
- `./src/dummy`: A example/debugging compressor/encoder implementation.
- `./src/...`: Other implementations and tools.
- `./test/`: All unit test source code.
- `./datasets/`: Script for downloading datasets (different kinds of text files).

## Unit tests

Unit tests are compiled and executed with `make check`.
They are using the "gtest" library and will output to the
terminal for each test wether it succeeded or not,
as well as any additional error infomation.

## Extending tudocomp with new compressor or encoder implementations

Extending this framework mainly involves implementing custom compressor
and/or encoder classes that generate a list of substitution rules and/or
encode them in an output file. This is done by inheriting from a
`Compressor` and/or `Coder`, and registering those custom implementations in
the framework.

So, if you want to add new code to the repository, say everything produced as
part of a bachelor thesis, then you'll probably want to create a
new subproject.

For that, you'll need to add and change a number of files in the repository
as described below.

Note that every of those location will already contain entries for the "Dummy"
subproject that can be used as an example.

1. Create a new directory for your subproject in `./src`
2. Create (or copy from another subproject) a new `CMakeLists.txt`
   into that directory. This will be the cmake build script for your code.
   For more details on how to write such a file, see the CMake docs.
3. Add all `.cpp` files that make up your project into that file.
4. Add a additional `add_subdirectory(<YOUR DIRECTORY>)` line into
   `./src/CMakeLists.txt`.
   This will make your code actually get picked up by the buildsystem.
5. Add a C++ file for your project into `./test/` that contains unit tests
   for your code, and add a corresponding
   `run_test(<YOUR CPP FILENAME> <YOUR SUBPROJECT>)`
   line into `./test/CMakeLists.txt`.
6. Add a dependency on your subproject in
   `./src/tudocomp_driver/CMakeLists.txt` to make the command line interface
   link to it.
7. For every compression and encoding algorithm add a entry in
   `./src/tudocomp_driver/tudocomp_algorithms.cpp` to register it in the
   command line interface.

### Logging

You can use the `glog` library to add log statements into your code that can
be enabled/disabled with a environment variable and/or compiletime flags.
This can be used for debugging and during development as a better mechanism
for debug println()s.

## Corpus of text files from pizzachili.dcc.uchile.cl

`make datasets` will download a lot of large text files that can be
used for comparing different compression and encoding algorithms against each
other.
