# SDSL
ExternalProject_Add(
    get_sdsl PREFIX external/sdsl
    GIT_REPOSITORY https://github.com/simongog/sdsl-lite.git
    GIT_TAG ddb0fbbc33bb183baa616f17eb48e261ac2a3672 # master as of Sep 18, 2018
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL 1
)

find_package(SDSL)
if(SDSL_FOUND)
    include_directories(${SDSL_INCLUDE_DIRS})
else()
    add_dependencies(get_alldeps get_sdsl)
    MESSAGE(STATUS
    "    SDSL is required for some functionality. "
    "Use -DSDSL_ROOT_DIR=<path> to point to an existing SDSL installation or "
    "use 'make get_sdsl' to download and install SDSL locally")
endif()

