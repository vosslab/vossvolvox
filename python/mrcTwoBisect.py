#!/usr/bin/env python

import os
import sys
import math
import numpy
from pyami import mrc
from optparse import OptionParser

if __name__ == "__main__":
	parser = OptionParser()
	parser.add_option("-1", "--file1", dest="mrcfile1",
		help="MRC file 1", metavar="FILE")
	parser.add_option("-2", "--file2", dest="mrcfile2",
		help="MRC file 2", metavar="FILE")		
	parser.add_option("-a", "--axes", dest="axes", default="xyz",
		help="Axis choices, can be combination", metavar="AXES")
	(options, args) = parser.parse_args()


	file1 = options.mrcfile1
	file2 = options.mrcfile2
	if file1 is None or file2 is None:
		print "Usage mrcTwoBisect.py -1 file1.mrc -2 file2.mrc"
		parser.print_help()
		sys.exit(1)
	if not os.path.isfile(file1) or not os.path.isfile(file2):
		print "Usage mrcTwoBisect.py -1 file1.mrc -2 file2.mrc"
		parser.print_help()
		sys.exit(1)

	axes = options.axes
	root1 = os.path.splitext(file1)[0]
	root2 = os.path.splitext(file2)[0]
	
	orig1 = mrc.read(file1)
	orig2 = mrc.read(file2)

	orig = orig1 + orig2

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
		cut_1a = orig1.copy()
		cut_1b = orig1.copy()
		cut_2a = orig2.copy()
		cut_2b = orig2.copy()
		cut_1a[:x_index,:,:] = 0
		cut_1b[x_index:,:,:] = 0
		cut_2a[:x_index,:,:] = 0
		cut_2b[x_index:,:,:] = 0
	elif y_maxcrossarea == maxcrossarea:
		print "Bisecting along Y-axis"
		y_index = y_linedensity.argmax()
		cut_1a = orig1.copy()
		cut_1b = orig1.copy()
		cut_2a = orig2.copy()
		cut_2b = orig2.copy()
		cut_1a[:,:y_index,:] = 0
		cut_1b[:,y_index:,:] = 0
		cut_2a[:,:y_index,:] = 0
		cut_2b[:,y_index:,:] = 0	
	elif z_maxcrossarea == maxcrossarea:
		print "Bisecting along Z-axis"
		z_index = z_linedensity.argmax()
		cut_1a = orig1.copy()
		cut_1b = orig1.copy()
		cut_2a = orig2.copy()
		cut_2b = orig2.copy()
		cut_1a[:,:,:z_index] = 0
		cut_1b[:,:,z_index:] = 0
		cut_2a[:,:,:z_index] = 0
		cut_2b[:,:,z_index:] = 0
	header = mrc.readHeaderFromFile(file1)
	print "Writing %s to file"%(root1+"-top.mrc")
	mrc.write(cut_1a, root1+"-top.mrc", header)
	print "Writing %s to file"%(root1+"-bot.mrc")
	mrc.write(cut_1b, root1+"-bot.mrc", header)
	print "Writing %s to file"%(root2+"-top.mrc")
	mrc.write(cut_2a, root2+"-top.mrc", header)
	print "Writing %s to file"%(root2+"-bot.mrc")
	mrc.write(cut_2b, root2+"-bot.mrc", header)






