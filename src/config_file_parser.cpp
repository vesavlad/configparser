// Copyright 2015 Patrick Brosi
// info@patrickbrosi.de

#include "configparse/config_file_parser.h"

#include "configparse/parse_file_exception.h"
#include "configparse/parse_exception.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <utility>

namespace configparser
{
    enum state: uint8_t
    {
        NONE,
        IN_HEAD,
        IN_INC,
        IN_HEAD_KEY,
        IN_HEAD_AW_COM_OR_END,
        IN_KEY_VAL_KEY,
        IN_KEY_VAL_VAL,
        AW_KEY_VAL_VAL,
        IN_KEY_VAL_VAL_HANG,
        IN_KEY_VAL_VAL_HANG_END
    };

    config_file_parser::config_file_parser() :
            _secs{},
            _kvs{}
    {}

    void config_file_parser::parseStr(const std::string &str)
    {
        std::stringstream ss;
        ss << str;
        parse(ss, "<string literal>");
    }


    void config_file_parser::parse(const std::string &path)
    {
        std::ifstream is(path);

        if (!is.good()) {
            throw parse_file_exception(path);
        }

        parse(is, path);
    }


    void config_file_parser::parse(std::istream &is, const std::string &path)
    {
        state s = NONE;
        std::set<std::string> headers;
        key_vals curKV;
        std::string tmp, tmp2;

        char c;
        size_t l = 1, pos = 0, valLine = 0, valPos = 0;

        while (is.get(c)) {
            pos++;
            // skip comments
            while (c == '#') {
                is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                c = '\n';
            }
            if (c == '\n') {
                l++;
                pos = 1;
            }
            switch (s) {
                case NONE:
                    if (std::isspace(c)) continue;
                    if (c == '[' || c == '<') {
                        if (!tmp.empty()) curKV[tmp] = val{trim(tmp2), valLine, valPos, path};
                        tmp.clear();
                        tmp2.clear();
                        for (const auto& h : headers) {
                            if (_secs.find(h) != _secs.end()) {
                                updateVals(_secs[h], curKV);
                            } else {
                                _secs[h] = _kvs.size();
                                _kvs.push_back(curKV);
                            }
                        }
                        headers.clear();
                        curKV.clear();
                        if (c == '[') s = IN_HEAD;
                        if (c == '<') s = IN_INC;
                        continue;
                    }
                    if (isKeyChar(c)) {
                        if (headers.empty()) headers.insert("");
                        tmp.push_back(c);
                        s = IN_KEY_VAL_KEY;
                        continue;
                    }
                    throw parse_exception(l, pos, "header or key", std::string("'") + c + "'",
                                          path);

                case IN_INC:
                    if (c == '\n') {
                        throw parse_exception(l, pos, ">", "<newline>", path);
                    } else if (c == '>') {
                        parse(relPath(tmp, path));
                        tmp.clear();
                        s = NONE;
                    } else {
                        tmp += c;
                    }
                    continue;

                case IN_HEAD:
                    if (std::isspace(c)) continue;
                    if (isKeyChar(c)) {
                        s = IN_HEAD_KEY;
                        tmp.push_back(c);
                        continue;
                    }
                    throw parse_exception(l, pos, "header name", std::string("'") + c + "'", path);

                case IN_HEAD_KEY:
                    if (isKeyChar(c)) {
                        tmp.push_back(c);
                        continue;
                    }

                    if (std::isspace(c)) {
                        s = IN_HEAD_AW_COM_OR_END;
                        continue;
                    }
                    // fall through here
                case IN_HEAD_AW_COM_OR_END:
                    if (std::isspace(c)) continue;
                    if (c == ',') {
                        headers.insert(tmp);
                        tmp.clear();
                        s = IN_HEAD;
                        continue;
                    }
                    if (c == ']') {
                        s = NONE;
                        headers.insert(tmp);
                        tmp.clear();
                        continue;
                    }
                    throw parse_exception(l, pos, "next header oder header end",
                                   std::string("'") + c + "'", path);
                case IN_KEY_VAL_KEY:
                    if (isKeyChar(c)) {
                        tmp.push_back(c);
                        continue;
                    }

                    if (c == ':' || c == '=') {
                        valLine = l;
                        valPos = pos;
                        s = AW_KEY_VAL_VAL;
                        continue;
                    }

                    throw parse_exception(l, pos, "key/value", std::string("'") + c + "'", path);

                case AW_KEY_VAL_VAL:
                    if (c == '\r' || c == '\n') {
                        s = IN_KEY_VAL_VAL_HANG;
                        continue;
                    }
                    if (std::isspace(c)) continue;
                case IN_KEY_VAL_VAL:
                    s = IN_KEY_VAL_VAL;
                    if (c == '\r' || c == '\n') {
                        s = IN_KEY_VAL_VAL_HANG;
                        continue;
                    }
                    tmp2.push_back(c);
                    continue;
                case IN_KEY_VAL_VAL_HANG:
                    if (c == '\r' || c == '\n') {
                        continue;
                    }
                    if (c == '[') {
                        curKV[tmp] = val{trim(tmp2), valLine, valPos, path};
                        tmp.clear();
                        tmp2.clear();
                        for (const auto& h : headers) {
                            if (_secs.find(h) != _secs.end()) {
                                updateVals(_secs[h], curKV);
                            } else {
                                _secs[h] = _kvs.size();
                                _kvs.push_back(curKV);
                            }
                        }
                        headers.clear();
                        curKV.clear();
                        s = IN_HEAD;
                        continue;
                    }
                    if (isKeyChar(c)) {
                        curKV[tmp] = val{trim(tmp2), valLine, valPos, path};
                        tmp.clear();
                        tmp2.clear();
                        tmp.push_back(c);
                        s = IN_KEY_VAL_KEY;
                        continue;
                    }
                    if (std::isspace(c)) {
                        s = IN_KEY_VAL_VAL_HANG_END;
                        continue;
                    }

                case IN_KEY_VAL_VAL_HANG_END:
                    if (c == '\r' || c == '\n') {
                        s = IN_KEY_VAL_VAL_HANG;
                        continue;
                    }
                    if (std::isspace(c)) continue;
                    if (!tmp2.empty()) tmp2.push_back(' ');
                    tmp2.push_back(c);
                    s = IN_KEY_VAL_VAL;
                    continue;
            }
        }

        if (s != IN_KEY_VAL_VAL && s != NONE && s != IN_KEY_VAL_VAL_HANG &&
            s != IN_KEY_VAL_VAL_HANG_END) {
            throw parse_exception(l, pos, "character", "<EOF>", path);
        }

        if (!tmp.empty() && !tmp2.empty()) curKV[tmp] = val{tmp2, valLine, valPos, path};
        tmp.clear();
        tmp2.clear();
        for (const auto& h : headers) {
            if (_secs.find(h) != _secs.end()) {
                updateVals(_secs[h], curKV);
            } else {
                _secs[h] = _kvs.size();
                _kvs.push_back(curKV);
            }
        }
    }


