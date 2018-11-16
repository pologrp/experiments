# Reproducing Results for 112018-00062

This document walks through the necessary steps to reproduce the results
reported in paper #112018-00062 submitted for possible publication in
[Mathematical Programming Computation][mpc]. Hence, the below `bash` commands
are written for their servers, and should be modified accordingly for other *nix
platforms.

We assume that the host machine has the following installed:

  - `curl`, `configure` and `make` for building [CMake][cmake],
  - a toolchain that supports C++11 for building and using [POLO][polo],
  - `boost`'s `program_options` module for building the test scripts,
  - `curl` and `bunzip2` for downloading and unpacking the `rcv1` dataset, and,
  - `pdflatex` with `mathtools` and `pgfplots` packages for generating the
    figures.

[mpc]: https://link.springer.com/journal/12532
[cmake]: https://gitlab.kitware.com/cmake/cmake
[polo]: https://github.com/pologrp/polo

## Initial Setup

Before anything else, we should make a `local` directory under `$HOME` that
contains binaries, libraries and configuration files of local installations of
the programs:

```bash
mkdir -p $HOME/local/{bin,etc,include,lib,share}
ln -s lib $HOME/local/lib64
```

Then, we should modify `$HOME/.bash_profile` so that the environment variables
`PATH` and `LD_LIBRARY_PATH` point to the correct locations:

```bash
# $HOME/.bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
  . ~/.bashrc
fi

# User specific environment and startup programs

PATH=$HOME/local/bin:$PATH
export PATH

LD_LIBRARY_PATH=$HOME/local/lib
export LD_LIBRARY_PATH
```

After saving the file, we need to `source $HOME/.bash_profile` to make the
changes valid for the current session.

Now that we have setup the paths, we can proceed with the installation of CMake.
POLO requires CMake (at least `v3.9.0`) to install its headers and C-API while
managing its dependencies. Moreover, this repository also contains a superbuild
CMake file to automate the dependency management and generation of the figures.
To install CMake from source, we issue the following on the terminal:

```bash
# Build and install CMake from source
curl --output /tmp/cmake.tar.gz   \
  https://gitlab.kitware.com/cmake/cmake/-/archive/v3.9.0/cmake-v3.9.0.tar.gz
tar xzf /tmp/cmake.tar.gz -C /tmp
cd /tmp/cmake-v3.9.0
./configure --prefix=$HOME/local  \
            --datadir=share/cmake \
            --docdir=doc/cmake    \
            --no-qt-gui
make
make install
```

**NOTE.** At this point, we *might* need to logoff and login back to make
environment changes valid so that `which cmake` points to the local installation
with `cmake --version` reporting `3.9.0`.

## Experiments

Having successfully installed CMake, we finaly clone this repository and
initiate the superbuild:

```bash
git clone https://github.com/pologrp/experiments $HOME/experiments
mkdir $HOME/experiments/build
cd $HOME/experiments/build
cmake -D CMAKE_INSTALL_PREFIX=$HOME/local ../
cmake --build .
```

to

  - install all the necessary programs, i.e., [0MQ (`v4.2.5`)][zeromq],
    [OpenBLAS (`v0.3.3`)][openblas], [cereal (`v1.2.2`)][cereal], [Google Test
    (`v1.8.1`, for unit testing)][gtest], and [POLO][polo],
  - build and run the test scripts used in the paper, and,
  - reproduce the figures from the generated results.

[zeromq]: https://github.com/zeromq/libzmq
[openblas]: https://github.com/xianyi/OpenBLAS
[cereal]: https://github.com/USCiLab/cereal
[gtest]: https://github.com/google/googletest

When the superbuild finishes, we should find a `figures.pdf` file under
`$HOME/experiments/build/external/BUILD/experiments`.
