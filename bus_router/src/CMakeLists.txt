PROJECT(bus)
ADD_SUBDIRECTORY(proxy)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
#AUX_SOURCE_DIRECTORY(./ SRC_LIST)
AUX_SOURCE_DIRECTORY(./router SRC_LIST)
SET(EXECUTABLE_OUTPUT_PATH ../bin/)

ADD_DEFINITIONS(-g -W -Wall -Wno-deprecated -D_REENTRANT
-D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO -DAC_HAS_DEBUG -DAC_HAS_WARNING
-DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL -DTHREAD)

INCLUDE_DIRECTORIES(./ ../include /usr/local/include
/usr/local/include/c-client-src)

LINK_DIRECTORIES( ../lib /usr/local/lib /usr/local/protobuf/lib
/usr/local/zeromq/lib )

ADD_EXECUTABLE(bus_router ${SRC_LIST})

TARGET_LINK_LIBRARIES(bus_router pthread protobuf svrutil zmq zookeeper_mt)