    const std::string &config_file_parser::getStr(const sec& sec, const key &key) const
    {
        return _kvs[_secs.find(sec)->second].find(key)->second.val;
    }


    int config_file_parser::getInt(const sec& sec, const key &key) const
    {
        return atoi(_kvs[_secs.find(sec)->second].find(key)->second.val.c_str());
    }


    double config_file_parser::getDouble(const sec& sec, const key &key) const
    {
        return atof(_kvs[_secs.find(sec)->second].find(key)->second.val.c_str());
    }


    bool config_file_parser::getBool(const sec& sec, const key &key) const
    {
        return toBool(getStr(sec, key));
    }


    bool config_file_parser::toBool(std::string v)
    {
        std::transform(v.begin(), v.end(), v.begin(), ::tolower);

        if (v == "true") return true;
        if (v == "false") return false;
        if (v == "yes") return true;
        if (v == "no") return false;
        if (v == "enable") return true;
        if (v == "disable") return false;
        if (v == "on") return true;
        if (v == "off") return false;
        if (v == "+") return true;
        if (v == "-") return false;
        if (v == "t") return true;
        if (v == "f") return false;

        return atoi(v.c_str());
    }


    std::vector<std::string> config_file_parser::getStrArr(const sec& sec, const key &key,
                                                           char del) const
    {
        size_t p, l = 0;
        std::string s = getStr(sec, key) + del;
        std::vector<std::string> ret;
        while ((p = s.find(del, l)) != std::string::npos) {
            std::string tk = trim(s.substr(l, p - l));
            if (!tk.empty()) ret.push_back(tk);
            l = p + 1;
        }

        return ret;
    }


