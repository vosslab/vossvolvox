INSTALL.txt - Voss Volume Voxelator Package

Prerequisites

Before you begin, make sure you have the following:

- C++17 Compiler: The package requires a C++17-capable compiler (e.g., g++ 7+ or clang++).
- Make: The make utility is required for building the package.

Installation Steps

1. Download or Clone the Repository
   - If you have not already, clone or download the repository to your local machine.
   - Use the following command to clone the repository:
     git clone https://github.com/vosslab/vossvolvox.git

2. Navigate to the Source Code Directory
   - Open a terminal and navigate to the directory containing the source code. For example:
     cd /path/to/vossvolvox/src

3. Compile the Package
   - In the terminal, type the following command to compile the package:
     make
   - This will build the necessary executables from the source code.

4. Check the Output
   - After compilation, you should see the output executables in the /path/to/vossvolvox/bin/ directory. You can list the files in the bin folder:
     ls /path/to/vossvolvox/bin/

5. Run the Programs
   - To test the programs, you can run any of the compiled executables. For example:
     ./bin/pdb_to_xyzr.exe --help   # new converter with filtering options
     ./bin/Volume.exe -h            # main volume tool
     ./bin/Volume.exe -i 1A01.pdb --exclude-ions --exclude-water -p 1.5 -g 0.5  # direct PDB input with filters

See the QUICKSTART file for example usage.
