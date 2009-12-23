#!/bin/bash

# convert all files to consistent line endings via 'flip -u'
#	-v xas_dir="/home/ingo/code/sasfit/trunk/from_cvs/sasfit.vfs/lib/app-sasfit/tcl/x_as/"\
awk \
	-F";" \
	-v xas_dir="/home/ingo/code/qSLDcalc/res/data/raw_xas/"\
	-f convert_csv.awk \
	/home/ingo/code/qSLDcalc/res/data/raw_data

for datafile in *.xml; do
	echo "</chemical_element_list>" >> $datafile;
done;

