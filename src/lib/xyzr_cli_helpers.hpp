#pragma once

#include <string>
#include <vector>

#include "pdb_io.hpp"
#include "utils.hpp"

namespace vossvolvox {

struct GridPrepResult {
  int total_atoms = 0;
  std::vector<int> per_input;
};

bool load_xyzr_or_exit(const std::string& path,
                       const vossvolvox::pdbio::ConversionOptions& opts,
                       XYZRBuffer& out);

GridPrepResult prepare_grid_from_xyzr(const std::vector<const XYZRBuffer*>& buffers,
                                      float grid_spacing,
                                      float max_probe,
                                      const std::string& input_label,
                                      bool debug_limits);

}  // namespace vossvolvox
