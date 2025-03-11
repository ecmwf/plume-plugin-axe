# plume-plugin-axe

:warning: This software is still under development and not yet ready for operational use

The Plume Plugin *"Axe"* extracts data from global fields of an Earth System model in pre-defined lat/lon areas. This plugin is compatible with the Plume plugin mechanism. More on Plume and plugins on GitHub (links below).

 - Plume: https://github.com/ecmwf/plume
 - Plume example plugins: https://github.com/ecmwf/plume-examples

### Requirements
Build dependencies:

- C/C++ compiler (C++17)
- Fortran 2008 compiler
- CMake >= 3.16 --- For use and installation see http://www.cmake.org/
- ecbuild >= 3.5 --- ECMWF library of CMake macros (https://github.com/ecmwf/ecbuild)

Runtime dependencies:
  - eckit >= 1.18.0 (https://github.com/ecmwf/eckit)
  - Atlas >= 0.32.0 (https://github.com/ecmwf/atlas)
  - Plume >= 0.1.0 (https://github.com/ecmwf/poume)

### Installation
*plume-plugin-axe* employs an out-of-source build/install based on CMake.
Make sure ecbuild is installed and the ecbuild executable script is found ( `which ecbuild` ).
Now proceed with installation as follows:

```bash
# Environment --- Edit as needed
srcdir=$(pwd)
builddir=build
installdir=$HOME/local  

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

### Testing
To test the installation:

```bash
cd $builddir
make test
```