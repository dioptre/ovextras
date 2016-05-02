#!/bin/bash
#
# This script tries to locate boxes in scenarios which need to be updated
#
# Assumes openvibe is compiled and installed to dist/, and that we are in test/
#

SYSTEM=`uname -o`
if [ $SYSTEM == "Cygwin" ]; then
  EXT="cmd"
else
  EXT="sh"
fi

pushd .. >/dev/null

echo Scenarios requiring update:
find -iname "*xml" | grep "box-tutorials/\|test/\|bci-examples/" | grep -v dist | while read FN; do
	dist/openvibe-designer.$EXT --no-session-management --no-pause --no-gui --open "$FN" | grep -q -E ".*WARNING.*Scenario requires.*update.*box"
	if [ $? == 0 ]; then
		echo $FN
	fi
done

popd >/dev/null

