#!/usr/bin/python

import re
import os
import sys
import numpy
import random
import urllib
import itertools
import numpy.linalg

#http://modbase.compbio.ucsf.edu/allosmod/html/file/restyp.lib
aminoacids = (
'ALA', 'ARG', 'ASN', 'ASP', 'ASX', 'CSH', 'CYS', 'GAP', 'GLN', 'GLU', 'GLX',
'GLY', 'HIS', 'HIS', 'HIS', 'ILE', 'LEU', 'LYS', 'MET', 'PCA', 'PGA', 'PHE',
'PR0', 'PRO', 'PRZ', 'SER', 'THR', 'TRP', 'TYR', 'UNK', 'VAL', 'MSE', 'HYP',
'MSE', 'SEP', 'PTR', 'TYS', 'CGU', 'DAR', 'OCS', 'MEN', 'KCX', 'CZZ', 'CSS',
'MDO', 'MME', 'SMC', 'SCH', 'NEP', 'IAS', 'M3L', 'SAC', 'GMA', 'MLY',
)
aadict = dict((y,x) for x,y in enumerate(aminoacids))
nucleicacids = (
'A','G','C','U','T','DA','DG','DC','DU','DT','ADE','GUA','CYT','URI','THY',
'1MA', '1MG', '5MC', '5MU', 'H2U', 'OMG', 'OMU', 'PSU', 'TM2', 'UR3','ADP',
'ATP', 'ADI', 'GTP', 'GDP',
)
nadict = dict((y,x) for x,y in enumerate(nucleicacids))
solvent = (
'HOH','H2O','OH2','MOH','WAT','CA','CAL','ZN','ZN2','OXY','O2','CMO','CO',
'MG','NA','SOD','CL', 'K','CD','PO4', 'MN', 'AU', 'SO4', 'NH2','ACE','ACT',
'ACY','ACN','MES','CFO','TLA','FMT','PEG',
)
solvdict = dict((y,x) for x,y in enumerate(solvent))
sugars = (
'FUL','BMA','MAN','GAL','FUC','GOL','DGD',
)
sugdict = dict((y,x) for x,y in enumerate(sugars))
ligand = (
'PAR','HEM','HYG','NAG','MRC','PEE','BOG','FES','U10','RTL','PLM','ROP',
'U89','0Z6','CLA','EDO','BCR','BCL','CHO','CLR','CHD','FAD','CYC','LI1',
'PEH','CDL','ANP','MGE','UNL','FMN','NAD',
)
ligdict = dict((y,x) for x,y in enumerate(ligand))

#==========
def getRepresentativeModel(pdbId):
	url = "http://www.ebi.ac.uk/pdbe/nmr/olderado/searchEntry?pdbCode="+pdbId
	data = urllib.urlopen(url)
	getNextLine = False
	modelLine = None
	for line in data:
		sline = line.strip()
		if getNextLine is True:
			modelLine = sline
			break
		if '<td class="leftsubheading" >Most&nbsp;representative&nbsp;model</td>' == sline:
			getNextLine = True
	if modelLine is None:
		print "ERROR: pdb id %s has no entry: %s"%(pdbId, url)
		return -1
	modelLine = re.sub("<[^>]*>","\t", modelLine)
	bits = modelLine.split()
	modelNumber = int(bits[0])
	print "Most representation model is #%d for %s"%(modelNumber, pdbId)
	return modelNumber

#==========
def downloadPDB(pdbId):
	url = "http://www.rcsb.org/pdb/download/downloadFile.do?fileFormat=pdb&compression=NO&structureId="+pdbId
	rawdata = urllib.urlopen(url)
	pdbdata = []
	for line in rawdata:
		pdbdata.append(line)
	return pdbdata

#==========
def pdb2atomdict(pdbfile):
	if not os.path.isfile(pdbfile):
		print "File not exist"
		return None
	f = open(pdbfile, "r")
	atomlist = pdbData2atomdict(f)
	f.close()
	print ("finished %s - found %d atoms"%(pdbfile, len(atomlist)))
	return atomlist

