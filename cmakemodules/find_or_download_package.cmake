macro(find_or_download_package package package_found package_external)
find_package(${package})

# ...

if (NOT ${package_found})
message(WARNING "${package} not found, will download and build locally")

else()
message(STATUS "${package} found, no need to build locally")

endif()

include(${package_external})


find_package(${package} REQUIRED)
endmacro()
