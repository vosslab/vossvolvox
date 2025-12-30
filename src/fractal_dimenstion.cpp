#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"
#include "vossvolvox_cli_common.hpp"
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
  double probe = 10.0;
  double grid_start = 0.4;
  double grid_end = 0.8;
  double grid_steps = 10.0;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Calculate fractional dimensions across a range of grid spacings.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-p",
                    "--probe",
                    probe,
                    10.0,
                    "Probe radius in Angstroms.",
                    "<probe>");
  parser.add_option("-g1",
                    "--grid-start",
                    grid_start,
                    0.4,
                    "Minimum grid spacing in Angstroms.",
                    "<grid>");
  parser.add_option("-g2",
                    "--grid-end",
                    grid_end,
                    0.8,
                    "Maximum grid spacing in Angstroms.",
                    "<grid>");
  parser.add_option("-gn",
                    "--grid-steps",
                    grid_steps,
                    10.0,
                    "Number of grid steps between g1 and g2.",
                    "<steps>");
  vossvolvox::add_filter_options(parser, filters);
  parser.add_example("./FracDim.exe -i sample.xyzr -p 1.5 -g1 0.4 -g2 0.8 -gn 8");

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

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }

  const auto convert_options = vossvolvox::make_conversion_options(filters);
  XYZRBuffer xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(input_path, convert_options, xyzr_buffer)) {
    return 1;
  }

	double xsum=0, yAsum=0, xyAsum=0, yBsum=0, xyBsum=0, x2sum=0, yA2sum=0, yB2sum=0, N=0;

	//HEADER CHECK
	cerr << "Probe Radius: " << probe << endl;
	cerr << "Input file:   " << input_path << endl;

	//log(1/GRID1) - log(1/GRID2) = constant
	//log(GRID2) - log(GRID1) = constant
	//log(GRID1*A) - log(GRID1) = constant
	//log(GRID1) + log(A) - log(GRID1) = constant
	//log(A) = constant

	//GRID2 = GRID1 * GRIDSTEP ^ NUMGRIDSTEP
	//GRIDSTEP ^ NUMGRIDSTEP = GRID2/GRID1
	//GRIDSTEP = [ GRID2/GRID1 ] ^ ( 1/NUMGRIDSTEP )
	double GRIDSTEP = pow(grid_end/grid_start, 1.0/grid_steps);
	const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
	for (GRID=grid_start; GRID<=grid_end; GRID*=GRIDSTEP) {  
		const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
		    buffers,
		    static_cast<float>(GRID),
		    static_cast<float>(probe),
		    input_path,
		    false);
		const int numatoms = grid_result.total_atoms;

		cerr << "Grid Spacing: " << GRID << endl;
//SLOPE	2.99953	2.10564 0.0
//SLOPE	2.83974	2.12005 0.5
//SLOPE	2.93329	2.08918 1.0
//SLOPE	2.93304	2.04321 1.5
//SLOPE	2.95198	2.03213 3.0
//SLOPE	2.95141	2.02238 4.5
//SLOPE	2.95626	2.02310 6.0
//SLOPE	2.96453	2.02655 7.5
		// ****************************************************
		// STARTING FIRST FILE
		// ****************************************************
		//READ FILE INTO SASGRID
		gridpt *EXCgrid;
		EXCgrid = (gridpt*) std::malloc (NUMBINS);
		if (EXCgrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
		zeroGrid(EXCgrid);
		int voxels;
		if(probe > 0.0) { 
			voxels = get_ExcludeGrid_fromArray(numatoms, probe, xyzr_buffer, EXCgrid);
		} else {
			voxels = fill_AccessGrid_fromArray(numatoms, 0.0f, xyzr_buffer, EXCgrid);
		}

		int edgeVoxels = countEdgePoints(EXCgrid);
//1.5	2.97079	0.999992	2.02719	0.999988 // big run
//1.5	2.75922	-0.999936	2.01569	-0.999939 // weighted

		//RELEASE TEMPGRID
		std::free (EXCgrid);

		//cout << GRID << "\t" << voxels << "\t" << edgeVoxels << endl;
		double x = -1.0*log(GRID);
		double yA = log(double(voxels));
		double yB = log(double(edgeVoxels));
		
		cerr << endl;
		//cout << x << "\t" << yA << "\t" << yB << "\t" << GRID << endl;
		
		double weight = 1.0/x - 1/grid_end + 1e-6;
		xsum += weight*x;
		x2sum += weight*x*x;
		xyAsum += weight*x*yA;
		yAsum += weight*yA;
		yA2sum += weight*yA*yA;
		xyBsum += weight*x*yB;
		yBsum += weight*yB;
		yB2sum += weight*yB*yB;
		N += weight;
	}
	cerr << endl << "Program Completed Sucessfully" << endl << endl;
	double numer, denom;
	numer = ((N * xyAsum) - (xsum * yAsum));
    denom = ((N * x2sum - xsum * xsum)* (N * yA2sum - yAsum * yAsum));	
	double corrA = numer / sqrt(denom);
	numer = ((N * xyBsum) - (xsum * yBsum));
    denom = ((N * x2sum - xsum * xsum)* (N * yB2sum - yBsum * yBsum));	
	double corrB = numer / sqrt(denom);
	double slopeA = (xyAsum - xsum*yAsum/N)/(x2sum - xsum*xsum/N);
	double slopeB = (xyBsum - xsum*yBsum/N)/(x2sum - xsum*xsum/N);
	corrA = fabs(corrA);
	corrB = fabs(corrB);	
	//cout << probe << "\t" << slopeA << "\t" << corrA << "\t" << slopeB << "\t" << corrB << endl;
	cout << probe << "\t" << slopeA << "\t" << slopeB << endl;

	return 0;
};
