# TODO

## utils.cpp
- 2006-06-20: fix `write_ThirdEZD()` and `write_FifthEZD()`.
- 2005-03-02: use a single-bit flag instead of 8-bit bools. See
  [bit operations notes](http://www.cs.umd.edu/class/spring2003/cmsc311/Notes/BitOp/bitI.html).
- 2005-01-27: optimize `trun` grid by doing multiple points at the same time
  (good for `trun_exclude` and `surface_area`).
- 2005-01-27: find surface points in groups rather than individually
  (good for `trun_exclude` and `surface_area`).
- 2004-12-01: write a smarter fill accessible grid function.
