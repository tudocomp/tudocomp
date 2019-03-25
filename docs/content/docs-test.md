@page test Unit Tests

# Unit Tests
The unit tests are powered by Google Test, which needs to be installed in order
for the tests to become available (see @ref build for more information).

Test suites are implemented in the source code files located in the `test`
directories and are registered in the CMake script (`CMakeLists.txt`). Said
script will generate a `make` target for each test suite. For example, the test
suite implemented in `bit_io_tests.cpp` can be run as follows:
@code{.sh}
make bit_io_tests
@endcode

The following global targets are available for batch testing:
* `make check` -- performs *all* tests.
* `make build_check` -- builds all tests, but without executing them.
* `make matrix_tests` -- see below.

Google Test suites are usually run completely, continuing even in case a single
test fails. In order to terminate the test once a failure occurs, the
`--gtest_break_on_failure` parameter can be passed to the make target via the
command line parameter. This can be useful for debugging with `gdb`.

### Matrix Test
The matrix test performs a compression/decompression roundtrip on various
border-case input files for each registered compressor class instance
(as provided by the @ref registry).

In each roundtrip, an input file is compressed and then decompressed. It is
successful if the output of the decompression equals the original input file.
The test inputs include various border cases such as the empty string, strings
of alphabet size one (*aaaa...*), byte alphabets and so on.

To this end, the matrix test is a very useful tool for final testing of
newly developed compressors or after refactorings.

#### Filters
Matrix tests are often needed only for specific compressor instances. The
following enviroment variables are supported to filter based on a compressor's
command line (see @ref driver for more details on the command lines):

| Variable | Description |
|----------|-------------|
| `MATRIX_PATTERN` | Only tests containing the given pattern are executed. |
| `MATRIX_EXCLUDE` | Tests containing the given pattern are not executed. |

For example, in case we want to run only instances of the
@ref tdc::LZ78Compressor that use rice encoding (@ref tdc::RiceCoder), we use:
@code{.sh}
MATRIX_PATTERN="lz78(coder=rice()" make matrix_tests
@endcode

Running the matrix test without any filters will print a list of command lines
to be executed. This list can be used as a reference when looking for a pattern.
Note that `MATRIX_PATTERN` and `MATRIX_EXCLUDE` may be combined.

### Sandbox
The `sandbox_tests` suite is ignored by the framework's repository and can be
used for quick developmental tests to avoid the registration procedure
altogether.
