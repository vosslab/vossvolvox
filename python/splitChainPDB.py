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
	splitlist = pdblib.splitChain(atomlist)
	for chain in splitlist.keys():
		newfile = "%s-%s.pdb"%(root,chain)
		if os.path.isfile(newfile):
			newfile = "%s-%s2.pdb"%(root,chain)
		print "writing %s atoms to to %s"%(chain, newfile)
		f = open(newfile, "w")
		for atomdict in splitlist[chain]:
			line = pdblib.atomdict2line(atomdict)
			f.write(line+"\n")
		f.close()
	