#==========
def getSymmInfo(pdbdata):
	symDicts = []
	symCount = 0
	scaleOp = [numpy.zeros((3,3)), numpy.zeros((3))]
	for line in pdbdata:
		sline = line.strip()
		if sline.startswith("SCALE"):
			mstr = "^SCALE([0-9]) +([\-.0-9]+) +([\-.0-9]+) +([\-.0-9]+) +([\-.0-9]+)"
			m = re.match(mstr, sline)
			if m and m.groups():
				row = int(m.groups()[0])-1
				scaleOp[0][row, 0] = float(m.groups()[1])
				scaleOp[0][row, 1] = float(m.groups()[2])
				scaleOp[0][row, 2] = float(m.groups()[3])
				scaleOp[1][row] = float(m.groups()[4])
		if sline.startswith("CRYST1"):
			mstr = "^CRYST1 +"
			mstr += "([\-.0-9]+) +([\-.0-9]+) +([\-.0-9]+) +"
			mstr += "([.0-9]+) +([.0-9]+) +([.0-9]+) +"
			mstr += "([A-Za-z0-9/ ]+)"
			#print mstr
			m = re.match(mstr, sline)
			if m and m.groups():
				crystDict = {
						'a': float(m.groups()[0]),
						'b': float(m.groups()[1]),	
						'c': float(m.groups()[2]),
						'alpha': float(m.groups()[3]),
						'beta': float(m.groups()[4]),
						'gamma': float(m.groups()[5]),
						'spaceGroup': m.groups()[6:],
					}
				#print crystDict
		if sline.startswith("REMARK 290"):
			subline = sline[13:]
			if subline.startswith("SMTRY"):
				mstr = "^SMTRY([0-9]) +([0-9]+) +([\-.0-9]+) +([\-.0-9]+) +([\-.0-9]+) +([\-.0-9]+)"
				m = re.match(mstr, subline)
				#print subline
				if m and m.groups():
					symNum = int(m.groups()[1])
					if symNum > symCount:
						symCount = symNum
					symDict = {
						'row': int(m.groups()[0]),
						'num': symNum,	
						'rot1': float(m.groups()[2]),
						'rot2': float(m.groups()[3]),
						'rot3': float(m.groups()[4]),
						'shift': float(m.groups()[5]),
					}
					symDicts.append(symDict)
	symOps = []
	for i in range(symCount):
		symNum = i+1
		symOp = [numpy.zeros((3,3)), numpy.zeros((3))]
		for symDict in symDicts:
			if symDict['num'] != symNum:
				continue
			row = symDict['row']-1
			symOp[0][row,:] = [symDict['rot1'], symDict['rot2'], symDict['rot3'], ]
			symOp[1][row] = symDict['shift']
		symOps.append(symOp)
	print symOps
	return crystDict, scaleOp, symOps

#==========
def makeUnitCell(atomlist, symOps):
	numOps = len(symOps)
	atomlistSet = []
	print "Original Atoms %d"%(len(atomlist))	
	for i in range(numOps):
		newatomlist = []
		print symOps[i][1]
		sys.stderr.write("Operation Number %d... "%(i+1))
		for atomdict in atomlist:
			coord = numpy.array([atomdict['x'], atomdict['y'], atomdict['z']])
			newcoord = numpy.dot(symOps[i][0],coord) - symOps[i][1]
			newatomdict = atomdict.copy()
			newatomdict['x'] = newcoord[0]
			newatomdict['y'] = newcoord[1]
			newatomdict['z'] = newcoord[2]
			newatomlist.append(newatomdict)
		atomlistSet.append(newatomlist)
		sys.stderr.write("Total Atoms %d\n"%(len(atomlistSet)))	
	return atomlistSet

