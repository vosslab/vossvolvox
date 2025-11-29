#include "argument_helper.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace vossvolvox {

namespace {
bool g_quiet_mode = false;
}

bool quiet_mode() {
  return g_quiet_mode;
}

ArgumentParser::ArgumentParser(std::string program_name, std::string description)
    : program_name_(std::move(program_name)), description_(std::move(description)) {
  auto entry = std::make_unique<OptionEntry>();
  entry->short_name = "-q";
  entry->long_name = "--quiet";
  entry->description = "Suppress program banner/citation output.";
  entry->expects_value = false;
  entry->setter = [](const std::string&) { g_quiet_mode = true; };
  register_option(std::move(entry));
}

void ArgumentParser::add_flag(const std::string& short_name,
                              const std::string& long_name,
                              bool& target,
                              bool default_value,
                              std::string description) {
  target = default_value;
  auto entry = std::make_unique<OptionEntry>();
  entry->short_name = short_name;
  entry->long_name = long_name;
  entry->description = std::move(description);
  entry->expects_value = false;
  entry->setter = [&target](const std::string&) { target = true; };
  register_option(std::move(entry));
}

void ArgumentParser::add_example(const std::string& example) {
  examples_.push_back(example);
}

ArgumentParser::ParseResult ArgumentParser::parse(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      print_help();
      return ParseResult::HelpRequested;
    }
    const auto lookup = lookup_.find(arg);
    if (lookup == lookup_.end()) {
      std::cerr << program_name_ << ": unknown option '" << arg << "'\n";
      return ParseResult::Error;
    }
    OptionEntry* option = lookup->second;
    if (option->expects_value) {
      if (i + 1 >= argc) {
        std::cerr << program_name_ << ": missing value for option '" << arg << "'\n";
        return ParseResult::Error;
      }
      const std::string value = argv[++i];
      try {
        option->setter(value);
      } catch (const std::exception& err) {
        std::cerr << program_name_ << ": " << err.what() << "\n";
        return ParseResult::Error;
      }
    } else {
      option->setter({});
    }
  }
  return ParseResult::Ok;
}

void ArgumentParser::print_help(std::ostream& os) const {
  os << "Usage: " << program_name_ << " [options]\n\n";
  if (!description_.empty()) {
    os << description_ << "\n\n";
  }
  os << "Options:\n";
  for (const auto& opt_ptr : options_) {
    const auto& opt = *opt_ptr;
    std::ostringstream names;
    if (!opt.short_name.empty()) {
      names << opt.short_name;
      if (!opt.long_name.empty()) {
        names << ", " << opt.long_name;
      }
    } else if (!opt.long_name.empty()) {
      names << opt.long_name;
    }
    std::string rendered = names.str();
    if (opt.expects_value && !opt.placeholder.empty()) {
      rendered += " " + opt.placeholder;
    }
    os << "  " << std::left << std::setw(18) << rendered << opt.description << "\n";
  }
  if (!examples_.empty()) {
    os << "\nExamples:\n";
    for (const auto& example : examples_) {
      os << "  " << example << "\n";
    }
  }
  os << std::endl;
}

ArgumentParser::OptionEntry* ArgumentParser::register_option(std::unique_ptr<OptionEntry> entry) {
  auto* ptr = entry.get();
  options_.push_back(std::move(entry));
  if (!ptr->short_name.empty()) {
    lookup_[ptr->short_name] = ptr;
  }
  if (!ptr->long_name.empty()) {
    lookup_[ptr->long_name] = ptr;
  }
  return ptr;
}

}  // namespace vossvolvox
