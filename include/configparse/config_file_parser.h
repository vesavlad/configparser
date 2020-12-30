// Copyright 2015 Patrick Brosi
// info@patrickbrosi.de

#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace configparser
{

    struct val
    {
        std::string val;
        size_t line;
        size_t pos;
        std::string file;
    };

    using key = std::string;
    using sec = std::string;
    using key_vals = std::unordered_map<std::string, val>;

    class config_file_parser
    {
    public:
        config_file_parser();

        ~config_file_parser() = default;

        void parse(const std::string &path);

        void parse(std::istream &is, const std::string &path);

        void parseStr(const std::string &str);

        const std::string &getStr(const sec& sec, const key &key) const;

        int getInt(const sec& sec, const key &key) const;

        double getDouble(const sec& sec, const key &key) const;

        bool getBool(const sec& sec, const key &key) const;

        std::vector<std::string> getStrArr(const sec& sec, const key &key, char del) const;

        std::vector<int> getIntArr(const sec& sec, const key &key, char del) const;

        std::vector<double> getDoubleArr(const sec& sec, const key &key, char del) const;

        std::vector<bool> getBoolArr(const sec& sec, const key &key, char del) const;

        std::string toString() const;

        std::unordered_map<std::string, size_t> getSecs() const;

        bool hasKey(const sec& section, const key& key) const;

        const val &getVal(const sec& section, const key& key) const;

        const key_vals &getKeyVals(const sec& section) const;

    private:
        std::unordered_map<std::string, size_t> _secs;
        std::vector<key_vals> _kvs;

        static bool isKeyChar(char t) ;
        static std::string trim(const std::string &str) ;
        static bool toBool(std::string str) ;
        void updateVals(size_t sec, const key_vals &kvs);
        static std::string relPath(const std::string &file, std::string curFile) ;
    };
}
