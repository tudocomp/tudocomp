ExternalProject_Add(
    boost_external
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.59.0
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./bootstrap.sh
    BUILD_COMMAND ./b2 -s NO_BZIP2=1 --prefix=<INSTALL_DIR> --with-system install
    #BUILD_COMMAND ./b2 --show-libraries
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(boost_external source_dir install_dir)

# Only build on demand
set_target_properties(boost_external PROPERTIES EXCLUDE_FROM_ALL TRUE)

file(MAKE_DIRECTORY "${install_dir}/include")

set(${package_found_prefix}_CMAKE_DEP boost_external)
set(${package_found_prefix}_LIBRARIES
    "${install_dir}/lib/libboost_system.a"
)
set(${package_found_prefix}_INCLUDE_DIRS "${install_dir}/include")
