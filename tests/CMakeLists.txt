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
    data/rcv1-0
  COMMAND
    curl --output data/rcv1-0.bz2
      https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/binary/rcv1_test.binary.bz2
  COMMAND
    bunzip2 data/rcv1-0.bz2
  COMMENT
    "Downloading and unpacking the rcv1 dataset..."
)
add_custom_command(
  OUTPUT
    data/rcv1-0.bin
    data/rcv1-1.bin
    data/rcv1-2.bin
    data/rcv1-3.bin
    data/rcv1-4.bin
    data/rcv1-5.bin
  DEPENDS
    data/rcv1-0
  COMMAND
    csv-splitter -i data/rcv1-0 -o data/rcv1 -n 5
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
    "Splitting the rcv1 dataset and saving them in binary format..."
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

# Binaries for serial execution
add_executable(qp-experiment
  src/qp.cpp
)
target_include_directories(qp-experiment
  PRIVATE
    include
)
target_link_libraries(qp-experiment
  PRIVATE
    Boost::program_options
    polo::polo
)

add_executable(logloss-serial-mb-amsgrad
  src/logloss-shared.cpp
)
target_include_directories(logloss-serial-mb-amsgrad
  PRIVATE
    include
)
target_compile_definitions(logloss-serial-mb-amsgrad
  PRIVATE
    SERIAL_MB_AMSGRAD
)
target_link_libraries(logloss-serial-mb-amsgrad
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
    results/qp-serial-gd.csv
    results/qp-serial-nesterov.csv
    results/qp-serial-adam.csv
  COMMAND
    qp-experiment -d 10000 -L 20 -K 2500
  COMMENT
    "Running serial QP experiments..."
)
add_custom_command(
  OUTPUT
    results/rcv1-0-serial-mb-amsgrad-1000.csv
    results/rcv1-0-serial-mb-amsgrad-5000.csv
    results/rcv1-0-serial-mb-amsgrad-20000.csv
  DEPENDS
    data/rcv1-0.bin
    data/rcv1-1.bin
    data/rcv1-2.bin
    data/rcv1-3.bin
    data/rcv1-4.bin
    data/rcv1-5.bin
  COMMAND
    logloss-serial-mb-amsgrad -d 0 -f 0 -l 1e-4 -K 20000 -M 1000
  COMMAND
    simulator -d 0 -s 0-serial-mb-amsgrad-1000 -t 0.05
  COMMAND
    logloss-serial-mb-amsgrad -d 0 -f 0 -l 1e-4 -K 20000 -M 5000
  COMMAND
    simulator -d 0 -s 0-serial-mb-amsgrad-5000 -t 0.05
  COMMAND
    logloss-serial-mb-amsgrad -d 0 -f 0 -l 1e-4 -K 20000 -M 20000
  COMMAND
    simulator -d 0 -s 0-serial-mb-amsgrad-20000 -t 0.05
  COMMENT
    "Running serial logloss experiments..."
)
add_custom_command(
  OUTPUT
    results/rcv1-0-ps-piag.csv
  DEPENDS
    data/rcv1-0.bin
    data/rcv1-1.bin
    data/rcv1-2.bin
    data/rcv1-3.bin
    data/rcv1-4.bin
    data/rcv1-5.bin
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
    results/qp-serial-gd.csv
    results/qp-serial-nesterov.csv
    results/qp-serial-adam.csv
    results/rcv1-0-serial-mb-amsgrad-1000.csv
    results/rcv1-0-serial-mb-amsgrad-5000.csv
    results/rcv1-0-serial-mb-amsgrad-20000.csv
    results/rcv1-0-ps-piag.csv
  COMMAND
    pdflatex figures.tex
  COMMENT
    "Generating the figures..."
)
