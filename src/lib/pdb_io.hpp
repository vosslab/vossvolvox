#pragma once

#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#ifndef VOSS_HAVE_GEMMI
#  if __has_include(<gemmi/structure.hpp>)
#    define VOSS_HAVE_GEMMI 1
#  else
#    define VOSS_HAVE_GEMMI 0
#  endif
#endif

namespace vossvolvox::pdbio {

struct Filters {
    bool exclude_ions = false;
    bool exclude_ligands = false;
    bool exclude_hetatm = false;
    bool exclude_water = false;
    bool exclude_nucleic_acids = false;
    bool exclude_amino_acids = false;
};

struct ConversionOptions {
    bool use_united = true;
    Filters filters;
};

struct XyzrAtom {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double radius = 0.0;
};

struct XyzrData {
    std::vector<XyzrAtom> atoms;
};

class PdbToXyzrConverter {
   public:
    PdbToXyzrConverter();
    ~PdbToXyzrConverter();

    bool ConvertStream(std::istream& input,
                       const std::string& label,
                       const ConversionOptions& options,
                       std::ostream& output) const;

    bool ConvertFile(const std::string& path,
                     const ConversionOptions& options,
                     std::ostream& output) const;

    bool ConvertStreamToAtoms(std::istream& input,
                              const std::string& label,
                              const ConversionOptions& options,
                              std::vector<XyzrAtom>& atoms) const;

    bool ConvertFileToAtoms(const std::string& path,
                            const ConversionOptions& options,
                            std::vector<XyzrAtom>& atoms) const;

#if VOSS_HAVE_GEMMI
    bool ConvertWithGemmi(const std::string& path,
                          const ConversionOptions& options,
                          std::ostream& output) const;

    bool ConvertWithGemmiToAtoms(const std::string& path,
                                 const ConversionOptions& options,
                                 std::vector<XyzrAtom>& atoms) const;
#endif

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

bool IsMmcifFile(const std::string& path);
bool IsPdbmlFile(const std::string& path);
bool IsXyzrFile(const std::string& path);

bool LoadStructureAsXyzr(const std::string& path,
                         const ConversionOptions& options,
                         std::vector<XyzrAtom>& atoms);

bool ReadFileToXyzr(const std::string& path,
                    const ConversionOptions& options,
                    XyzrData& data);

bool WriteXyzrToFile(const std::string& path, const XyzrData& data);
void WriteXyzrToStream(std::ostream& output, const XyzrData& data);

}  // namespace vossvolvox::pdbio
