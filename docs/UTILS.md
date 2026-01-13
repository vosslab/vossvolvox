# Utility Function Reference (utils-main.cpp)

This companion document summarizes the major helper routines defined in `src/lib/utils-main.cpp`. The functions are grouped by their responsibilities so you can look up how the grid, file, point, and utility workflows interact without hunting through the implementation.

## Initialization & Limit Management
- `void initGridState(float maxprobe)` - reset derived constants (grid volume, water density), zero the tracking filename, and set wide spanning extrema before loading new input.
- `void finalGridDims(float maxprobe)` - deprecated wrapper that calls `initGridState` in the modern codepath; legacy code still uses the original name.
- `float getIdealGrid()` - find a grid spacing that keeps `DX*DY*DZ` under the legacy 1024*512*512 voxel cap while aligning the dimensions to multiples of four.
- `unsigned int calculateDimension(float min, float max, float grid)` - convert a world-space extent into a grid cell count padded to multiples of four for safe indexing.
- `void assignLimits()` - pad every axis by `MAXPROBE`, compute `DX`, `DY`, `DZ`, the strides `DXY`, `DXYZ`, and `NUMBINS`, then log grid usage plus the ideal spacing.
- `void testLimits(gridpt grid[])` - dump the current bounds, strides, and first/last filled voxels to assist debugging or regression checks.

## Grid Utilities
- `int countGrid(const gridpt grid[])` - return the number of nonzero voxels.
- `void zeroGrid(gridpt grid[])` - allocate (if necessary) and clear every voxel.
- `int copyGridFromTo(const gridpt oldgrid[], gridpt newgrid[])` - thin shim that forwards to `copyGrid()` for backward compatibility.
- `int copyGrid(const gridpt oldgrid[], gridpt newgrid[])` - duplicate a grid while counting filled voxels.
- `void inverseGrid(gridpt grid[])` - flip every occupancy bit to turn accessible areas into excluded ones.
- `int subt_Grids(gridpt biggrid[], gridpt smgrid[])` - subtract the second grid from the first (B AND !S), returning voxels changed.
- `int intersect_Grids(gridpt grid1[], gridpt grid2[])` - keep only voxels set in both grids.
- `int merge_Grids(gridpt grid1[], gridpt grid2[])` - union the two grids.
- `int bounding_box(gridpt grid[], gridpt bbox[])` - fill a tight rectangular volume around the occupied voxels.
- `int fill_cavities(gridpt grid[])` - identify internal cavities by subtracting channels from the inverse access map, then fill them back into the main grid.

## File-Based Readers & Access Grid Builders
- `int read_NumAtoms(char file[])` / `int read_NumAtoms_from_array(const XYZRBuffer&)` - parse XYZR contents (file or in-memory), track min/max coordinates, apply padding for `MAXVDW+MAXPROBE`, and update the globals for the bounding box. Returns the counted atoms (>=3).
- `int fill_AccessGrid_fromFile(...)` / `int fill_AccessGrid_fromArray(...)` - build the accessible grid for a specified probe radius either from disk or from the provided `XYZRBuffer`, log progress, and return the filled voxel count.
- `int get_ExcludeGrid_fromFile(...)` / `int get_ExcludeGrid_fromArray(...)` - build an accessible grid, call `trun_ExcludeGrid_fast()` to contract it into the excluded map, free intermediates, and log the excluded volume.

