#!/usr/bin/env python

import re
import os
import sys
import pdblib
from optparse import OptionParser

#====================
#====================
class CrystalClass(object):
	#====================
	#====================
	def __init__(self):
		self.parser = OptionParser()
		self.setupParserOptions()
		self.params = self.convertParserToParams()

	#=====================
	def setupParserOptions(self):
		"""
		set the input parameters
		this function should be rewritten in each program
		"""
		self.parser.set_usage("Usage: %prog [options]")

		self.parser.add_option("-i", "--pdbid", dest="pdbid",
			help="PDB id to create crystal structure", metavar="####")	

		self.parser.add_option("-d", "--maxdist", dest="maxdist", type="int",
			help="Maximum distance to generate crystal", metavar="#")	

		self.parser.add_option("-u", "--unitcells", dest="unitcells", type="int",
			help="Number of unit cells to create to find atoms", metavar="#", default=8)	


	#=======================================
	def checkConflicts(self):
		"""
		put in any additional conflicting parameters
		"""
		if self.params['pdbid'] is None:
			apDisplay.printError("please specify a pdbid, --pdbid")
		return

	#=====================
	def convertParserToParams(self):
		self.parser.disable_interspersed_args()
		(options, args) = self.parser.parse_args()
		if len(args) > 0:
			raise Exception, "Unknown commandline options: "+str(args)
		if len(sys.argv) < 2:
			self.parser.print_help()
			self.parser.error("no options defined")
		self.params = {}
		for i in self.parser.option_list:
			if isinstance(i.dest, str):
				self.params[i.dest] = getattr(options, i.dest)
		return self.params


if __name__ == "__main__":
	cryst = CrystalClass()
	#pdbdata = pdblib.downloadPDB(cryst.params['pdbid'])
	f = open("ice.pdb", "r")
	pdbdata = f.readlines()
	atomlist = pdblib.pdbData2atomdict(pdbdata)
	splitlist = pdblib.splitResTypes(atomlist)
	bioatomlist = []
	if 'nucleicacid' in splitlist:
		bioatomlist.extend(splitlist['nucleicacid'])
	if 'aminoacid' in splitlist:
		bioatomlist.extend(splitlist['aminoacid'])
	if 'solvent' in splitlist:
		bioatomlist.extend(splitlist['solvent'])		
	crystDict, scaleOp, symOps = pdblib.getSymmInfo(pdbdata)
	print scaleOp
	atomlistSet = pdblib.makeUnitCell(bioatomlist, symOps)
	fullatomlist = pdblib.translateUnitCell(atomlistSet, scaleOp, 
		numCopies=cryst.params['unitcells'], maxDist=cryst.params['maxdist'])
	pdbfile = "%s-cryst.pdb"%(cryst.params['pdbid'])
	pdblib.writePDB(fullatomlist, pdbfile)

	models = range(999)
	random.shuffle(models)
	evens = models[0:][::2]
	odds = models[1:][::2]
	pdbfile = "%s-even.pdb"%(cryst.params['pdbid'])
	pdblib.writePDB(fullatomlist, pdbfile, evens)
	pdbfile = "%s-odd.pdb"%(cryst.params['pdbid'])
	pdblib.writePDB(fullatomlist, pdbfile, odds)	

	
	
	
	
	
