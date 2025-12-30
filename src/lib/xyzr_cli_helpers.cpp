#include "xyzr_cli_helpers.h"

#include "vossvolvox_cli_common.hpp"

#include "utils.h"

#include <cstring>
#include <iostream>

namespace vossvolvox {

void debug_report_grid_state() {
  if (!debug_enabled()) {
    return;
  }
  std::cerr << "Debug: grid_state"
            << " GRID=" << GRID
            << " GRIDVOL=" << GRIDVOL
            << " MAXPROBE=" << MAXPROBE
            << " WATER_RES=" << WATER_RES
            << " NUMBINS=" << NUMBINS
            << " DX=" << DX
            << " DY=" << DY
            << " DZ=" << DZ
            << " XMIN=" << XMIN
            << " YMIN=" << YMIN
            << " ZMIN=" << ZMIN
            << " XMAX=" << XMAX
            << " YMAX=" << YMAX
            << " ZMAX=" << ZMAX
            << " XYZRFILE=" << XYZRFILE
            << std::endl;
}

bool load_xyzr_or_exit(const std::string& path,
                       const vossvolvox::pdbio::ConversionOptions& opts,
                       XYZRBuffer& out) {
  vossvolvox::pdbio::XyzrData data;
  if (!vossvolvox::pdbio::ReadFileToXyzr(path, opts, data)) {
    std::cerr << "Error: unable to load XYZR data from '" << path << "'\n";
    return false;
  }

  out.atoms.clear();
  out.atoms.reserve(data.atoms.size());
  for (const auto& atom : data.atoms) {
    out.atoms.push_back(XYZRAtom{static_cast<float>(atom.x),
                                 static_cast<float>(atom.y),
                                 static_cast<float>(atom.z),
                                 static_cast<float>(atom.radius)});
  }
  return true;
}

GridPrepResult prepare_grid_from_xyzr(const std::vector<const XYZRBuffer*>& buffers,
                                      float grid_spacing,
                                      float max_probe,
                                      const std::string& input_label,
                                      bool debug_limits) {
  GRID = grid_spacing;
  initGridState(max_probe);

  if (!input_label.empty()) {
    std::strncpy(XYZRFILE, input_label.c_str(), sizeof(XYZRFILE));
    XYZRFILE[sizeof(XYZRFILE) - 1] = '\0';
  }

  GridPrepResult result;
  result.per_input.reserve(buffers.size());
  for (const auto* buffer : buffers) {
    if (!buffer) {
      result.per_input.push_back(0);
      continue;
    }
    const int atoms = read_NumAtoms_from_array(*buffer);
    result.per_input.push_back(atoms);
    result.total_atoms += atoms;
  }

  assignLimits();
  debug_note_grid_prep_start();
  debug_report_grid_state();
  (void)debug_limits;
  return result;
}

}  // namespace vossvolvox
