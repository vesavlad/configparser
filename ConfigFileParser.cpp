// Copyright 2015 Patrick Brosi
// info@patrickbrosi.de

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include "./ConfigFileParser.h"

using namespace configparser;

// _____________________________________________________________________________
ConfigFileParser::ConfigFileParser() {}

// _____________________________________________________________________________
void ConfigFileParser::parse(const std::string& path) {
  State s = NONE;
  std::set<std::string> headers;
  KeyVals curKV;
  std::string tmp, tmp2;
  std::ifstream is(path);

  if (!is.good()) {
    throw ParseFileExc(path);
  }

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
        if (c == '[') {
          if (tmp.size()) curKV[tmp] = Val{trim(tmp2), valLine, valPos, path};
          tmp.clear();
          tmp2.clear();
          for (auto h : headers) {
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
          if (!headers.size()) headers.insert("");
          tmp.push_back(c);
          s = IN_KEY_VAL_KEY;
          continue;
        }
        throw ParseExc(l, pos, "header or key", std::string("'") + c + "'",
                       path);
      case IN_HEAD:
        if (std::isspace(c)) continue;
        if (isKeyChar(c)) {
          s = IN_HEAD_KEY;
          tmp.push_back(c);
          continue;
        }
        throw ParseExc(l, pos, "header name", std::string("'") + c + "'", path);

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
        throw ParseExc(l, pos, "next header oder header end",
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

        throw ParseExc(l, pos, "key/value", std::string("'") + c + "'", path);

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
          curKV[tmp] = Val{trim(tmp2), valLine, valPos, path};
          tmp.clear();
          tmp2.clear();
          for (auto h : headers) {
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
          curKV[tmp] = Val{trim(tmp2), valLine, valPos, path};
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
    throw ParseExc(l, pos, "character", "<EOF>", path);
  }

  if (tmp.size() && tmp2.size()) curKV[tmp] = Val{tmp2, valLine, valPos, path};
  tmp.clear();
  tmp2.clear();
  for (auto h : headers) {
    if (_secs.find(h) != _secs.end()) {
      updateVals(_secs[h], curKV);
    } else {
      _secs[h] = _kvs.size();
      _kvs.push_back(curKV);
    }
  }
}

// _____________________________________________________________________________
const std::string& ConfigFileParser::getStr(Sec sec, const Key& key) const {
  return _kvs[_secs.find(sec)->second].find(key)->second.val;
}
// _____________________________________________________________________________
int ConfigFileParser::getInt(Sec sec, const Key& key) const {
  return atoi(_kvs[_secs.find(sec)->second].find(key)->second.val.c_str());
}
// _____________________________________________________________________________
double ConfigFileParser::getDouble(Sec sec, const Key& key) const {
  return atof(_kvs[_secs.find(sec)->second].find(key)->second.val.c_str());
}
// _____________________________________________________________________________
bool ConfigFileParser::getBool(Sec sec, const Key& key) const {
  return toBool(getStr(sec, key));
}

// _____________________________________________________________________________
bool ConfigFileParser::toBool(std::string v) const {
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

// _____________________________________________________________________________
std::vector<std::string> ConfigFileParser::getStrArr(Sec sec, const Key& key,
                                                     char del) const {
  size_t p, l = 0;
  std::string s = getStr(sec, key) + del;
  std::vector<std::string> ret;
  while ((p = s.find(del, l)) != std::string::npos) {
    std::string tk = trim(s.substr(l, p - l));
    if (tk.size()) ret.push_back(tk);
    l = p + 1;
  }

  return ret;
}
// _____________________________________________________________________________
std::vector<int> ConfigFileParser::getIntArr(Sec sec, const Key& key,
                                             char del) const {
  std::vector<int> ret;
  for (const std::string& s : getStrArr(sec, key, del)) {
    ret.push_back(atoi(s.c_str()));
  }

  return ret;
}
// _____________________________________________________________________________
std::vector<double> ConfigFileParser::getDoubleArr(Sec sec, const Key& key,
                                                   char del) const {
  std::vector<double> ret;
  for (const std::string& s : getStrArr(sec, key, del)) {
    ret.push_back(atof(s.c_str()));
  }

  return ret;
}
// _____________________________________________________________________________
std::vector<bool> ConfigFileParser::getBoolArr(Sec sec, const Key& key,
                                               char del) const {
  std::vector<bool> ret;
  for (const std::string& s : getStrArr(sec, key, del)) {
    ret.push_back(toBool(s.c_str()));
  }

  return ret;
}

// _____________________________________________________________________________
bool ConfigFileParser::isKeyChar(char t) const {
  return std::isalnum(t) || t == '_' || t == '-' || t == '.';
}

// _____________________________________________________________________________
std::string ConfigFileParser::toString() const {
  std::stringstream ss;
  for (auto i : _secs) {
    ss << std::endl << "[" << i.first << "]" << std::endl << std::endl;
    for (auto kv : _kvs[i.second]) {
      ss << "  " << kv.first << " = " << kv.second.val << std::endl;
    }
  }

  return ss.str();
}

// _____________________________________________________________________________
std::string ConfigFileParser::trim(const std::string& str) const {
  size_t s = str.find_first_not_of(" \f\n\r\t\v");
  size_t e = str.find_last_not_of(" \f\n\r\t\v");
  if (s == std::string::npos) return "";
  return str.substr(s, e - s + 1);
}

// _____________________________________________________________________________
const std::unordered_map<std::string, size_t> ConfigFileParser::getSecs()
    const {
  return _secs;
}

// _____________________________________________________________________________
const Val& ConfigFileParser::getVal(Sec section, Key key) const {
  return _kvs[_secs.find(section)->second].find(key)->second;
}

// _____________________________________________________________________________
bool ConfigFileParser::hasKey(Sec section, Key key) const {
  if (_secs.find(section) == _secs.end()) return false;
  if (_kvs[_secs.find(section)->second].find(key) ==
      _kvs[_secs.find(section)->second].end())
    return false;

  return true;
}

// _____________________________________________________________________________
void ConfigFileParser::updateVals(size_t sec, const KeyVals& kvs) {
  for (auto& kv : kvs) {
    _kvs[sec][kv.first] = kv.second;
  }
}
