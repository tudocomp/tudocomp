ExternalProject_Add(
    boost_external
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.59.0
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./bootstrap.sh
    BUILD_COMMAND ./b2 --prefix=<INSTALL_DIR> --without-python install
    #BUILD_COMMAND ./b2 --show-libraries
    INSTALL_COMMAND ""
)
