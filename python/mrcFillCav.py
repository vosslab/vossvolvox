#!/usr/bin/env python

import os
import sys
import math
import numpy
from pyami import mrc
from scipy import ndimage
from optparse import OptionParser

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option("-f", "--file", dest="mrcfile",
		help="MRC file", metavar="FILE")
	parser.add_option("-c", "--contour", "--threshold", dest="threshold", default=1.0,
		help="Contour threshold for density", metavar="#", type="float")
	(options, args) = parser.parse_args()

	mrcfile = options.mrcfile
	if not mrcfile or not os.path.isfile(mrcfile):
		parser.print_help()
		
	floatdata = mrc.read(mrcfile)
	header = mrc.readHeaderFromFile(mrcfile)

	#del floatdata

	s = numpy.ones((3,3,3))
	print s
	print "Median filter"
	floatdata = ndimage.median_filter(floatdata, size=3)
	
	print "Closing operation"
	booldata = numpy.array(floatdata > options.threshold, dtype=int)
	closebooldata = ndimage.morphology.binary_closing(booldata, iterations=3)
	floatdata = numpy.where(closebooldata > booldata, floatdata+options.threshold/1.5, floatdata)
	mrc.write(numpy.array(closebooldata, dtype=int), "bool.mrc", header)


	print "Cavity fill 1"
	booldata = numpy.array(floatdata > options.threshold, dtype=int)	
	labeledArray, n = ndimage.measurements.label(booldata, structure=s)
	floatdata = numpy.where(labeledArray >= 2, options.threshold*2, floatdata)

	print "Cavity fill 2"	
	booldata = numpy.array(floatdata < options.threshold, dtype=int)
	labeledArray, n = ndimage.measurements.label(booldata, structure=s)
	floatdata = numpy.where(labeledArray >= 2, options.threshold*2, floatdata)

	#print ndimage.measurements.histogram(labeledArray, 0, labeledArray.max(), labeledArray.max())
	#print labeledArray
	print "shape=", booldata.shape
	print "sum=", booldata.sum()	
	print "Found %d cavities in original map"%(n-1)
	newbooldata = numpy.array(floatdata < options.threshold, dtype=int)
	print "sum=", newbooldata.sum()
	
	labeledArray, n = ndimage.measurements.label(newbooldata)
	print ndimage.measurements.histogram(labeledArray, 0, labeledArray.max(), labeledArray.max())	
	print "Found %d cavities after fill"%(n-1)
	
	root = os.path.splitext(mrcfile)[0]
	newfile = "%s-fill.mrc"%(root)
	mrc.write(floatdata, newfile, header)

	print "new data is in %s"%(newfile)