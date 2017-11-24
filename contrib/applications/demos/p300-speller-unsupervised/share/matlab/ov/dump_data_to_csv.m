%
%
% CSV export datasets downloaded from
%
% https://zenodo.org/record/192684
%
% for initial testing of the P300 in OV
%

clear;

% !! Set your data path !!
your_data_path = '';

% Change to the current folder and add required paths to the Matlab repository
addpath('EM')
addpath('helpers')
	
% Write openvibe readable CSV files for testing
haveNonEpoched=true;
if(haveNonEpoched)
	rawdata = load('S1_cnt_mrk.mat');
	exportcsv('../signals-old/01-raw-data.csv',[(0:(size(rawdata.cnt.x,1)-1))'/rawdata.cnt.fs,rawdata.cnt.x],false);
	stims = repmat(32779,[size(rawdata.mrk.time,2) 1]);
	featTime = rawdata.mrk.time'/1000;
	exportcsv('../signals-old/01-raw-stims.csv',[featTime,stims,zeros(size(featTime,1),1)],true);		
	exportcsv('../signals-old/02-stimuli.csv',[featTime,rawdata.mrk.stimuli'],true);
	exportcsv('../signals-old/03-sequence.csv',[featTime,rawdata.mrk.sequence'],true);	
else
	load(fullfile(your_data_path,'S1.mat'));
	load(fullfile(your_data_path,'sequence.mat'));

	fv = proc_jumpingMeans(epo,[50 120; 121 200; 201 280;281 380;381 530; 531 700]);

	data = reshape(fv.x,31*6,12852)';

	stimuli = epo.stimuli;

	flashDuration = 0.1;
	featTime = (0:(size(data,1)-1))'*flashDuration;

	%exportcsv20('../signals-new/01-epoched-data.csv',data,featTime, false);
	%exportcsv20('../signals-new/02-stimuli.csv',stimuli',featTime, true);
	%exportcsv20('../signals-new/03-sequence.csv',sequence,featTime, true);
	exportcsv('../signals-old/01-epoched-data.csv',[featTime,data]);
	exportcsv('../signals-old/02-stimuli.csv',[featTime,stimuli']);
	exportcsv('../signals-old/03-sequence.csv',[featTime,sequence]);
end
	
