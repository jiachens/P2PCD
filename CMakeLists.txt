cmake_minimum_required(VERSION 3.10)
#set(CMAKE_TOOLCHAIN_FILE arm_toolchain.cmake)

project(p2pcd_testing)

set(CMAKE_CXX_STANDARD 14)

# env
set(BASE_DIR /home/jiachen/mk5)
set(STACK_DIR ${BASE_DIR}/stack)
set(BOARD mk5)
#set(CROSS_COMPILE arm-linux-gnueabihf-)
#set(ARCH arm)
set(INSTALLDIR ${BASE_DIR}/bsp/image/rootfs)

set(LIB1609_SRC_DIR ${BASE_DIR}/ieee1609/app/lib1609)
set(LIB1609_LDLIB ${LIB1609_SRC_DIR}/lib1609.a)
set(LIB1609_LIB lib1609.a)

set(V2XSEC_DIR ${BASE_DIR}/aerolink/${BOARD})
set(
        V2XSEC_LIBS
        libviicsec.so
        libc2x.so
        libNxDispatcher.so
)
set(SEC_LDLIBS -lviicsec -lc2x -Wl,--no-as-needed -lNxDispatcher -ldl)

set(V2X_LIBS ${LIB1609_LIB} ${V2XSEC_LIBS})
set(LDLIBS ${SEC_LDLIBS} ${LIB1609_LDLIB})

set(LIBS ${V2X_LIBS})
set(LIBS_DIR ${STACK_DIR}/v2x-lib/lib/${BOARD})

set(CMAKE_SYSROOT ${INSTALLDIR})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Target
add_executable(p2pcd_testing main.cpp aerolink_dot2.cpp aerolink_dot2.h utils.cpp utils.h)
add_executable(p2pcd_server server.cpp aerolink_dot2.cpp aerolink_dot2.h utils.cpp utils.h)

# Include
target_include_directories(
        p2pcd_testing
        PUBLIC
        ${BASE_DIR}/stack/v2x-lib/include
        $(INSTALLDIR)/usr/include
        ${BASE_DIR}/ieee1609/kernel/include
        ${BASE_DIR}/ieee1609/kernel/include/linux/ieee1609/dot3
        ${BASE_DIR}/cohda/kernel/include
        ${LIB1609_SRC_DIR}/include
        ${V2XSEC_DIR}/include
)
target_include_directories(
        p2pcd_server
        PUBLIC
        ${BASE_DIR}/stack/v2x-lib/include
        $(INSTALLDIR)/usr/include
        ${BASE_DIR}/ieee1609/kernel/include
        ${BASE_DIR}/ieee1609/kernel/include/linux/ieee1609/dot3
        ${BASE_DIR}/cohda/kernel/include
        ${LIB1609_SRC_DIR}/include
        ${V2XSEC_DIR}/include
)

# Libraries
link_directories(${LIBS_DIR})
message( ${LIBS_DIR} )
find_library(1609_LIB NAMES 1609 HINTS ${LIBS_DIR})
find_library(V2XSEC_LIB NAMES viicsec HINTS ${LIBS_DIR})
find_library(C2X_LIB NAMES c2x HINTS ${LIBS_DIR})
find_library(NXP_LIB NAMES NxDispatcher HINTS ${LIBS_DIR})
find_library(PTHREAD_LIB NAMES pthread)
#find_library(RT_LIB NAMES rt)
#find_library(M_LIB NAMES m)

target_link_libraries(p2pcd_testing ${1609_LIB} ${V2XSEC_LIB} ${C2X_LIB} ${NXP_LIB} -lpthread -lrt -lm)
target_link_libraries(p2pcd_server ${1609_LIB} ${V2XSEC_LIB} ${C2X_LIB} ${NXP_LIB} -lpthread -lrt -lm)

#EXTRA_CFLAGS += -O -g
#CFLAGS += -Wall -Werror -MD


#LDFLAGS += -L$(LIB1609_SRC_DIR) -l1609 \
#-lpthread -lrt -lm
#
#LIBS += $(LIB1609_APP_BIN)
