#!/usr/bin/python

import os
import sys
import pdblib

if __name__ == "__main__":
	if len(sys.argv) > 1:
		pdbfile = sys.argv[1]
	else:
		print "Usage: splitPDB.py file.pdb"
		sys.exit(1)
	if not os.path.isfile(pdbfile):
		sys.exit(1)
		
	root = os.path.splitext(pdbfile)[0]	
	atomlist = pdblib.pdb2atomdict(pdbfile)
	splitlist = pdblib.splitResTypes(atomlist)
	for restype in splitlist.keys():
		newfile = "%s-%s.pdb"%(root,restype)
		print ("writing %d atoms of type %s to %s"
			%(len(splitlist[restype]), restype, newfile))
		f = open(newfile, "w")
		for atomdict in splitlist[restype]:
			line = pdblib.atomdict2line(atomdict)
			f.write(line+"\n")
		f.close()
	