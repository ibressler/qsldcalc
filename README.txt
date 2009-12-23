qSLDcalc: a Scattering Length Density Calculator

Features
--------

- Calculation of Neutron and X-Ray scattering length densities for compounds
- Standalone executable for various platforms (Windows, Linux, Mac)
- Internationalization support (switch language at runtime)
- free as in freedom (GPL)
- Written in C++ and Qt (http://qt.nokia.com/products)

Documentation
-------------

- The full source documentation is located at

  <qSLDcalc-directory>/doc/html/index.html

How to build
------------

Qt and libcfp is required to build the application. Build libcfp first and
edit <qSLDcalc-directory>/CMakeLists.txt to point it to the preferably static
library.

CMake is used for building the program on various platforms. Just run 'cmake'
to get a list of available generators (which generate build environment
specific project files). On a common Linux system it should be like that:

cd <qSLDcalc-directory>
mkdir build_files
cd build_files
cmake ..
make

Afterwards, the standalone binary can be found in 
<qSLDcalc-directory>/bin

For advanced build settings (debug symbols, optimization, warnings, etc ...), 
adjust <qSLDcalc-directory>/CMakeLists.txt to your needs.

In a MSYS shell on a Windows combined with MinGW, you may have to replace the
command <cmake ..> by <cmake .. -G "MSYS Makefiles"> (choosing the cmake
generator for Makefiles used in a MSYS shell).

Copyright
---------

This program is released under the GNU General Public License (GPL).
For further information see LICENSE.txt

Initially, it was written by Ingo Bressler (ingo {at} cs . tu-berlin . de) 
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

