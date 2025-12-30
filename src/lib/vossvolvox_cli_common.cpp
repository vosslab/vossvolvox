#include "vossvolvox_cli_common.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

namespace {

bool g_debug_enabled = false;
bool g_debug_registered = false;
bool g_grid_timer_started = false;
std::chrono::steady_clock::time_point g_grid_start;

void report_debug_timing() {
  if (!g_debug_enabled || !g_grid_timer_started) {
    return;
  }
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = now - g_grid_start;
  std::cerr << "Debug: grid_time_seconds=" << elapsed.count() << std::endl;
}

}  // namespace

namespace vossvolvox {

bool debug_enabled() {
  return g_debug_enabled;
}

void add_filter_options(ArgumentParser& parser, FilterSettings& filters) {
  add_xyzr_filter_flags(parser,
                        filters.use_hydrogens,
                        filters.exclude_ions,
                        filters.exclude_ligands,
                        filters.exclude_hetatm,
                        filters.exclude_water,
                        filters.exclude_nucleic,
                        filters.exclude_amino);
}

void add_output_options(ArgumentParser& parser, OutputSettings& outputs) {
  add_output_file_options(parser, outputs.pdbFile, outputs.ezdFile, outputs.mrcFile);
}

pdbio::ConversionOptions make_conversion_options(const FilterSettings& filters) {
  if (debug_enabled()) {
    std::cerr << "Debug: filters"
              << " use_hydrogens=" << (filters.use_hydrogens ? "true" : "false")
              << " exclude_ions=" << (filters.exclude_ions ? "true" : "false")
              << " exclude_ligands=" << (filters.exclude_ligands ? "true" : "false")
              << " exclude_hetatm=" << (filters.exclude_hetatm ? "true" : "false")
              << " exclude_water=" << (filters.exclude_water ? "true" : "false")
              << " exclude_nucleic=" << (filters.exclude_nucleic ? "true" : "false")
              << " exclude_amino=" << (filters.exclude_amino ? "true" : "false")
              << std::endl;
  }
  pdbio::ConversionOptions options;
  options.use_united = !filters.use_hydrogens;
  options.filters.exclude_ions = filters.exclude_ions;
  options.filters.exclude_ligands = filters.exclude_ligands;
  options.filters.exclude_hetatm = filters.exclude_hetatm;
  options.filters.exclude_water = filters.exclude_water;
  options.filters.exclude_nucleic_acids = filters.exclude_nucleic;
  options.filters.exclude_amino_acids = filters.exclude_amino;
  if (debug_enabled()) {
    std::cerr << "Debug: conversion_options"
              << " use_united=" << (options.use_united ? "true" : "false")
              << " exclude_ions=" << (options.filters.exclude_ions ? "true" : "false")
              << " exclude_ligands=" << (options.filters.exclude_ligands ? "true" : "false")
              << " exclude_hetatm=" << (options.filters.exclude_hetatm ? "true" : "false")
              << " exclude_water=" << (options.filters.exclude_water ? "true" : "false")
              << " exclude_nucleic=" << (options.filters.exclude_nucleic_acids ? "true" : "false")
              << " exclude_amino=" << (options.filters.exclude_amino_acids ? "true" : "false")
              << std::endl;
  }
  return options;
}

void add_debug_option(ArgumentParser& parser, DebugSettings& debug) {
  parser.add_option("",
                    "--debug",
                    debug.debug,
                    false,
                    "Enable debug output (filters, grid state, timing).",
                    "");
}

void enable_debug(const DebugSettings& debug) {
  g_debug_enabled = debug.debug;
  if (g_debug_enabled && !g_debug_registered) {
    std::atexit(report_debug_timing);
    g_debug_registered = true;
  }
}

void debug_report_cli(const std::string& input_label, const OutputSettings* outputs) {
  if (!debug_enabled()) {
    return;
  }
  std::cerr << "Debug: input="
            << (input_label.empty() ? "<none>" : input_label)
            << std::endl;
  if (outputs) {
    std::cerr << "Debug: outputs"
              << " pdb=" << (outputs->pdbFile.empty() ? "<none>" : outputs->pdbFile)
              << " ezd=" << (outputs->ezdFile.empty() ? "<none>" : outputs->ezdFile)
              << " mrc=" << (outputs->mrcFile.empty() ? "<none>" : outputs->mrcFile)
              << " mrc_small=" << (outputs->use_small_mrc ? "true" : "false")
              << std::endl;
  } else {
    std::cerr << "Debug: outputs=<none>" << std::endl;
  }
}

void debug_note_grid_prep_start() {
  if (!debug_enabled() || g_grid_timer_started) {
    return;
  }
  g_grid_timer_started = true;
  g_grid_start = std::chrono::steady_clock::now();
}

}  // namespace vossvolvox
