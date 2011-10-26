## qSLDcalc: a Scattering Length Density Calculator

It calculates properties as well as scattering length densities (SLD) of
compounds given by chemical formula.

### Features

- Calculation of Neutron and X-Ray scattering length densities for compounds
- Standalone executable for various platforms (Windows, Linux, Mac)
- Internationalization support (switch language at runtime)
- free as in freedom (GPL)
- Written in C++ and [Qt](http://qt.nokia.com/products)

On github: https://github.com/ibressler/qsldcalc

### Documentation

The full source documentation is 
[available online](http://ibressler.github.com/qsldcalc/).

### How to build

Qt and libcfp is required to build the application. Build libcfp first and
make sure, it's located in *src/libcfp* (either by copy or symbolic link).

CMake is used for building the program on various platforms. Just run *cmake*
to get a list of available generators (which generate build environment
specific project files). On a common Linux system it should be like that:

    cd <qSLDcalc-directory>
    mkdir build
    cd build
    cmake ..
    make

Afterwards, the standalone binary can be found in *bin/*

For advanced build settings (debug symbols, optimization, warnings, etc ...), 
adjust *CMakeLists.txt* to your needs.

In a MSYS shell on a Windows combined with MinGW, you may have to specify a
Makefile generator:

    cmake .. -G "MSYS Makefiles"
    
The selects the cmake generator for Makefiles used in a MSYS shell.

### Copyright

This library is released under the GNU Lesser General Public License (LGPL).
For further information see LICENSE

It was written by Ingo Bressler (qsldcalc at ingobressler.net)
at the Stranski-Laboratory for Physical and Theoretical Chemistry of the 
Technische Universität Berlin.
Postal address:

    TU-Berlin
    Stranski-Laboratorium
    Institut für Chemie
    Fakultät II
    Sekr. TC 7
    Straße des 17. Juni 124
    10623 Berlin

