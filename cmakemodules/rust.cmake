ExternalProject_Add(
    rust_external
    DOWNLOAD_COMMAND curl -sSf https://static.rust-lang.org/rustup.sh > <SOURCE_DIR>/rustup.sh
    DOWNLOAD_NAME <SOURCE_DIR>/rustup.sh
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND cat <SOURCE_DIR>/rustup.sh | sh -s -- --prefix=<INSTALL_DIR> --disable-sudo --disable-ldconfig --yes
    EXCLUDE_FROM_ALL 1
)

# --channel=nightly

# TODO
# add_cargo_executable
# add_cargo_library
# signature something like "target-name source-location dependencies"

function(add_cargo_executable target)
    ExternalProject_Get_Property(rust_external install_dir)
    set(rustc ${install_dir}/bin/rustc)
    set(rustdoc ${install_dir}/bin/rustdoc)
    set(cargo ${install_dir}/bin/cargo)
    set(lib ${install_dir}/lib)
    add_custom_target(${target}
        COMMAND env
            LD_LIBRARY_PATH=${lib}
            RUSTC=${rustc}
            RUSTDOC=${rustdoc}
            CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}
        ${cargo} build
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_dependencies(${target} rust_external)
endfunction(add_cargo_executable)
