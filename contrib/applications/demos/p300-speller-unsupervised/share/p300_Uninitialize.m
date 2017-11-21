% p300_Uninitialize.m
% -------------------------------
% Author : Jussi T. Lindgren / Inria
% Date   :
%
function box_out = p300_Uninitialize(box_in)

	clear gnr_ept;
	clear gfeatures;
	clear gflashed;
	clear gsequence;
	clear gC;
	clear gTotal;
	clear gTrials;
	clear gCnt;
	
	rmpath('matlab/helpers');
	rmpath('matlab/EM');	
	rmpath('matlab');
			
	box_out = box_in;
end
    