if (NOT polo_FOUND)
  set(experimentsDepends polo)
endif()

ExternalProject_Add(
  experiments
  DEPENDS
    ${experimentsDepends}
  SOURCE_DIR
    ${experiments-super_SOURCE_DIR}/tests
  CMAKE_ARGS
    -D CMAKE_BUILD_TYPE=Release
    -D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
  INSTALL_COMMAND
    ""
)
