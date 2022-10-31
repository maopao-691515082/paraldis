#include "internal.h"

namespace paraldis
{

class ReqParserImpl : public ReqParser
{
    std::list<std::vector<std::string>> cmds_;

    enum Stat
    {
        STAT_BEGIN,
        STAT_FAIL,
        STAT_READING_ARG_COUNT,
        STAT_READING_DOLLOR,
        STAT_READING_ARG_LEN,
        STAT_READING_ARG,
    };

    Stat stat_ = STAT_BEGIN;
    std::vector<std::string> args_;
    int64_t arg_count_;
    int64_t arg_len_;

    std::string buf_;

public:

    virtual void Feed(const char *data, size_t len) override
    {
        if (stat_ == STAT_FAIL)
        {
            return;
        }

        while (len > 0)
        {
            switch (stat_)
            {
                case STAT_BEGIN:
                {
                    if (data[0] != '*')
                    {
                        stat_ = STAT_FAIL;
                        return;
                    }
                    ++ data;
                    -- len;
                    stat_ = STAT_READING_ARG_COUNT;
                    break;
                }
                case STAT_READING_ARG_COUNT:
                {
                    if (!buf_.empty() && buf_.back() == '\r')
                    {
                        if (data[0] != '\n')
                        {
                            stat_ = STAT_FAIL;
                            return;
                        }
                        ++ data;
                        -- len;
                        buf_.resize(buf_.size() - 1);
                        if (!(ParseInt64(buf_, arg_count_) && arg_count_ > 0))
                        {
                            stat_ = STAT_FAIL;
                            return;
                        }
                        buf_.clear();
                        stat_ = STAT_READING_DOLLOR;
                        break;
                    }
                    size_t int_str_len = len;
                    for (size_t i = 0; i < len; ++ i)
                    {
                        if (data[i] == '\r')
                        {
                            int_str_len = i + 1;
                            break;
                        }
                    }
                    buf_.append(data, int_str_len);
                    data += int_str_len;
                    len -= int_str_len;
                    break;
                }
                case STAT_READING_DOLLOR:
                {
                    if (data[0] != '$')
                    {
                        stat_ = STAT_FAIL;
                        return;
                    }
                    ++ data;
                    -- len;
                    stat_ = STAT_READING_ARG_LEN;
                    break;
                }
                case STAT_READING_ARG_LEN:
                {
                    if (!buf_.empty() && buf_.back() == '\r')
                    {
                        if (data[0] != '\n')
                        {
                            stat_ = STAT_FAIL;
                            return;
                        }
                        ++ data;
                        -- len;
                        buf_.resize(buf_.size() - 1);
                        if (!(ParseInt64(buf_, arg_len_) && arg_len_ >= 0))
                        {
                            stat_ = STAT_FAIL;
                            return;
                        }
                        buf_.clear();
                        stat_ = STAT_READING_ARG;
                        break;
                    }
                    size_t int_str_len = len;
                    for (size_t i = 0; i < len; ++ i)
                    {
                        if (data[i] == '\r')
                        {
                            int_str_len = i + 1;
                            break;
                        }
                    }
                    buf_.append(data, int_str_len);
                    data += int_str_len;
                    len -= int_str_len;
                    break;
                }
                case STAT_READING_ARG:
                {
                    size_t need_len = (size_t)arg_len_ + 2 - buf_.size();
                    if (len < need_len)
                    {
                        buf_.append(data, len);
                        data += len;
                        len = 0;
                        break;
                    }
                    buf_.append(data, need_len);
                    data += need_len;
                    len -= need_len;
                    if (!(buf_.at(buf_.size() - 2) == '\r' && buf_.back() == '\n'))
                    {
                        stat_ = STAT_FAIL;
                        return;
                    }
                    buf_.resize(buf_.size() - 2);
                    args_.emplace_back(std::move(buf_));
                    buf_.clear();
                    if (args_.size() < (size_t)arg_count_)
                    {
                        stat_ = STAT_READING_DOLLOR;
                    }
                    else
                    {
                        //transfrom cmd arg to upper
                        for (auto &c : args_.at(0))
                        {
                            c = toupper(c);
                        }
                        cmds_.emplace_back(std::move(args_));
                        args_.clear();
                        stat_ = STAT_BEGIN;
                    }
                    break;
                }
                case STAT_FAIL:
                default:
                {
                    Die("bug");
                }
            }
        }
    }

    virtual bool PopCmd(std::vector<std::string> &args) override
    {
        if (cmds_.empty())
        {
            if (stat_ == STAT_FAIL)
            {
                return false;
            }
            args.clear();   //empty args means no cmd yet
            return true;
        }

        args = std::move(cmds_.front());
        cmds_.pop_front();
        return true;
    }
};

ReqParser::Ptr ReqParser::New()
{
    return ReqParser::Ptr(new ReqParserImpl);
}

}
