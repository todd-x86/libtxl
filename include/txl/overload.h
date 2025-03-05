#pragma once

namespace txl
{
    template<class... Visitors>
    struct overload : Visitors...
    {
        using Visitors::operator() ...;
    };

    template<class... Visitors>
    overload(Visitors...) -> overload<Visitors...>;
}
