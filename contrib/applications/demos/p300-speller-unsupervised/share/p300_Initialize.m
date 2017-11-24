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
	
	% We carry settings in this structure instead of using global variables
	box_in.user_data.flashes_per_trial = box_in.settings(1).value;
	box_in.user_data.max_trials = 15;
	box_in.user_data.keyboard_cols = 7; % @FIXME bad hardcoding	
	box_in.user_data.previous_time = 0;
	box_in.user_data.gfeatures = [];
	box_in.user_data.total_flashes = 0;
	box_in.user_data.total_trials = 0;
	box_in.user_data.C = []; % classifier
	
	fprintf(1,'%d\n', box_in.settings(1).value);
	
	% declare some identifiers
	% OV_stimulations();
	
	box_out = box_in;
		
end
    