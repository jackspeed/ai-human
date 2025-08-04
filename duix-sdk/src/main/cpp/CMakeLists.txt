cmake_minimum_required(VERSION 3.13.2)
project(gjmywrt)

#set(CMAKE_CXX_COMPILER g++)
#set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC  -funwind-tables -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC ")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")
set(ORT_NO_EXCEPTIONS FALSE)

#set(DEVAUD false)
option(DEVARM "shared library support" TRUE)

if(DEVARM)
  set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/third/opencv-mobile-4.6.0-android/sdk/native/jni)
  find_package(OpenCV REQUIRED core imgproc highgui)

  set(ncnn_DIR ${CMAKE_SOURCE_DIR}/third/ncnn-20231027-android-shared/${ANDROID_ABI}/lib/cmake/ncnn)
  find_package(ncnn REQUIRED)

  add_library(turbojpeg STATIC IMPORTED)
  set_target_properties(turbojpeg
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/third/arm/${ANDROID_ABI}/libturbojpeg.a)

  add_library(libjpeg STATIC IMPORTED)
  set_target_properties(libjpeg
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/third/arm/${ANDROID_ABI}/libjpeg.a)

  add_library(onnx-lib SHARED IMPORTED)
  set_target_properties(
    onnx-lib
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/third/arm/${ANDROID_ABI}/libonnxruntime.so)
endif()

option(USE_OPENCV "shared library support" TRUE)
option(USE_NCNN "shared library support" TRUE)
option(USE_OPENVINO "shared library support" FALSE)
set(THIRD_INC "third/include")

if(DEVARM)
  set(THIRD_LIB "third/libarm")
else()
  set(THIRD_LIB "third/lib64")
endif()



if(DEVARM)

  include_directories(
    include
    dhcore
    dhmfcc
    aes
    android
    third/arm/include
    third/arm/include/onnx
    third/arm/include/ncnn
    third/arm/include/turbojpeg
  )
else()

  include_directories(
    include
    dhcore
    dhmfcc
    aes
    third2/include
    third2/inc2404
    third2/include/onnx
    third2/include/turbojpeg
    third2/include/ncnn
    /usr/local/include/opencv4
  )

  link_directories(
    ${CMAKE_SOURCE_DIR}/lib64
    ${CMAKE_SOURCE_DIR}/third2/lib64
    ${CMAKE_SOURCE_DIR}/third2/lib2404
    /usr/local/lib
  )
endif()

add_library(dhcore STATIC
  dhcore/dh_mem.c
  dhcore/dh_data.cpp
  dhcore/dh_que.cpp
)

target_link_libraries(dhcore
  -lm -lz -pthread
)





add_library(dhmfcc STATIC
  dhmfcc/dhpcm.cpp
  dhmfcc/dhwenet.cpp
  dhmfcc/wenetai.cpp
  dhmfcc/AudioFFT.cpp
  dhmfcc/iir_filter.cpp
  dhmfcc/mfcc.cpp
)

target_link_libraries(dhmfcc
  dhcore
  -lz -lm 
)

target_compile_options(dhmfcc   PRIVATE
  -std=c++17
)

include_directories(
  include
  dhunet
)

add_library(dhunet STATIC
  dhunet/jmat.cpp
  dhunet/blendgram.cpp
  dhunet/face_utils.cpp
  dhunet/malpha.cpp
  dhunet/munet.cpp
)

target_link_libraries(dhunet
  dhcore
  dhmfcc
  -lz -lm 
)

if(DEVARM)

  add_library(gjduix SHARED
    duix/gjduix.cpp
    duix/gjsimp.cpp
    android/Log.cpp
    android/DuixJni.cpp
    android/JniHelper.cpp
    aes/aes_cbc.c  aes/aes_core.c  aes/aes_ecb.c  aes/base64.c  aes/cbc128.c  aes/gj_aes.c
    aes/aesmain.c
  )

  target_link_libraries(gjduix
    dhcore
    dhmfcc
    dhunet
    ${OpenCV_LIBS}
    ${log-lib}
    ncnn
    onnx-lib
    libjpeg
    turbojpeg
    -lz -lm 
    -landroid
  )

else()
  add_library(gjduix SHARED
    duix/gjduix.cpp
    duix/gjsimp.cpp
  )

  target_link_libraries(gjduix
    dhcore
    dhmfcc
    dhunet
    -ljpeg
    -lopencv_core
    -lopencv_imgproc
    -lopencv_highgui
    -lturbojpeg
    -lonnxruntime
    -lncnn
    -lz -lm 
  )


endif()


add_executable(duixtest
  #iostest/testduix.cpp
  iostest/testsimp.cpp
)

target_link_libraries(duixtest
  dhcore
  gjduix
)

