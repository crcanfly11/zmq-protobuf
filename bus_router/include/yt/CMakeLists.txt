PROJECT(NETUTIL)

#SET(
#SRC_LIST
#log/datefilelogger.cpp
#log/logdispatcher.cpp
#log/logger.cpp
#log/logpriority.cpp
#log/nulllogger.cpp
#log/ostreamlogger.cpp
#log/stacktrace.cpp
#log/udplogger.cpp
#util/allocator.cpp
#util/circle_queue.cpp
#util/data_block.cpp
#util/inifile.cpp
#util/mutex.cpp
#util/notify.cpp
#util/protocolstream.cpp
#util/thread.cpp
#util/threadpool.cpp
#util/tss.cpp
#)

ADD_SUBDIRECTORY(network)

#AUX_SOURCE_DIRECTORY(./crypt SRC_LIST)
AUX_SOURCE_DIRECTORY(./log SRC_LIST)
AUX_SOURCE_DIRECTORY(./util SRC_LIST)
AUX_SOURCE_DIRECTORY(./file SRC_LIST)
AUX_SOURCE_DIRECTORY(./xml SRC_LIST)

SET(CMAKE_CXX_FLAGS "-fPIC")  
SET(LIBRARY_OUTPUT_PATH ../../lib)
ADD_DEFINITIONS(-g -W -Wall -Wno-deprecated -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO -DAC_HAS_WARNING -DAC_HAS_DEBUG -DAC_HAS_ERROR -DAC_HAS_CRITICAL -D_AC_HAS_TRACE -DTIXML_USE_STL)

INCLUDE_DIRECTORIES(./ ../)
LINK_DIRECTORIES()

ADD_LIBRARY(svrutil STATIC ${SRC_LIST})
