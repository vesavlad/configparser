//
// Created by Vesa Vlad on 30.12.2020.
//

#include "configparse/parse_file_exception.h"

namespace configparser
{
    parse_file_exception::parse_file_exception(std::string file) :
            _file(std::move(file))
    {
        std::stringstream ss;
        ss << _file << ": Could not open file.";
        _msg = ss.str();
    }

    const char *parse_file_exception::what() const noexcept { return _msg.c_str(); }
}