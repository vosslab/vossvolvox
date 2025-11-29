#pragma once

#include <functional>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace vossvolvox {

bool quiet_mode();

class ArgumentParser {
  struct OptionEntry {
    std::string short_name;
    std::string long_name;
    std::string description;
    std::string placeholder;
    bool expects_value = false;
    std::function<void(const std::string&)> setter;
  };

 public:
  enum class ParseResult { Ok, HelpRequested, Error };

  ArgumentParser(std::string program_name, std::string description);

  template <typename T>
  void add_option(const std::string& short_name,
                  const std::string& long_name,
                  T& target,
                  const T& default_value,
                  std::string description,
                  std::string placeholder = "<value>") {
    target = default_value;
    auto entry = std::make_unique<OptionEntry>();
    entry->short_name = short_name;
    entry->long_name = long_name;
    entry->description = std::move(description);
    entry->placeholder = std::move(placeholder);
    entry->expects_value = true;
    entry->setter = [&target](const std::string& token) { target = convert_value<T>(token); };
    register_option(std::move(entry));
  }

  void add_flag(const std::string& short_name,
                const std::string& long_name,
                bool& target,
                bool default_value,
                std::string description);

  void add_example(const std::string& example);

  ParseResult parse(int argc, char** argv);

  void print_help(std::ostream& os) const;
  void print_help() const { print_help(std::cerr); }

 private:
  template <typename T>
  static T convert_value(const std::string& token) {
    T value{};
    std::istringstream stream(token);
    if (!(stream >> value)) {
      throw std::runtime_error("Unable to parse value: " + token);
    }
    return value;
  }

  OptionEntry* register_option(std::unique_ptr<OptionEntry> entry);

  std::vector<std::unique_ptr<OptionEntry>> options_;
  std::unordered_map<std::string, OptionEntry*> lookup_;
  std::vector<std::string> examples_;
  std::string program_name_;
  std::string description_;
};

template <>
inline std::string ArgumentParser::convert_value<std::string>(const std::string& token) {
  return token;
}

struct XYZROptions {
  std::string input;
  std::string pdb_output;
  std::string ezd_output;
  std::string mrc_output;
};

inline void add_input_option(ArgumentParser& parser, std::string& target) {
  parser.add_option("-i",
                    "--input",
                    target,
                    std::string(),
                    "Input XYZR file (required).",
                    "<XYZR file>");
}

inline void add_pdb_option(ArgumentParser& parser, std::string& target) {
  parser.add_option("-o",
                    "--pdb-output",
                    target,
                    std::string(),
                    "Write accessible surface points to this PDB file.",
                    "<PDB file>");
}

inline void add_ezd_option(ArgumentParser& parser, std::string& target) {
  parser.add_option("-e",
                    "--ezd-output",
                    target,
                    std::string(),
                    "Write excluded density to this EZD file.",
                    "<EZD file>");
}

inline void add_mrc_option(ArgumentParser& parser, std::string& target) {
  parser.add_option("-m",
                    "--mrc-output",
                    target,
                    std::string(),
                    "Write excluded density to this MRC file.",
                    "<MRC file>");
}

inline bool ensure_input_present(const std::string& input, const ArgumentParser& parser) {
  if (!input.empty()) {
    return true;
  }
  parser.print_help();
  std::cerr << "Error: input XYZR file not specified. Use -i <XYZR file>.\n";
  return false;
}

}  // namespace vossvolvox
