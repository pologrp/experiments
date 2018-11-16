cmake_minimum_required(VERSION 3.9.0)

ExternalProject_Add(
  zeromq
  GIT_REPOSITORY
    https://github.com/zeromq/libzmq
  GIT_TAG
    v4.2.5
  CMAKE_ARGS
    -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -D ENABLE_DRAFTS=OFF
    -D ENABLE_CURVE=OFF
    -D BUILD_TESTS=OFF
    -D BUILD_SHARED=ON
    -D BUILD_STATIC=ON
    -D WITH_OPENPGM=OFF
    -D WITH_DOC=OFF
    -D LIBZMQ_WERROR=OFF
    -D LIBZMQ_PEDANTIC=OFF
)

ExternalProject_Add(
  openblas
  GIT_REPOSITORY
    https://github.com/xianyi/OpenBLAS
  GIT_TAG
    v0.3.3
  CMAKE_ARGS
    -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
    -D BUILD_WITHOUT_LAPACK=OFF
    -D BUILD_WITHOUT_CBLAS=ON
    -D DYNAMIC_ARCH=OFF
)

ExternalProject_Add(
  cereal
  GIT_REPOSITORY
    https://github.com/USCiLab/cereal
  GIT_TAG
    v1.2.2
  CMAKE_ARGS
    -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -D JUST_INSTALL_CEREAL=ON
)

ExternalProject_Add(
  googletest
  GIT_REPOSITORY
    https://github.com/google/googletest
  GIT_TAG
    release-1.8.1
  CMAKE_ARGS
    -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
)

ExternalProject_Add(
  polo
  GIT_REPOSITORY
    https://github.com/pologrp/polo
  CMAKE_ARGS
    -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
  DEPENDS
    zeromq
    openblas
    cereal
    googletest
)
