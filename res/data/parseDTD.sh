#!/bin/bash
# parseDTD.sh
# A specialized DTD parser for generating C/C++ structure declarations
#
# based on:
# bparser 0.7e (JAN2006) by George Makrydakis <gmakmail at gmail.com>
# http://www.linuxfromscratch.org/pipermail/alfs-discuss/2006-January/007537.html
# license: GPL v3
#
# May 2010, Ingo Bressler (ingo@wollsau.de)

#
# conversion of old dtd&xml files:
# sed -e 's/neutron_scattering_length/nsc_length/g' test.xml -e 's/neutron_scattering_cross_section/nsc_xs/g' -e 's/xray_scattering_anomalous_coefficients/xsc_acoeff/g' | less
#

declare -a xmlSTAT		# contains the status informer
declare -a xmlDATA		# contains any relevant data (element/attributes name/values and unparsed data)
declare -a attMember
declare -i attIdx=0		# global xINDEX pointer for xml**** arrays
declare    elemMember
declare -i checkpoint=0		# a character counter & pointer for parserLINE
declare -i doall=0		# a simple counter variable
declare -i nLine=0		# a simple counter variable
declare -r myfile="$1"	# the filename / path of the file to "parse"

declare -r startTAG='<'		# literal <
declare -r closeTAG='>'		# literal >
declare -r prefixTAG='!'	# literal !
declare -r startElemTAG='('	# literal (
declare -r closeElemTAG=')'	# literal )
declare -r quoteTAG='"'		# literal "
declare elmentNAME=""		# element name value
declare elmentDUMP=""		# element name dump

declare parserLINE=""		# contains a single line read from the XML document sent to the parser
declare parserFLAG="ENABLED"	# can have two mutually exclusive values: ENABLED / DISABLD
declare elemFLAG="DISABLD"
declare attFLAG="DISABLD"

function invalidChar {
	[ "$1" == ' ' ] || \
	[ "$1" == $'\t' ] || \
	[ "$1" == $'\r' ];
}

function trim {
	input="$1"
	declare -i idx=0
	declare -i len=${#input}
	result=""
	while invalidChar "${input:$idx:1}" || \
		[ "${input:$idx:1}" == '"' ] || \
		[ $idx == $len ]; do let "idx++"; done
	until invalidChar "${input:$idx:1}" || \
		[ "${input:$idx:1}" == '"' ] || \
		[ $idx == $len ]; 
	do
		result="$result${input:$idx:1}"
		let "idx++"
	done
	echo -n $result
}

function format_datatype {
	printf "%-15s" "$1"
}

while read parserLINE
do
	for ((checkpoint=0; checkpoint < ${#parserLINE}; checkpoint++)) ;
	do
		case ${parserLINE:$checkpoint:1} in
		$startTAG)
			let "checkpoint++"; elmentNAME=""; parserFLAG="ENABLED"

			if [ "${parserLINE:$checkpoint:1}" != "$prefixTAG" ]; then
				echo "ERROR: no '$prefixTAG' after '$startTAG'!"
				echo "(line $nLine)"
				exit 1
			fi;
			let "checkpoint++"

			# ignore comments <!-- -->
			if [ "${parserLINE:$checkpoint:2}" == '--' ]; then continue; fi;
			elmentDUMP="";
			until [ "${parserLINE:$checkpoint:1}" == ' ' ];
			do
				elmentDUMP=$elmentDUMP${parserLINE:$checkpoint:1}
				let "checkpoint++"
			done
			# skip spaces
			until [ "${parserLINE:$checkpoint:1}" != ' ' ]; do let "checkpoint++"; done
			until invalidChar "${parserLINE:$checkpoint:1}" || \
				[ "${parserLINE:$checkpoint:1}" == '>' ] || \
				[ "${parserLINE:$checkpoint:1}" == '(' ] || \
				  [ $checkpoint = ${#parserLINE} ];
			do
				elmentNAME=$elmentNAME${parserLINE:$checkpoint:1}
				let "checkpoint++"
			done
			if [ "$elmentDUMP" == "ELEMENT" ]; then
				elemMember=""
			elif [ "$elmentDUMP" == "ATTLIST" ]; then
				attFLAG=ENABLED
				echo
				echo "typedef struct {"
				if [ "$elmentNAME" != "ev" ]; then
					echo "    $(format_datatype "bool") valid;"
				fi;
			else
				echo "ERROR: tag type '$elmentDUMP' neither 'ELEMENT' nor 'ATTLIST'!"
				echo "(line $nLine)"
				exit 1
			fi;
		;;
		$closeTAG)
			case $parserFLAG in
			ENABLED)
				let "checkpoint+=2"
			;;
			DISABLD)
			;;
			esac
			if [ "$attFLAG" == "ENABLED" ]; then
				attFLAG=DISABLD
				if [ $((${#attMember[@]}%3)) -ne 0 ]; then
					echo "ERROR: #Attributes not multiple of 3!"
					echo "(line $nLine)"
				fi;
				datatype=$(format_datatype "double")
				# define a single attribute field
				for ((i=0; i < $((${#attMember[@]}/3)); i++));
				do
					line=""
					case "${attMember[$(($i*3))]}" in
					"type")
						case "${attMember[$(($i*3+2))]}" in
						"integer")
							datatype=$(format_datatype "int")
							;;
						"text")
							datatype=$(format_datatype "const char *")
							;;
						*)	
							;;
						esac
						;;
					"re"|"im")
						if [ "${attMember[$((($i+1)*3))]}" == "im" ]; then
							line="    $(format_datatype "complex") val;"
						fi;
						;;
					"unit"|"symbol")
						datatype=$(format_datatype "const char *")
						line="    $datatype ${attMember[$(($i*3))]};"
						;;
					*)
						line="    $datatype ${attMember[$(($i*3))]};"
						;;
					esac
					if [ -n "$line" ]; then
						echo "$line"
					fi;
				done
				# add custom structures
				for member in $elemMember;
				do
					echo "    $(format_datatype "${member}_t") $member;"
				done
				echo "} ${elmentNAME}_t;"
				attMember=()
				attIdx=0
				elemMember=""
			fi;
		;;
		$startElemTAG)
			elemFLAG="ENABLED"
		;;
		$closeElemTAG)
			let "checkpoint++";
			if [ "$elemFLAG" == "ENABLED" ]; then
				elemFLAG="DISABLD"
			fi;
		;;
		*)
			# skip spaces
			until [ "${parserLINE:$checkpoint:1}" != ' ' ]; do 
				let "checkpoint++"; done
			memberNAME=""
			until invalidChar "${parserLINE:$checkpoint:1}" || \
				  [ "${parserLINE:$checkpoint:1}" == ')' ] || \
				  [ "${parserLINE:$checkpoint:1}" == '?' ] || \
				  [ "${parserLINE:$checkpoint:1}" == '*' ] || \
				  [ "${parserLINE:$checkpoint:1}" == ',' ] || \
				  [ $checkpoint = ${#parserLINE} ];
			do
				memberNAME=$memberNAME${parserLINE:$checkpoint:1}
				let "checkpoint++"
			done
			if [ "$memberNAME" != "" ]; then
				if [ "$elemFLAG" == "ENABLED" ]; then
					elemMember="$elemMember $memberNAME"
				elif [ "$attFLAG" == "ENABLED" ]; then
					attMember[attIdx]=$(trim $memberNAME)
					let "attIdx++"
				fi;
			fi;
		;;
		esac
		
	done
	let "nLine++"
done < "$myfile"


