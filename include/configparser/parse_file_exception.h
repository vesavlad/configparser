#pragma once

#include <exception>
#include <sstream>
#include <string>

namespace configparser
{
    class parse_file_exception : public std::exception
    {
    public:
        explicit parse_file_exception(std::string file);

        const char *what() const noexcept override;

    private:
        std::string _file;
        std::string _msg;
    };
}
