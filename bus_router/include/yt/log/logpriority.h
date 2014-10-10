#pragma once

#include <string>

namespace yt
{

enum LogPriority
{
        /// \brief 基础级别。 此级别一般不用。
        LP_BASE          =  1,

        //=============================================================================


        /*! \name ac内部日志级别
        */
        // \{
        
        /// \brief 跟踪级别。用在函数的进入/退出时做记录用，一般只会用在前期的debug版。\n
        /// 生成release版时一般要将此级别的信息忽略 (使用NullLogger处理)
        LP_AC_TRACE        = LP_BASE << 1,          

        /// \brief 调试级别。程序输出debug信息时用。\n
        /// 生成release版时一般要将此级别的信息忽略 (使用NullLogger处理)
        LP_AC_DEBUG       = LP_BASE << 2,

        /// \brief 普通级别。记录一般性的非错信息
        LP_AC_INFO          = LP_BASE << 3,

        /// \brief 警告信息。
        LP_AC_WARNING   =  LP_BASE << 4,

        /// \brief 普通错误。大部分错误信息都用此级别记录。
        LP_AC_ERROR       =   LP_BASE << 5,

        /// \brief 严重错误。只在系统层发生严重错误时，才用此级别。例如出现硬件故障的情况。
        LP_AC_CRITICAL  =   LP_BASE << 6,


        // \}

        //=============================================================================

        /*! \name 用户级日志级别
        */
        // \{

        
        /// \brief 跟踪级别。用在函数的进入/退出时做记录用，一般只会用在前期的debug版。\n
        /// 生成release版时一般要将此级别的信息忽略 (使用NullLogger处理)
        LP_TRACE        = LP_BASE << 7,          

        /// \brief 调试级别。程序输出debug信息时用。\n
        /// 生成release版时一般要将此级别的信息忽略 (使用NullLogger处理)
        LP_DEBUG       = LP_BASE << 8,

        /// \brief 普通级别。记录一般性的非错信息
        LP_INFO          = LP_BASE << 9,

        /// \brief 用户级别1。级别说明的最终解释权归应用程序所有。
        LP_USER1        = LP_BASE << 10,

        /// \brief 用户级别2。级别说明的最终解释权归应用程序所有。
        LP_USER2        = LP_BASE << 11,

        /// \brief 警告信息。
        LP_WARNING   =  LP_BASE << 12,

        /// \brief 普通错误。大部分错误信息都用此级别记录。
        LP_ERROR       =   LP_BASE << 13,

        /// \brief 严重错误。只在系统层发生严重错误时，才用此级别。例如出现硬件故障的情况。
        LP_CRITICAL  =   LP_BASE << 14,
		
		LP_STANDARD  =   LP_BASE << 15,

        // \}

};

std::string FormatLog(const std::string & sFormat, LogPriority iLogPriority, const char * szFile, int iLine, const char * szFunction, const char * szFmt, ...);

}

