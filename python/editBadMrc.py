#!/usr/bin/env python

import os
import re
import time
import glob
import random
import subprocess

#===============
def fileSize(filename, msg=False):
	"""
	return file size in bytes
	"""
	if not os.path.isfile(filename):
		return 0
	stats = os.stat(filename)
	size = stats[6]
	return size

#===============
def getImages():
	images = glob.glob("*.mrc")
	print "Found %d images"%(len(images))
	random.shuffle(images)
	return images

#===============
def processImage(image, count):

	if not os.path.isfile(image):
		print count, "-- cannot find file", image
		return False

	if fileSize(image) != 67109888:
		print count, "-- wrong initial size", image
		return False

	inf = open(image, "rb")
	msg = inf.read(15)
	inf.close()

	if not str(msg).startswith('string(2) "50"'):
		print count, "-- no string flag", image
		return False

	#print str(msg)

	newfile = "edit"+image

	if os.path.isfile(newfile) and fileSize(newfile) == 67109873:
		print count, "-- already complete"
		return False

	cmd = "tail -c +16 %s > %s"%(image, newfile)
	proc = subprocess.Popen(cmd, shell=True)
	proc.communicate()

	if fileSize(newfile) != 67109873:
		print count, "-- wrong final size", newfile
		return False
	
	print count, "-- completed successfully -- ", newfile
	return True

#===============
#===============
if __name__ == "__main__":
	images = getImages()
	i = 0
	j = 0
	for image in images:
		i += 1
		time.sleep(1)
		if processImage(image, i) is True:
			time.sleep(0.1)
			j += 1
		#if i > 10:
		#	break
	print "Edited %d of %d files"%(j, i)




