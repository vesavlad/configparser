// Copyright 2015 Patrick Brosi
// info@patrickbrosi.de

#pragma once

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace configparser {

struct Val {
  std::string val;
  size_t line;
  size_t pos;
  std::string file;
};

typedef std::string Key;
typedef std::string Sec;
typedef std::unordered_map<std::string, Val> KeyVals;

enum State {
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

class ParseFileExc : public std::exception {
 public:
  ParseFileExc(const std::string file) : _file(file) {
    std::stringstream ss;
    ss << _file << ": Could not open file.";
    _msg = ss.str();
  };
  virtual const char* what() const throw() { return _msg.c_str(); }

 private:
  std::string _file, _msg;
};

class ParseExc : public std::exception {
 public:
  ParseExc(size_t l, size_t p, std::string exc, std::string f, std::string file)
      : _l(l), _p(p), _exc(exc), _f(f), _file(file), _msg() {
    std::stringstream ss;
    ss << _file << ":" << _l << ", at pos " << _p << ": Expected " << _exc
       << ", found " << _f;
    _msg = ss.str();
  };
  virtual const char* what() const throw() { return _msg.c_str(); }

 private:
  size_t _l;
  size_t _p;
  std::string _exc, _f, _file, _msg;
};

class ConfigFileParser {
 public:
  ConfigFileParser();
  void parse(const std::string& path);

  const std::string& getStr(Sec sec, const Key& key) const;
  int getInt(Sec sec, const Key& key) const;
  double getDouble(Sec sec, const Key& key) const;
  bool getBool(Sec sec, const Key& key) const;

  std::vector<std::string> getStrArr(Sec sec, const Key& key, char del) const;
  std::vector<int> getIntArr(Sec sec, const Key& key, char del) const;
  std::vector<double> getDoubleArr(Sec sec, const Key& key, char del) const;
  std::vector<bool> getBoolArr(Sec sec, const Key& key, char del) const;

  std::string toString() const;

  const std::unordered_map<std::string, size_t> getSecs() const;

  bool hasKey(Sec section, Key key) const;

  const Val& getVal(Sec section, Key key) const;
  const KeyVals& getKeyVals(Sec section) const;

 private:
  std::unordered_map<std::string, size_t> _secs;
  std::vector<KeyVals> _kvs;

  bool isKeyChar(char t) const;
  std::string trim(const std::string& str) const;
  bool toBool(std::string str) const;

  void updateVals(size_t sec, const KeyVals& kvs);
};
}
