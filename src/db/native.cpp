#include "ppgo.h"

#pragma ppgo define-THIS_MOD

namespace ppgo
{

namespace PPGO_THIS_MOD
{

::ppgo::Exc::Ptr cls_VSlotKeyStyle::method_f64_rank_num(
    ::std::tuple<::ppgo::tp_int> &ret, ::ppgo::tp_f64 score)
{
    if (score == 0)
    {
        std::get<0>(ret) = 0;
        return nullptr;
    }

    bool neg = score < 0;
    if (neg)
    {
        score = -score;
    }

    ::ppgo::tp_int n;
    static_assert(sizeof(n) == sizeof(score), "");
    memcpy(&n, &score, sizeof(n));
    if (neg)
    {
        n = ~n;
    }
    std::get<0>(ret) = n;
    return nullptr;
}

::ppgo::Exc::Ptr cls_VSlotKeyStyle::method_f64_from_rank_num(
    ::std::tuple<::ppgo::tp_f64> &ret, ::ppgo::tp_int n)
{
    if (n == 0)
    {
        std::get<0>(ret) = 0.0;
        return nullptr;
    }

    bool neg = n < 0;
    if (neg)
    {
        n = ~n;
    }

    ::ppgo::tp_f64 score;
    static_assert(sizeof(score) == sizeof(n), "");
    memcpy(&score, &n, sizeof(n));
    if (neg)
    {
        score = -score;
    }
    std::get<0>(ret) = score;
    return nullptr;
}

}

}
