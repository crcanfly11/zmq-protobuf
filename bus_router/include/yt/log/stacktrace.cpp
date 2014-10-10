#include <stdio.h>
#include <yt/log/stacktrace.h>
#include <yt/log/log.h>
#include <yt/util/tss.h>


namespace yt {

	void __StackTraceCleanup(void* p)
	{
		size_t* pi = static_cast<size_t*>(p);
		delete pi;
	}

	static /*TSS2*/TSS __StackTraceTSS(__StackTraceCleanup);

	StackTrace::StackTrace(const char* file_, size_t line_, const char* func_)
		: file(file_), line(line_), func(func_)
		{
			size_t* indent = static_cast<size_t*>(__StackTraceTSS.get());
			if (!indent)
			{
				indent = new size_t(0);
				__StackTraceTSS.set(indent);
			}

			Print(*indent, "->", file, line, func);

			(*indent) ++;
		}

	StackTrace::~StackTrace()
	{
		size_t* indent = static_cast<size_t*>(__StackTraceTSS.get());
		if (!indent)
		{
			indent = new size_t(0);
			__StackTraceTSS.set(indent);
		}

		(*indent) --;

		Print(*indent, "<-", file, line, func);
	}


	void StackTrace::Print(size_t indent, const char* action, const char* file, size_t line, const char* func)
	{
		char buffer[2048];
		char* p = buffer;
		size_t remain = sizeof(buffer);

		while (indent-- > 0)
		{
			int n = snprintf(p, remain, "  ");
			p += n;
			remain -= n;
		}

		snprintf(p, remain, "[%zx] %s %s(%s:%zu)", (ssize_t)pthread_self(), action, func, file, line);
		AC_LOG(LP_AC_TRACE, buffer);
	}
} // namespace yt

