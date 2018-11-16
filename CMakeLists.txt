cmake_minimum_required(VERSION 3.9.0)

project(experiments-super)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE external)

include(dependencies.cmake)
include(experiments.cmake)
