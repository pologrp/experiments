cmake_minimum_required(VERSION 3.9.0)

project(experiments)

enable_testing()
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(polo CONFIG REQUIRED)

file(COPY paramserver.sh figures.tex DESTINATION .)
file(COPY data/datasets.lst DESTINATION data)
file(MAKE_DIRECTORY ${experiments_BINARY_DIR}/results)

add_custom_command(
  OUTPUT
    data/url-0
  COMMAND
    curl --output data/url-0.bz2
      https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/binary/url_combined.bz2
  COMMAND
    bunzip2 data/url-0.bz2
  COMMENT
    "Downloading and unpacking the url dataset..."
)
add_custom_command(
  OUTPUT
    data/url-0.bin
    data/url-1.bin
    data/url-2.bin
    data/url-3.bin
    data/url-4.bin
    data/url-5.bin
  DEPENDS
    data/url-0
  COMMAND
    csv-splitter -i data/url-0 -o data/url -n 5
  COMMAND
    ds-save-binary -d 0 -s 0
  COMMAND
    ds-save-binary -d 0 -s 1
  COMMAND
    ds-save-binary -d 0 -s 2
  COMMAND
    ds-save-binary -d 0 -s 3
  COMMAND
    ds-save-binary -d 0 -s 4
  COMMAND
    ds-save-binary -d 0 -s 5
  COMMENT
    "Splitting the url dataset and saving them in binary format..."
)

# Utilities
add_executable(csv-splitter
  src/csv-splitter.cpp
)
target_compile_features(csv-splitter
  PRIVATE
    cxx_std_11
)
target_link_libraries(csv-splitter
  PRIVATE
    Boost::program_options
)

add_executable(ds-save-binary
  src/ds-save-binary.cpp
)
target_link_libraries(ds-save-binary
  PRIVATE
    Boost::program_options
    polo::polo
)

add_executable(simulator
  src/simulator.cpp
)
target_link_libraries(simulator
  PRIVATE
    Boost::program_options
    polo::polo
)

# Binaries for distributed-memory parallel execution
add_executable(logloss-ps-piag-master
  src/logloss-distributed.cpp
)
target_include_directories(logloss-ps-piag-master
  PRIVATE
    include
)
target_compile_definitions(logloss-ps-piag-master
  PRIVATE
    MASTER
)
target_link_libraries(logloss-ps-piag-master
  PRIVATE
    Boost::program_options
    polo::polo
)

add_executable(logloss-ps-piag-worker
  src/logloss-distributed.cpp
)
target_include_directories(logloss-ps-piag-worker
  PRIVATE
    include
)
target_compile_definitions(logloss-ps-piag-worker
  PRIVATE
    WORKER
)
target_link_libraries(logloss-ps-piag-worker
  PRIVATE
    Boost::program_options
    polo::polo
)

add_executable(logloss-ps-piag-scheduler
  src/logloss-distributed.cpp
)
target_include_directories(logloss-ps-piag-scheduler
  PRIVATE
    include
)
target_compile_definitions(logloss-ps-piag-scheduler
  PRIVATE
    SCHEDULER
)
target_link_libraries(logloss-ps-piag-scheduler
  PRIVATE
    Boost::program_options
    polo::polo
)

# Experiments
add_custom_command(
  OUTPUT
    results/url-0-ps-piag.csv
  DEPENDS
    data/url-0.bin
    data/url-1.bin
    data/url-2.bin
    data/url-3.bin
    data/url-4.bin
    data/url-5.bin
    logloss-ps-piag-master
    logloss-ps-piag-scheduler
    logloss-ps-piag-worker
  COMMAND
    ./paramserver.sh
  COMMAND
    simulator -d 0 -s 0-ps-piag -t 0.05
  COMMENT
    "Running Parameter Server experiments..."
)
add_custom_target(figures ALL
  DEPENDS
    results/url-0-ps-piag.csv
  COMMAND
    pdflatex figures.tex
  COMMAND
    ${CMAKE_COMMAND} -E echo "Successfully compiled experiment figures in ${experiments_BINARY_DIR}/figures.pdf"
  COMMENT
    "Generating the figures..."
)
