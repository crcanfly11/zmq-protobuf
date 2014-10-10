#pragma once

#include "yt/log/logpriority.h"
#include "yt/log/logdispatcher.h"


#define AC_SET_DEFAULT_LOGGER(pDefalutLogger) \
        ::yt::LogDispatcher::Instance()->SetDefaultLogger(pDefalutLogger)

#define AC_SET_LOGGER(iPriority, pLogger) \
        ::yt::LogDispatcher::Instance()->SetLogger(iPriority, pLogger)


#define AC_LOG(LP, ...) \
        ::yt::LogDispatcher::Instance()->Log(LP, ::yt::FormatLog(::yt::LogDispatcher::Instance()->GetLogger(LP)->GetFormat(), LP, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__))


#define AC_LOG_RETURN(RETURN_VALUE, LP, ...) \
        do { AC_LOG(LP, __VA_ARGS__); return RETURN_VALUE; } while(0)


#define AC_LOG_RETURN_VOID(LP, ...) \
        do { AC_LOG(LP, __VA_ARGS__); return; } while(0)


#ifdef AC_HAS_TRACE
#define _AC_TRACE()            AC_LOG(yt::LP_AC_TRACE, __PRETTY_FUNCTION__)
#define AC_TRACE()            AC_LOG(yt::LP_TRACE, __PRETTY_FUNCTION__)
#else
#define _AC_TRACE()            (void(0))
#define AC_TRACE()            (void(0))
#endif


#ifdef AC_HAS_DEBUG
#define _AC_DEBUG(...)          AC_LOG(yt::LP_AC_DEBUG, __VA_ARGS__)
#define AC_DEBUG(...)          AC_LOG(yt::LP_DEBUG, __VA_ARGS__)
#else
#define _AC_DEBUG(...)          (void(0))
#define AC_DEBUG(...)          (void(0))
#endif


#ifdef AC_HAS_INFO
#define _AC_INFO(...)          AC_LOG(yt::LP_AC_INFO, __VA_ARGS__)
#define AC_INFO(...)          AC_LOG(yt::LP_INFO, __VA_ARGS__)
#else
#define _AC_INFO(...)          (void(0))
#define AC_INFO(...)          (void(0))
#endif


#ifdef AC_HAS_WARNING
#define _AC_WARNING(...)      AC_LOG(yt::LP_AC_WARNING, __VA_ARGS__)
#define AC_WARNING(...)      AC_LOG(yt::LP_WARNING, __VA_ARGS__)
#else
#define _AC_WARNING(...)      (void(0))
#define AC_WARNING(...)      (void(0))
#endif


#ifdef AC_HAS_ERROR
#define _AC_ERROR(...)          AC_LOG(yt::LP_AC_ERROR, __VA_ARGS__)
#define AC_ERROR(...)          AC_LOG(yt::LP_ERROR, __VA_ARGS__)
#else
#define _AC_ERROR(...)          (void(0))
#define AC_ERROR(...)          (void(0))
#endif


#ifdef AC_HAS_CRITICAL
#define _AC_CRITICAL(...)          AC_LOG(yt::LP_AC_CRITICAL, __VA_ARGS__)
#define AC_CRITICAL(...)          AC_LOG(yt::LP_CRITICAL, __VA_ARGS__)
#else
#define _AC_CRITICAL(...)          (void(0))
#define AC_CRITICAL(...)          (void(0))
#endif

extern bool g_writerunlog;
inline void WriteRunLog(){g_writerunlog = true;}
inline void NotWriteRunLog(){g_writerunlog = false;}

#define RUNLOG(...) \
if(g_writerunlog)	AC_LOG(yt::LP_USER1,__VA_ARGS__);


