#!/usr/bin/env python

import os
import sys
import math
import numpy
from pyami import mrc
from optparse import OptionParser

if __name__ == "__main__":
	parser = OptionParser()
	parser.add_option("-f", "--file", dest="mrcfile",
		help="MRC file", metavar="FILE")
	parser.add_option("-a", "--axes", dest="axes", default="xyz",
		help="Axis choices, can be combination", metavar="AXES")
	(options, args) = parser.parse_args()

	filename = options.mrcfile
	if filename is None or not os.path.isfile(filename):
		print "Usage mrcBisect.py -f file.mrc <options>"
		parser.print_help()
		sys.exit(1)
	axes = options.axes
	orig = mrc.read(filename)
	rootname = os.path.splitext(filename)[0]

	x_linedensity = orig.sum(1).sum(1)
	y_linedensity = orig.sum(0).sum(1)
	z_linedensity = orig.sum(0).sum(0)

	if 'x' in axes:
		x_maxcrossarea = x_linedensity.max()
		print "X-axis has max area of %d"%(x_maxcrossarea)
	else:
		x_maxcrossarea = 0
	if 'y' in axes:
		y_maxcrossarea = y_linedensity.max()
		print "Y-axis has max area of %d"%(y_maxcrossarea)
	else:
		y_maxcrossarea = 0
	if 'z' in axes:
		z_maxcrossarea = z_linedensity.max()
		print "Z-axis has max area of %d"%(z_maxcrossarea)
	else:
		z_maxcrossarea = 0
	maxcrossarea = max(x_maxcrossarea,y_maxcrossarea,z_maxcrossarea)

	if x_maxcrossarea == maxcrossarea:
		print "Bisecting along X-axis"
		x_index = x_linedensity.argmax()
		cut_1 = orig.copy()
		cut_2 = orig.copy()
		cut_1[:x_index,:,:] = 0
		cut_2[x_index:,:,:] = 0
	elif y_maxcrossarea == maxcrossarea:
		print "Bisecting along Y-axis"
		y_index = y_linedensity.argmax()
		cut_1 = orig.copy()
		cut_2 = orig.copy()
		cut_1[:,:y_index,:] = 0
		cut_2[:,y_index:,:] = 0
	elif z_maxcrossarea == maxcrossarea:
		print "Bisecting along Z-axis"
		z_index = z_linedensity.argmax()
		cut_1 = orig.copy()
		cut_2 = orig.copy()
		cut_1[:,:,:z_index] = 0
		cut_2[:,:,z_index:] = 0

	header = mrc.readHeaderFromFile(filename)
	print "Writing %s to file"%(rootname+"-top.mrc")
	mrc.write(cut_1, rootname+"-top.mrc", header)
	print "Writing %s to file"%(rootname+"-bot.mrc")
	mrc.write(cut_2, rootname+"-bot.mrc", header)






