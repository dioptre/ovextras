% Initialize.m
% -------------------------------
% Author : Jussi T. Lindgren / Inria
% Date   : 
%
%

function box_out = p300_Initialize(box_in)
		
%	profile on;
	
	addpath('matlab');
	addpath('matlab/helpers');
	addpath('matlab/EM');
		
	global gnr_ept;
	
	gnr_ept = 68;

	box_out = box_in;
		
end
    