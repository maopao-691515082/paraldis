#include "internal.h"

namespace paraldis
{

int64_t NowMS()
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    return (int64_t)now.tv_sec * 1000 + (int64_t)now.tv_usec / 1000;
}

std::string Sprintf(const char *fmt, ...)
{
    static thread_local char buf[4096];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return buf;
}

static std::mutex log_lock;

void Log(const std::string &msg)
{
    time_t ts = (time_t)(NowMS() / 1000);

    struct tm tm_r;
    localtime_r(&ts, &tm_r);

    char log_tm[32];
    strftime(log_tm, sizeof(log_tm), "[%Y-%m-%d %H:%M:%S]", &tm_r);

    std::lock_guard<std::mutex> lg(log_lock);
    std::cerr << log_tm << " " << msg << std::endl;
}

void Die(const std::string &msg)
{
    std::cerr << "Die: " << msg << std::endl;
    _exit(1);
}

bool ParseInt64(const std::string &s, int64_t &v)
{
    if (!s.empty() && !isspace(s[0]))
    {
        const char *p = s.c_str();
        size_t len = s.size();
        const char *end_ptr;
        errno = 0;
        long long n = strtoll(p, (char **)&end_ptr, 10);
        if (*end_ptr == '\0' && end_ptr == p + len && errno == 0)
        {
            v = n;
            return true;
        }
    }
    return false;
}

}
