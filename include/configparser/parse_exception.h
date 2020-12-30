#pragma once

#include <exception>
#include <sstream>
#include <string>

namespace configparser
{
    class parse_exception : public std::exception
    {
    public:
        explicit parse_exception(size_t l, size_t p, std::string exc, std::string f, std::string file);

        const char *what() const noexcept override;

    private:
        size_t _l;
        size_t _p;
        std::string _exc;
        std::string _f;
        std::string _file;
        std::string _msg;
    };

}
