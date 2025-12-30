#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"
#include "xyzr_cli_helpers.h"

extern float XMIN, YMIN, ZMIN;
extern float XMAX, YMAX, ZMAX;
extern int DX, DY, DZ;
extern int DXY, DXYZ;
extern unsigned int NUMBINS;
extern float MAXPROBE;
extern float GRID;
extern float GRIDVOL;
extern float WATER_RES;
extern float CUTOFF;
extern char XYZRFILE[256];

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  std::string pdb_file;
  std::string ezd_file;
  std::string mrc_file;
  double BIGPROBE = 9.0;
  double SMPROBE = 1.5;
  double TRIMPROBE = 4.0;
  int MINSIZE = 20;
  double minvol = 0.0;
  double minperc = 0.0;
  float grid = GRID;
  bool use_hydrogens = false;
  bool exclude_ions = false;
  bool exclude_ligands = false;
  bool exclude_hetatm = false;
  bool exclude_water = false;
  bool exclude_nucleic = false;
  bool exclude_amino = false;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Extract all solvent channels from a structure above a cutoff.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-b",
                    "--big-probe",
                    BIGPROBE,
                    9.0,
                    "Probe radius for large probe (default 9.0 A).",
                    "<big probe>");
  parser.add_option("-s",
                    "--small-probe",
                    SMPROBE,
                    1.5,
                    "Probe radius for small probe (default 1.5 A).",
                    "<small probe>");
  parser.add_option("-t",
                    "--trim-probe",
                    TRIMPROBE,
                    4.0,
                    "Probe radius for trimming exterior solvent (default 4.0 A).",
                    "<trim probe>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing (default auto).",
                    "<grid spacing>");
  parser.add_option("-v",
                    "--min-volume",
                    minvol,
                    0.0,
                    "Minimum channel volume in A^3.",
                    "<min volume>");
  parser.add_option("-p",
                    "--min-percent",
                    minperc,
                    0.0,
                    "Minimum percentage of volume for inclusion (e.g., 0.01 for 1%).",
                    "<fraction>");
  vossvolvox::add_output_file_options(parser, pdb_file, ezd_file, mrc_file);
  vossvolvox::add_xyzr_filter_flags(parser,
                                    use_hydrogens,
                                    exclude_ions,
                                    exclude_ligands,
                                    exclude_hetatm,
                                    exclude_water,
                                    exclude_nucleic,
                                    exclude_amino);
  parser.add_example(
      "./AllChannelExc.exe -i 3hdi.xyzr -b 9.0 -s 1.5 -g 0.5 -t 4.0 -v 5000 -p 0.01");

  const auto parse_result = parser.parse(argc, argv);
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
    return 0;
  }
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
    return 1;
  }
  if (!vossvolvox::ensure_input_present(input_path, parser)) {
    return 1;
  }

  GRID = grid;

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }
  if (!pdb_file.empty() || !ezd_file.empty()) {
    std::cerr << "Warning: PDB/EZD outputs are not supported for this tool; ignoring.\n";
  }

  vossvolvox::pdbio::ConversionOptions convert_options;
  convert_options.use_united = !use_hydrogens;
  convert_options.filters.exclude_ions = exclude_ions;
  convert_options.filters.exclude_ligands = exclude_ligands;
  convert_options.filters.exclude_hetatm = exclude_hetatm;
  convert_options.filters.exclude_water = exclude_water;
  convert_options.filters.exclude_nucleic_acids = exclude_nucleic;
  convert_options.filters.exclude_amino_acids = exclude_amino;
  XYZRBuffer xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(input_path, convert_options, xyzr_buffer)) {
    return 1;
  }

  if (minvol > 0) {
    MINSIZE = int(minvol / GRIDVOL);
  } else if (minperc == 0) {
    minperc = 0.01;
  }

  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      GRID,
      static_cast<float>(BIGPROBE),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

  //HEADER CHECK
  cerr << "Probe Radius: " << BIGPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;
  cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

  // ****************************************************
  // STARTING LARGE PROBE
  // ****************************************************
  gridpt *biggrid;
  biggrid = (gridpt*) std::malloc (NUMBINS);
  if (biggrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(biggrid);
  int bigvox;
  if(BIGPROBE > 0.0) {
    bigvox = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, biggrid);
  } else {
    cerr << "BIGPROBE <= 0" << endl;
    return 1;
  }

  if (minperc > 0) {
    while (minperc > 1)
      minperc /= 100;
    MINSIZE = int(bigvox*minperc);
  }
  cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

  // ****************************************************
  // TRIM LARGE PROBE SURFACE
  // ****************************************************
  gridpt *trimgrid;
  trimgrid = (gridpt*) std::malloc (NUMBINS);
  if (trimgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(biggrid,trimgrid);
  trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  std::free (biggrid);

  //cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

  // ****************************************************
  // STARTING SMALL PROBE
  // ****************************************************
  gridpt *smgrid;
  smgrid = (gridpt*) std::malloc (NUMBINS);
  if (smgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(smgrid);
  int smvox;
  smvox = fill_AccessGrid_fromArray(numatoms, SMPROBE, xyzr_buffer, smgrid);

  // ****************************************************
  // GETTING ACCESSIBLE CHANNELS
  // ****************************************************
  gridpt *solventACC;
  solventACC = (gridpt*) std::malloc (NUMBINS);
  if (solventACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(trimgrid, solventACC); //copy trimgrid into solventACC
  subt_Grids(solventACC, smgrid); //modify solventACC
  std::free (smgrid);


// ***************************************************
// CALCULATE TOTAL SOLVENT
// ***************************************************
  gridpt *solventEXC;
  solventEXC = (gridpt*) std::malloc (NUMBINS);
  if (solventEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  grow_ExcludeGrid(SMPROBE, solventACC, solventEXC);
  intersect_Grids(solventEXC, trimgrid); //modifies solventEXC
  const std::string solvent_output = mrc_file.empty() ? "allsolvent.mrc" : mrc_file;
  writeMRCFile(solventEXC, const_cast<char*>(solvent_output.c_str()));
  std::free (solventACC);

// ***************************************************
// SELECT PARTICULAR CHANNEL
// ***************************************************

  // initialize channel volume
  gridpt *channelEXC;
  channelEXC = (gridpt*) std::malloc (NUMBINS);
  if (channelEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  int solventEXCvol = countGrid(solventEXC);
  //main channel loop
  int numchannels=0;
  int allchannels=0;
  int connected;
  int maxvox=0, minvox=1000000, goodminvox=1000000;
  cerr << "MIN SIZE: " << MINSIZE << " voxels" << endl;
  cerr << "MIN SIZE: " << MINSIZE*GRIDVOL << " Angstroms" << endl;

  while ( countGrid(solventEXC) > MINSIZE ) {
    allchannels++;

    // get channel volume
    zeroGrid(channelEXC); // initialize channelEXC volume
    int gp = get_GridPoint(solventEXC);  //modify x,y,z
    connected = get_Connected_Point(solventEXC, channelEXC, gp); //modify channelEXC
    subt_Grids(solventEXC, channelEXC); //modify solventEXC

    // ***************************************************
    // GETTING CONTACT CHANNEL
    // ***************************************************
    int channelEXCvol = countGrid(channelEXC); // initialize channelEXC volume
    if (channelEXCvol > maxvox)
      maxvox = channelEXCvol;
    if (channelEXCvol < minvox and channelEXCvol > 0)
      minvox = channelEXCvol;
    if (channelEXCvol <= MINSIZE) {
      cerr << "SKIPPING CHANNEL" << endl;
      cerr << "---------------------------------------------" << endl;
      continue;
    }
    if (channelEXCvol < goodminvox)
      goodminvox = channelEXCvol;

    numchannels++;

    // ***************************************************
    // OUTPUT RESULTS
    // ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    printVolCout(channelEXCvol);
    long double surf = surface_area(channelEXC);
    cout << "\t" << surf << "\t" << flush;
    cout << "\t#" << input_path << endl;
    char channel_file[64];
    std::snprintf(channel_file, sizeof(channel_file), "channel-%03d.mrc", numchannels);
    writeSmallMRCFile(channelEXC, channel_file);
    cerr << "---------------------------------------------" << endl;
  }
  if (numchannels == 0)
    goodminvox = 0;
  cerr << "Channel min size: " << int(minvox*GRIDVOL) << " A (all) " << int(goodminvox*GRIDVOL) << " A (good)" << endl;
  cerr << "Channel max size: " << int(maxvox*GRIDVOL) << " A " << endl;
  cerr << "Used " << numchannels << " of " << allchannels << " channels" << endl;
  if (allchannels > 0) {
    cerr << "Mean size: " << solventEXCvol/float(allchannels)*GRIDVOL << " A " << endl;
  }
  cerr << "Cutoff size: " << MINSIZE << " voxels :: "<< MINSIZE*GRIDVOL << " Angstroms" << endl;
  std::free (channelEXC);
  std::free (solventEXC);
  std::free (trimgrid);
  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