## Grid Generation / Transformation Helpers
- `void trun_ExcludeGrid(...)` - contract the accessible mask by zeroing voxels adjacent to filled neighbors and respecting probe radius; a slice-based parallel loop updates `EXCgrid`.
- `std::vector<int> computeOffsets(float radius_units)` - precompute flat offsets for a spherical probe (used by the fast truncation path).
- `void trun_ExcludeGrid_fast(...)` - same goal as `trun_ExcludeGrid()` but uses the precomputed offsets to avoid repeated inner loops.
- `void grow_ExcludeGrid(...)` - expand the excluded grid outward from filled voxels that have empty neighbors, using `fill_ExcludeGrid()`.
- `int fill_AccessGrid(const float x, const float y, const float z, const float R, gridpt grid[])` - rasterize a single atom sphere into the grid, returning added voxels.
- `void empty_ExcludeGrid(...)` / `void empty_ExcludeGrid_fast(...)` - clear voxels within a probe radius around a point, either by scanning neighbors or using the offset table.
- `void fill_ExcludeGrid(...)` - fill neighbors around a voxel when expanding excluded regions.
- `int fill_cavities(gridpt grid[])` - (see above) ensures internal cavities are captured by combining `bounding_box()`, `subt_Grids()`, and connectivity helpers.

## Point-Based & Connectivity Helpers
- `float *get_Point(gridpt grid[])` / `int get_GridPoint(gridpt grid[])` - find the first filled voxel and report its coordinates or index.
- `int get_Connected(...)`, `int get_ConnectedRange(...)`, `int get_Connected_Point(...)` - flood-fill from a target point (by coordinates or index) and populate the provided `connect` mask. They log progress and use `MAXLIST` buffers from `utils.h`.
- `bool isNearEdgePoint(...)`, `bool isContainedPoint(...)` - test whether a voxel is close to the boundary of a volume (useful for printed models) by scanning neighbors along each axis.
- `bool isCloseToVector(float radius, int pt)` / `void limitToTunnelArea(float radius, gridpt grid[])` / `float distFromPt(float x, float y, float z)` - compute whether voxels lie within a cylindrical tunnel about a hard-coded vector.
- `bool hasFilledNeighbor(...)`, `bool hasEmptyNeighbor(...)`, `bool hasEmptyNeighbor_Fill(...)` - star/cube neighbor queries used by the exclusion/expansion routines.
- `bool isContainedPoint(...)` / `bool isNearEdgePoint(...)` - heuristics for synthetic modifications such as `makerbot_fill()`.

## Coordinate Conversion & Geometry
- `int ijk2pt(int i, int j, int k)` / `void pt2ijk(...)` - convert between 3D indices and 1D linear arrays, with bounds checking.
- `void pt2xyz(...)` / `int xyz2pt(...)` - convert between voxel indices and physical coordinates for logging and CLI output.
- `float crossSection(const real p, const vector v, const gridpt grid[])` / `float crossSection(const gridpt grid[])` - legacy helpers for stepping a slicing plane through the tunnel (the overloaded version ignores the `p`/`v` inputs and uses defaults).
- `int countEdgePoints(gridpt grid[])`, `float surface_area(gridpt grid[])`, `int classifyEdgePoint(const int pt, gridpt grid[])` - analyze surface voxels with weighted types to compute area metrics.
- `void determine_MinMax(const gridpt grid[], int minmax[])` - scan a grid for its axis-aligned extents; used by `makerbot_fill()` and others.
- `int makerbot_fill(gridpt ingrid[], gridpt outgrid[])` - iteratively move interior voxels from one grid to another until only surface voxels remain visible for printing.

## Logging & String Utilities
- `void padLeft(char a[], int n)` / `void padRight(char a[], int n)` - simple fixed-width padding helpers for console output.
- `void printBar()` - print the standard progress bar used by numerous loops.
- `void printVol(int vox)` / `void printVolCout(int vox)` - format voxel volumes with comma-grouping either to `stderr` or `stdout`.
- `void basename(char str[], char base[])` - extract the filename part from a path string.

## Notes
- All functions rely on the global variables defined in `utils-main.cpp` (`XMIN`, `GRID`, `NUMBINS`, etc.), so run `initGridState()` or `finalGridDims()` plus the reader helpers before invoking the point-based utilities.
- The helper functions described here are not exported via headers-they live entirely in `src/lib/utils-main.cpp` and are used by the toolchain (Volume.exe, Solvent.exe, etc.). Use this file as a single reference point until you refactor the logic into separate modules.
