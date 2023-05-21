# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/mrmagellan/esp/esp-idf/components/bootloader/subproject"
  "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader"
  "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix"
  "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix/tmp"
  "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix/src/bootloader-stamp"
  "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix/src"
  "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/mrmagellan/Desktop/ASE/final-project/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
