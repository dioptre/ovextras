
Unsupervised P300 speller 
=========================
Status: Very experimental at the moment. Not well tested.

Relying almost fully on the work of David Hübner, 
Thibault Verhoeven, Pieter-Jan Kindermans, Michael
Tangermann and others(*). Many thanks to the original authors.

For original code, see https://github.com/DavidHuebner/Unsupervised-BCI-Matlab

*) These original authors are not responsible for any issues introduced by the OpenViBE integration 


What is it
----------
These scenarios provide a P300 speller without explicit training phase. Simply focus on 
the letters of your choice (but not the hash marks) and the used machine learning technique 
should catch on after a while. Typically you should notice the predictions start 
to improve after 5 letters or so, but it may also take longer.

Most people will be interested to run the scenario 'unsupervised-p300-online-1.xml'. The
other scenarios are intended solely for testing purposes and may require supplementary files.


Requirements
------------
Matlab at the moment.

Note that P300 can be very time sensitive, its recommended to minimize background processes on the
computer, always run OpenViBE release build outside Visual Studio, and set the Power
Options to 'Performance'. 


Design notes
------------

* The flash groups and sequences (type 1 or type 2) are read from CSV files. To allow the user
to choose the flash timings, these files are used like queues: flash 1 gets the group
and sequence of line 1 in those files, flash 2 gets associated with line 2, and so on. As
OV doesn't support 'fifo queues' at the moment, this is a bit of a hack done in a way
that those files have all feature vectors in them tagged to be sent very early, and the
receiving box simply pops when needed.

* This speller is 'multiletter' meaning that it can correct previous letters. This is why
you see the previously spelled text change during the spelling. Each prediction from the 
classifier is expected to be a stimulation set with markers 'target','row','col','row','col',...'nontarget'
which is then mapped to multiple letters. The last letter is highlighted in red in the grid.

* The timeline and target letter sequence is defined in most scenarios by .lua, 
except in those test scripts that rely purely on canned CSV data.

* The speller is currently limited to a buffer of 15 last letters to keep the computation
delays acceptable.

* Many parameters are presently hardcoded. The code could use lots of polish, and the
matlab parts should be ported to C++.

* Software tagging is currently used. The Visualizer II forwards the flash event markers
to Acquisition Server (using TCP Tagging) after each flash render.


Glossary
--------
What David & co. call 'stimuli' in their code are called 'Flash Groups' in the OV scenarios;
The set of flash time event markers are called 'Timeline'. 'Sequence' is called 'Group types'.


How to test & debug
-------------------

* See the scenarios named like unsupervised-p300-test*xml. Required data files can be imported 
from the format used by David Huber & co. ( https://zenodo.org/record/192684 ) by the script
'ov/dump_data_to_csv.m'. Using the S1.mat file from the aforementioned url should get
the 'epoched' test scenarios to print 'franzy_jagt_im'... etc.


 - J.T. Lindgren 29.Nov.2017
 
