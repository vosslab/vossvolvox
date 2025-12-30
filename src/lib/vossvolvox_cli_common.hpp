#pragma once

#include <string>

#include "argument_helper.hpp"
#include "pdb_io.hpp"

namespace vossvolvox {

struct FilterSettings {
  bool use_hydrogens = false;
  bool exclude_ions = false;
  bool exclude_ligands = false;
  bool exclude_hetatm = false;
  bool exclude_water = false;
  bool exclude_nucleic = false;
  bool exclude_amino = false;
};

struct OutputSettings {
  std::string pdbFile;
  std::string ezdFile;
  std::string mrcFile;
  bool use_small_mrc = false;
};

struct DebugSettings {
  bool debug = false;
};

void add_filter_options(ArgumentParser& parser, FilterSettings& filters);
void add_output_options(ArgumentParser& parser, OutputSettings& outputs);
pdbio::ConversionOptions make_conversion_options(const FilterSettings& filters);
void add_debug_option(ArgumentParser& parser, DebugSettings& debug);
void enable_debug(const DebugSettings& debug);
bool debug_enabled();
void debug_report_cli(const std::string& input_label, const OutputSettings* outputs);
void debug_report_grid_state();
void debug_note_grid_prep_start();

}  // namespace vossvolvox
