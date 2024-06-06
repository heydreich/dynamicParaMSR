aux_source_directory(. DIR_LIB_SRCS)
add_library (single ${DIR_LIB_SRCS})
target_link_libraries(single common ec util protocol pthread)
