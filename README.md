<p align="center">
  <a href="https://github.com/ecmwf/codex/blob/main/ESEE/readme.md">
    <img src="https://img.shields.io/badge/Production-Chain-blue" alt="ESEE Production Chain">
  </a>
  <a href="https://github.com/ecmwf/codex/blob/refs/heads/main/Project%20Maturity/readme.md">
    <img src="https://img.shields.io/badge/Maturity-Sandbox-yellow" alt="Maturity Sandbox">
  </a>
  <a href="https://codecov.io/gh/ecmwf-projects/plume-plugin-axe">
    <img src="https://codecov.io/gh/ecmwf-projects/plume-plugin-axe/branch/develop/graph/badge.svg" alt="Code Coverage">
  </a>
  <a href="https://opensource.org/licenses/apache-2-0">
    <img src="https://img.shields.io/badge/License-Apache%202.0-blue.svg" alt="License: Apache 2.0">
  </a>
  <a href="https://github.com/ecmwf-projects/plume-plugin-axe/releases">
    <img src="https://img.shields.io/github/v/release/ecmwf-projects/plume-plugin-axe?color=blue&label=Release&style=flat-square" alt="Latest Release">
  </a>
  <a href="https://plume-plugin-axe.readthedocs.io/en/latest/?badge=latest">
    <img src="https://readthedocs.org/projects/plume-plugin-axe/badge/?version=latest" alt="Documentation Status">
  </a>
</p>

<p align="center">
  <a href="#requirements">Requirements</a> *
  <a href="#installation">Installation</a> *
  <a href="#testing">Testing</a>
</p>

# plume-plugin-axe

> \[!IMPORTANT\]
> This software is **Sandbox** and subject to ECMWF's guidelines on [Software Maturity](https://github.com/ecmwf/codex/raw/refs/heads/main/Project%20Maturity).

The Plume Plugin *"axe"* extracts data from global fields of an Earth System model in pre-defined lat/lon areas. This plugin is compatible with the Plume plugin mechanism. More on Plume and plugins on the links below.

 - Plume: https://github.com/ecmwf/plume
 - Plume example plugins: https://github.com/ecmwf/plume-examples


# Requirements
Build dependencies:

- C/C++ compiler (C++17)
- Fortran 2008 compiler
- CMake >= 3.25 --- For use and installation see http://www.cmake.org/
- ecbuild >= 3.13.1 --- ECMWF library of CMake macros (https://github.com/ecmwf/ecbuild)

Runtime dependencies:
  - eckit >= 1.33.2 (https://github.com/ecmwf/eckit)
  - Atlas >= 0.46.0 (https://github.com/ecmwf/atlas)
  - Plume >= 0.5.0 (https://github.com/ecmwf/plume)


# Installation
plume-plugin-axe employs an out-of-source build/install based on CMake.
Make sure ecbuild is installed and the ecbuild executable script is found ( `which ecbuild` ).
Now proceed with installation as follows:

```bash
# Environment --- Edit as needed
srcdir=$(pwd)
builddir=build
installdir=$HOME/.local

# 1. Create the build directory:
mkdir $builddir
cd $builddir

# 2. Run CMake
ecbuild --prefix=$installdir -- \
  -Deckit_ROOT=<path/to/eckit/install> \
  -Datlas_ROOT=<path/to/atlas/install> \
  -Dplume_ROOT=<path/to/plume/install> $srcdir

# 3. Compile / Install
make -j10
make install
```

# Testing
To test the installation:

```bash
cd $builddir
make test
```

## License
See [LICENSE](LICENSE)

## Copyright
© 2023 ECMWF. All rights reserved.
