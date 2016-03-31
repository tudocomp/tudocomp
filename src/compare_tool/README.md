The compare tool

# How to build

- `make compare_tool` to only build the tool.
  The actual binary will be in `<build>/src/compare_tool/debug/compare_tool`

- `make compare_workspace` to also build a "compare_workspace" directory
  in the current cmake build directory (`<build>/compare_workspace`).

  It contains the following files per default:
  - `run.sh`, which is a script that triggers a
    `make tudocomp_driver`, `make compare_tool` and `make compare_workspace`
    and then runs the compare tool with environment variables set
    to point at the datasets directory.
    The reason for invoking the targets is to allow easy iteration
    between changes to either of the involved tools and config files.
  - `config.toml`, which is a default test config at that point in
    the repository.
    Note that in order to have files in the dataset directory to compare,
    you probably want to run `make datasets` once to download them.
    (Warning: Multiple gigabytes over a slow connection. To prevent accidental deletion, the files will be downloaded into the source directory instead of the build directory of the cmake build)

# How to use

You call the compare tool with the syntax

    <config_file> <profile> [--with_mem]

where config_file is a file like `config.toml` that contains a number of
comparision "profile"s.

A profile contains a m x n matrix of shell commands and text files,
which are executed in all combinations while
logging statistics like runtime and compression ratio to the
command line.

The `--with_mem` option enables running the individual commands under valgrind
to report a detailed memory profile. For more details, see its section in the
config file format below.

For the tool to work, both `sh` and `valgrind` need to be
callabel, so there is an implicit dependency on unix/linux systems for now.

Example in the default comparision workspace:

    ./run.sh config.toml lz78

# Config file format

The config file is written as TOML (https://github.com/toml-lang/toml)
which is a simple windows-ini-file like file format.

Each test profile is defined as a TOML "table" of the following format:

    [NAME]
    inputs = [
        FILE, ...
    ]
    commands = [
        COMMAND_AND_ARGS, HIDDEN_ARGS, FILE_ENDING
    ]
    compare_commands = BOOL
    with_mem = BOOL
    inherits_from = PROFILE_NAME

- `NAME` defines the name of the profile
- `FILE, ...` define the input text files to compare.
  They are given as strings that may see environment variables
  in the `sh` syntax, so things like `"$DATASETS/download/real/english.50MB"`
  work.
- `COMMAND_AND_ARGS` is a string that defines the command line for a given
  command, and is also used for display purposes in the tool output.
- `HIDDEN_ARGS` defines additional command line arguments for the
  previous command that should not be shown.
  This is mainly used for unrelated "glue" arguments, like directing
  input/output files correctly, or enabling/disabling unrelated options.
- `FILE_ENDING` defines what file ending to give to the output of this
  command. Might be automatically choosen in the future.
- `compare_commands` is a option that selects whether to
  compare commands or to compare input files.
  If it is set to `true`, the tool will iterate
  `forall commands { forall files { run } }`,
  if it is set to to `false` it will iterate
  `forall files { forall commands { run } }`.
  Defaults to false.
- `with_mem`, if set to `true`, will invoke the command through valgrinds
  massif tool and emit an additional output file containg
  a detailed heap profile in massifs format. This file
  can be viewed with an external tool like `massif-visualizer`.
  Defaults to false.

Example file:

    [gzip_and_lz78]
    compare_commands = true
    inputs = [
        "$DATASETS/download/real/english.50MB",
        "$DATASETS/download/real/sources.50MB",
    commands = [
        ["tudocomp_driver -a lz78.bit", "$IN -o $OUT",   "lz78.bit.tdc" ],
        ["gzip -9",                     "-c $IN > $OUT", "gz"           ],
    ]
