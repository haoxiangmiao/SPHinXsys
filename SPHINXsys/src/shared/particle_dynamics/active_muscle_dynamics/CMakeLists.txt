set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake) # main (top) cmake dir
include(Headersearch)

file(GLOB BASE_DIR_HEADERS *.h)
DIR_INC_HEADER_NAMES("${BASE_DIR_HEADERS}" BASE_DIR_HEADER_NAMES)
#message("${BASE_DIR_HEADER_NAMES}")
INSTALL(FILES ${BASE_DIR_HEADER_NAMES} DESTINATION 2d_code/include)
INSTALL(FILES ${BASE_DIR_HEADER_NAMES} DESTINATION 3d_code/include)