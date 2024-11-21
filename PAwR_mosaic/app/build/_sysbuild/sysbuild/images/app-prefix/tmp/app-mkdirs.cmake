# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app"
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/app"
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix"
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix/tmp"
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix/src/app-stamp"
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix/src"
  "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix/src/app-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix/src/app-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/hlord/Documents/Nordic/ePaper54L/PAwR_mosaic/app/build/_sysbuild/sysbuild/images/app-prefix/src/app-stamp${cfgdir}") # cfgdir has leading slash
endif()
