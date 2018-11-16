ExternalProject_Add(
  experiments
  DEPENDS
    polo
  SOURCE_DIR
    ${experiments-super_SOURCE_DIR}/tests
  CMAKE_ARGS
    -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
  INSTALL_COMMAND
    ""
)
