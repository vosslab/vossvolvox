#!/usr/bin/env python

import os
import sys
import math
import numpy
from pyami import mrc
from optparse import OptionParser

def getCutoff(linedensity):
	#cutoff = linedensity.max()*0.6
	#cutoff = linedensity.mean() + linedensity.std()/2.
	cutoff = numpy.median(linedensity) - linedensity.std()/3.
	return cutoff

def getPercentCut(linedensity, percentcut):
	cutoff = getCutoff(linedensity)
	if percentcut < 1e-3:
		mini = len(linedensity)
		maxi = 0
		for i in range(len(linedensity)):
			if linedensity[i] > cutoff and i < mini:
				mini = i
			i2 = len(linedensity)-i-1
			if linedensity[i2] > cutoff and i2 > maxi:
				maxi = i2
	else:
		mini = len(linedensity)*percentcut
		maxi = len(linedensity)*(1-percentcut)
	return maxi, mini

if __name__ == "__main__":
	parser = OptionParser()
	parser.add_option("-f", "--file", dest="mrcfile",
		help="MRC file", metavar="FILE")
	parser.add_option("-a", "--axes", dest="axes", default="xyz",
		help="Axis choices, can be combination, default=xyz", metavar="AXES")
	parser.add_option("-d", "--direction", dest="direction", default="both",
		help="Axis direction choices, can be min, max or both, default=both", metavar="AXES")
	parser.add_option("-p", "--percentcut", dest="percentcut", default=-1,
		help="Percent to cut, -1 = auto", metavar="#", type="float")		
	(options, args) = parser.parse_args()

	filename = options.mrcfile
	if filename is None or not os.path.isfile(filename):
		print "Usage mrcBisect.py -f file.mrc <options>"
		parser.print_help()
		sys.exit(1)
	percentcut = options.percentcut
	axes = options.axes
	direction = options.direction

	print "Axes", axes

	orig = mrc.read(filename)
	rootname = os.path.splitext(filename)[0]

	#print orig.sum(1)

	vols = []
	areas = []
	data = []
	label = []

	#x-axis: flatten y, then flatten z
	if 'x' in axes:
		print "checking X-axis"
		linedensity = orig.sum(1).sum(1)
		maxi, mini = getPercentCut(linedensity, percentcut)
		xmin = orig.copy()
		xmin[:mini,:,:] = 0
		if direction != 'max':
			label.append('xmin')
			areas.append(linedensity[mini])
			data.append(xmin)
		xmax = orig.copy()
		xmax[maxi:,:,:] = 0
		if direction != 'min':
			label.append('xmax')
			areas.append(linedensity[maxi])
			data.append(xmax)

	#y-axis: flatten x, then flatten z
	if 'y' in axes:
		print "checking Y-axis"
		linedensity = orig.sum(0).sum(1)
		maxi, mini = getPercentCut(linedensity, percentcut)
		ymin = orig.copy()
		ymin[:,:mini,:] = 0
		areas.append(linedensity[mini])
		if direction != 'max':
			label.append('ymin')
			areas.append(linedensity[mini])
			data.append(ymin)
		ymax = orig.copy()
		ymax[:,maxi:,:] = 0
		if direction != 'min':
			label.append('ymax')
			areas.append(linedensity[maxi])
			data.append(ymax)

	#z-axis: flatten x, then flatten y
	if 'z' in axes:
		print "checking Z-axis"
		linedensity = orig.sum(0).sum(0)
		maxi, mini = getPercentCut(linedensity, percentcut)
		zmin = orig.copy()
		zmin[:,:,:mini] = 0
		areas.append(linedensity[mini])
		if direction != 'max':
			label.append('zmin')
			areas.append(linedensity[mini])
			data.append(zmin)
		zmax = orig.copy()
		zmax[:,:,maxi:] = 0
		areas.append(linedensity[maxi])
		if direction != 'min':
			label.append('zmax')
			areas.append(linedensity[maxi])
			data.append(zmax)

	vols = []
	scores = []
	#print "ymin: per:%.3f area:%d vol:%d"%(mini/float(orig.shape[1]))
	for i in range(len(data)):
		vol = data[i].sum()
		vols.append(vol)
		area = areas[i]
		linarea = math.sqrt(float(area))
		linvol = float(vol)**(1/3.)
		#score = linarea*linvol
		score = math.sqrt(vol*area)
		scores.append(score)
		#percut = area
		#print label, areas, vols, scores
		try:
			print "%s: area:%d  vol:%d  score:%d"%(label[i], areas[i], vols[i], scores[i])
		except:
			print i, label, areas, vols, scores
	ratios = numpy.array(scores, dtype=numpy.float)
	m = numpy.argmax(ratios)
	print "Writing %s to file"%(label[m])
	header = mrc.readHeaderFromFile(filename)
	mrc.write(data[m], rootname+"-trim.mrc", header)