#==========
def translateUnitCell(atomlistSet, scaleOp, numCopies=4, maxDist=80):
	model = 0
	print
	fullatomlist = []
	print "Search Distance: %d Angstroms"%(maxDist)
	#print "Original Atoms %d"%(len(atomlist))
	random.shuffle(atomlistSet)
	atomCoordSet = []
	for atomlist in atomlistSet:
		atomCoords = getAtomCoords(atomlist)
		atomCoordSet.append(atomCoords)
	d3list = itertools.product(xrange(-numCopies, numCopies+1), repeat=3)
	fillatomlist = []
	for i,j,k in d3list:
		random.shuffle(atomCoordSet)
		for atomCoords in atomCoordSet:
			shiftOp = numpy.array([i,j,k])
			#sys.stderr.write("Operation (%d,%d,%d)... "%(i,j,k))
			fixAtomCoords = translateAtomList(atomCoords, scaleOp, shiftOp, maxDist)
			if fixAtomCoords is not None:
				sys.stderr.write("#")
				fillatomlist.append(fixAtomCoords)
			else:
				sys.stderr.write(".")
	model = 0
	newatomlist = []
	random.shuffle(fillatomlist)
	for atomCoords in fillatomlist:
		model += 1
		newatomlist = atomCoords2List(atomCoords, atomlistSet[0], model)
		fullatomlist.extend(newatomlist)
	sys.stderr.write("\nTotal Models %d\n"%(model))	
	return fullatomlist

#==========
def getAtomCoords(atomlist):
	coordList = []
	for atomdict in atomlist:
		coord = numpy.array([atomdict['x'], atomdict['y'], atomdict['z']])	
		coordList.append(coord)
	atomCoords = numpy.array(coordList)
	return atomCoords

#==========
def atomCoords2List(atomCoords, atomlist, model=1):
	newatomlist = []
	for i in range(len(atomlist)):
		atomdict = atomlist[i]
		atomcoord = atomCoords[i]
		newatomdict = atomdict.copy()
		newatomdict['x'] = atomcoord[0]
		newatomdict['y'] = atomcoord[1]
		newatomdict['z'] = atomcoord[2]
		newatomdict['model'] = model
		newatomlist.append(newatomdict)
	return newatomlist

#==========
def getBoxSize(atomCoords):
	coordArray = numpy.abs(atomCoords)
	return coordArray.max(0) - coordArray.min(0)

#==========
def centerOfMass(atomlist):
	atomCoords = getAtomCoords(atomlist)
	return atomCoords.mean(0)

#==========
def translateAtomList(atomCoords, scaleOp, shiftOp, maxDist):	
	invScale = numpy.linalg.inv(scaleOp[0])
	check = True
	newAtomCoords = numpy.dot(scaleOp[0], atomCoords.T).T + shiftOp
	fixAtomCoords = numpy.dot(invScale, newAtomCoords.T).T
	centerCoord = fixAtomCoords.mean(0)
	### circle
	#if numpy.linalg.norm(centerCoord) > maxDist:
	#	return None
	### box
	if numpy.max(numpy.abs(centerCoord)) > maxDist:
		return None
	return fixAtomCoords
	
#==========
def pdbData2atomdict(pdbdata):
	atomlist = []
	count = 0
	model = 0
	for line in pdbdata:
		count += 1
		sline = line.strip()
		#if sline.startswith("HETATM"):
		if sline.startswith("MODEL"):
			model += 1
		if sline.startswith("ATOM") or sline.startswith("HETATM"):
			#if count % 10 != 0:
			#	continue
			atomdict = line2atomdict(sline)
			atomdict['model'] = model
			if atomdict is not None:
				atomlist.append(atomdict)
		#if count % 1000 == 0:
		#	print ("read %d lines, found %d atoms"%(count, len(atomlist)))
	print ("read %d lines, found %d atoms, %d models"%(count, len(atomlist), model))
	return atomlist
	
#==========		
def line2atomdict(line):
	#print line
	atomdict = {}
	if line.startswith("ATOM"):
		atomdict['atomtype'] = "ATOM"
	elif line.startswith("HETATM"):
		atomdict['atomtype'] = "HETATM"
	else:
		return None
	atomdict['atomnum'] = int(line[6:11])
	atomdict['atomname'] = line[12:16].strip()
	atomdict['resname'] = line[17:20].strip()
	atomdict['restype'] = getResType(atomdict['resname'])	
	atomdict['chainid'] = line[21]
	atomdict['resnum'] = int(line[22:26])
	atomdict['x'] = float(line[30:38])
	atomdict['y'] = float(line[38:46])
	atomdict['z'] = float(line[46:54])
	atomdict['occupancy'] = float(line[54:60])
	atomdict['tempfactor'] = float(line[60:66])
	atomdict['element'] = line[76:78].strip()
	atomdict2line(atomdict)
	return atomdict

