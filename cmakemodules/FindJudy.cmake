
# - Try to find libjudy


include(CheckIncludeFiles)
check_include_files(Judy.h JUDY_H_AVAILABLE)
if(NOT JUDY_H_AVAILABLE)
	set(JUDY_H_AVAILABLE 0)
endif()
include(CheckLibraryExists)
check_library_exists(Judy  Judy1FreeArray "" JUDY_LIB_AVAILABLE)
include(CheckTypeSize)
check_type_size(size_t SIZEOF_SIZE_T)
set(LIBJUDY_SUPPORT FALSE)
if(SIZEOF_SIZE_T LESS 8)
  message(STATUS "libJudy support disabled (no 64 bit support).")
elseif(JUDY_H_AVAILABLE AND JUDY_LIB_AVAILABLE) 
  message(STATUS "Enabling libJudy support.")
  set(LIBJUDY_SUPPORT TRUE)
  add_definitions(-DDYNAMO_JUDY)
  link_libraries(Judy)
else()
  message(STATUS "libJudy header/library missing.")
endif()
