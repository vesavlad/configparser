#include "configparse/parse_exception.h"

namespace configparser
{
    parse_exception::parse_exception(size_t l, size_t p, std::string exc, std::string f, std::string file) :
            _l(l),
            _p(p),
            _exc(std::move(exc)),
            _f(std::move(f)),
            _file(std::move(file)),
            _msg()
    {
        std::stringstream ss;
        ss << _file << ":" << _l << ", at pos " << _p << ": Expected " << _exc
           << ", found " << _f;
        _msg = ss.str();
    }

    const char *parse_exception::what() const noexcept { return _msg.c_str(); }
}