    std::vector<int> config_file_parser::getIntArr(const sec& sec, const key &key,
                                                   char del) const
    {
        std::vector<int> ret;
        for (const std::string &s : getStrArr(sec, key, del)) {
            ret.push_back(atoi(s.c_str()));
        }

        return ret;
    }


    std::vector<double> config_file_parser::getDoubleArr(const sec& sec, const key &key,
                                                         char del) const
    {
        std::vector<double> ret;
        for (const std::string &s : getStrArr(sec, key, del)) {
            ret.push_back(atof(s.c_str()));
        }

        return ret;
    }


    std::vector<bool> config_file_parser::getBoolArr(const sec& sec, const key &key,
                                                     char del) const
    {
        std::vector<bool> ret;
        for (const std::string &s : getStrArr(sec, key, del)) {
            ret.push_back(toBool(s));
        }

        return ret;
    }


    bool config_file_parser::isKeyChar(char t)
    {
        return std::isalnum(t) || t == '_' || t == '-' || t == '.';
    }


    std::string config_file_parser::toString() const
    {
        std::stringstream ss;
        for (const auto& i : _secs) {
            ss << std::endl << "[" << i.first << "]" << std::endl << std::endl;
            for (const auto& kv : _kvs[i.second]) {
                ss << "  " << kv.first << " = " << kv.second.val << std::endl;
            }
        }

        return ss.str();
    }


    std::string config_file_parser::trim(const std::string &str)
    {
        size_t s = str.find_first_not_of(" \f\n\r\t\v");
        size_t e = str.find_last_not_of(" \f\n\r\t\v");
        if (s == std::string::npos) return "";
        return str.substr(s, e - s + 1);
    }


    std::unordered_map<std::string, size_t> config_file_parser::getSecs() const
    {
        return _secs;
    }


    const val &config_file_parser::getVal(const sec& section, const key& key) const
    {
        return _kvs[_secs.find(section)->second].find(key)->second;
    }


    const key_vals &config_file_parser::getKeyVals(const sec& section) const
    {
        return _kvs[_secs.find(section)->second];
    }


    bool config_file_parser::hasKey(const sec& section, const key& key) const
    {
        if (_secs.find(section) == _secs.end()) return false;
        if (_kvs[_secs.find(section)->second].find(key) ==
            _kvs[_secs.find(section)->second].end())
            return false;

        return true;
    }


    void config_file_parser::updateVals(size_t sec, const key_vals &kvs)
    {
        for (auto &kv : kvs) {
            _kvs[sec][kv.first] = kv.second;
        }
    }


    std::string config_file_parser::relPath(const std::string &file,
                                            std::string curF)
    {
        // pass absolute paths through unchanged
        if (!file.empty() && file[0] == '/')
            return file;
        curF.erase(std::find(curF.rbegin(), curF.rend(), '/').base(), curF.end());
        if (curF.empty())
            return file;
        return curF + "/" + file;
    }

}