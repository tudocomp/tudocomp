ExternalProject_Add(
    glog_external
    GIT_REPOSITORY https://github.com/google/glog.git
    GIT_TAG d0531421fd5437ae3e5249106c6fc4247996e526 # Working master at this point in time
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
)
ExternalProject_Get_Property(glog_external source_dir install_dir)

# Only build on demand
set_target_properties(glog_external PROPERTIES EXCLUDE_FROM_ALL TRUE)

file(MAKE_DIRECTORY "${install_dir}/include")

set(${package_found_prefix}_CMAKE_DEP glog_external)
if(APPLE)
    set(${package_found_prefix}_LIBRARIES "${install_dir}/lib/libglog.a")
elseif(UNIX)
    set(${package_found_prefix}_LIBRARIES "${install_dir}/lib${LIBSUFFIX}/libglog.a")
endif()
set(${package_found_prefix}_INCLUDE_DIRS "${install_dir}/include")
