#!/bin/bash


FormatFrom[0]='dat' #BCI2000
FormatFrom[1]='vhdr' #brainamp
FormatFrom[2]='csv'
FormatFrom[3]='gdf'
FormatFrom[4]='ov'


FormatTo[0]='csv'
FormatTo[1]='edf'
FormatTo[2]='gdf'
FormatTo[3]='ov'


#get the extension
Extension="${1##*.}";
DestinationExtension="${2##*.}";

#check if the source format is valid
From=false
i=0

while  [ $From == false ]  &&  [ $i -le 4 ]
do
	if [ "$Extension" == "${FormatFrom[$i]}" ]; then
	From=true
	else
	(( i++ ))
	fi 	
done <<< "$From"
echo $From

if [ $From == false ]; then
echo "There is no reader box for the ."$Extension" format in openvibe"
exit 1
fi

#check if the destination format is valid
To=false
j=0

while [ $To == false ] && [ $j -le 3 ]
do
	if [ "$DestinationExtension" == "${FormatTo[$j]}" ]; then
	To=true
	else
	(( j++ ))
	fi	
done <<< "$To"

if [ $To == false ]; then
echo "There is no writer box for the ."$DestinationExtension" format in openvibe"
exit 1
fi



if [ $From ] && [ $To ]; then

#TODO change to match the new folder structure
#ScenarioFolder="../openvibe-scenarios/trunc/share/openvibe-scenarios/conversion/"
ScenarioFolder="../conversion/"
ScenarioToOpen=$Extension"2"$DestinationExtension".xml"


#get the files path to the scenario
export OV_CONVERT_SRC=$1
export OV_CONVERT_DEST=$2

#launch designer
../dist/openvibe-designer.sh --no-gui --no-session-management --play-fast $ScenarioFolder$ScenarioToOpen

fi