#==========		
def getResType(resname):
	try:
		aadict[resname]
		return 'aminoacid'
	except KeyError:
		pass
	try:
		nadict[resname]
		return 'nucleicacid'
	except KeyError:
		pass
	try:
		solvdict[resname]
		return 'solvent'
	except KeyError:
		pass
	try:
		sugdict[resname]
		return 'sugar'
	except KeyError:
		pass		
	try:
		ligdict[resname]
		return 'ligand'
	except KeyError:
		pass		
	return 'unknown'

#==========	
def countResTypes(atomlist):
	typehist = {}
	for atomdict in atomlist:
		type = atomdict['restype']
		try:
			typehist[type] += 1
		except KeyError:
			typehist[type] = 1
	return typehist

#==========	
def splitResTypes(atomlist):
	splitatomlists = {}
	for atomdict in atomlist:
		type = atomdict['restype']
		try:
			splitatomlists[type].append(atomdict)
		except KeyError:
			splitatomlists[type] = [atomdict]
	return splitatomlists

#==========	
def getModel(atomlist, modelNum):
	modelAtomList = []
	for atomdict in atomlist:
		if atomdict['model'] == modelNum:
			modelAtomList.append(atomdict)
	print "Selected %d of %d atoms for model %d"%(len(modelAtomList), len(atomlist), modelNum)
	return modelAtomList

#==========	
def splitChain(atomlist):
	splitatomlists = {}
	for atomdict in atomlist:
		chain = atomdict['chainid']
		try:
			splitatomlists[chain].append(atomdict)
		except KeyError:
			splitatomlists[chain] = [atomdict]
	return splitatomlists

#==========	
def atomdict2line(atomdict):
	line = ""
	if atomdict['atomtype'] == "ATOM":
		line = "ATOM  "
	elif atomdict['atomtype'] == "HETATM":
		line = "HETATM"
	line += "%5d "%(atomdict['atomnum']%100000)
	line += rightPadString(" %s"%(atomdict['atomname']),5) #imperfect, spacing is weird
	line += "%3s "%(atomdict['resname'])
	line += "%1s"%(atomdict['chainid'])
	line += "%4d    "%(atomdict['resnum'])
	line += "%8.3f"%(atomdict['x'])
	line += "%8.3f"%(atomdict['y'])
	line += "%8.3f"%(atomdict['z'])
	line += "%6.2f"%(atomdict['occupancy'])
	line += "%6.2f"%(atomdict['tempfactor'])
	line += "          %2s"%(atomdict['element'])
	try:
		line += " %1s"%(atomdict['restype'][:5]) #non-standard
	except:
		print atomdict
		print line
		raise
	return line
	
#==========	
def rightPadString(s,n=10,fill=" "):
	n = int(n)
	s = str(s)
	if(len(s) > n):
		return s[:n]
	while(len(s) < n):
		s += fill
	return s

#==========	
def leftPadString(s,n=10,fill=" "):
	n = int(n)
	s = str(s)
	if(len(s) > n):
		return s[:n]
	while(len(s) < n):
		s = fill+s
	return s

#==========	
def writePDB(atomlist, pdbfile, modellist=None):
	f = open(pdbfile, "w")
	print "writing %d atoms to file %s"%(len(atomlist), pdbfile)
	model = None
	for atomdict in atomlist:
		atommodel = atomdict['model']
		if modellist is not None and atommodel not in modellist:
			continue
		if model != atommodel:
			if model is not None:
				f.write("ENDMDL\n")
			model = atommodel
			modelstr = leftPadString("%d"%(model), 5)
			line = "MODEL    %s"%(modelstr)
			f.write(line+"\n")
		line = atomdict2line(atomdict)
		f.write(line+"\n")
	if model is not None:
		f.write("ENDMDL\n")
	f.close()

#==========
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
		print "writing %s atoms to to %s"%(restype, newfile)
		writePDB(splitlist[restype], newfile)


