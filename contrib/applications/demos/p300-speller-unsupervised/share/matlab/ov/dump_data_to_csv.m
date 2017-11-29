%
% CSV export datasets downloaded from
%
% https://zenodo.org/record/192684
%
% for initial testing of the P300 in OV
%

clear;

% Change to the current folder and add required paths to the Matlab repository
addpath('EM')
addpath('helpers')
addpath('ov')

haveNonEpoched = false;
haveEpoched = false;

if(exist('S1_cnt_mrk.mat','file'))
	haveNonEpoched = true;
end
if(exist('S1.mat','file'))
	haveEpoched = true;
end

if(haveNonEpoched)
	rawdata = load('S1_cnt_mrk.mat');
	exportcsv('../signals-test/01-raw-data.csv',[(0:(size(rawdata.cnt.x,1)-1))'/rawdata.cnt.fs,rawdata.cnt.x]);
	featTime = rawdata.mrk.time'/1000; % orig time is in milliseconds	
	tightTime = (0:(size(featTime,1)-1))'.*0.005; % this is a kludge to get a kind of fifo behavior from openvibe for groups & sequence
	
	stims = [0,32769,0];t=0;
	for i=1:size(rawdata.mrk.time,2);
		stims=[stims;featTime(i),32779,0; featTime(i)+0.1,32780,0];
	end
	stims = [stims;featTime(end)+2,32770,0];
	exportcsv('../signals-test/02-flashes.csv',stims);
	exportcsv('../signals-test/03-groups.csv',[tightTime,rawdata.mrk.stimuli']);
	exportcsv('../signals-test/04-sequence.csv',[tightTime,rawdata.mrk.sequence']);	

	if(haveEpoched)
		load('S1.mat');
		fv = proc_jumpingMeans(epo,[50 120; 121 200; 201 280;281 380;381 530; 531 700]);
		data = reshape(fv.x,31*6,12852)';	
		data = data(1:size(featTime,1),:);
		% delayed a little on purpose so the vectors are received after the flashes; this is to make sure in the
		% visualizer the flash of 68 doesn't overwrite the prediction at that some point; now prediction +0.2s later
		exportcsv('../signals-test/01-epoched-data.csv',[featTime+0.2,data]); 
	end
end

if(haveEpoched && ~haveNonEpoched)
	load(fullfile(your_data_path,'S1.mat'));
	load(fullfile(your_data_path,'sequence.mat'));

	fv = proc_jumpingMeans(epo,[50 120; 121 200; 201 280;281 380;381 530; 531 700]);

	data = reshape(fv.x,31*6,12852)';

	stimuli = epo.stimuli;

	flashDuration = 0.1;
	featTime = (0:(size(data,1)-1))'*flashDuration;

	stims = [0,32769,0];t=0;
	for i=1:size(rawdata.mrk.time,2);
		stims=[stims;featTime(i),32779,0; featTime(i)+0.1,32780,0];
	end
	stims = [stims;featTime(end)+2,32770,0];
	
	exportcsv('../signals-test/01-epoched-data.csv',[featTime,data]);
	exportcsv('../signals-test/02-flashes.csv',stims);	
	exportcsv('../signals-test/03-stimuli.csv',[featTime,stimuli']);
	exportcsv('../signals-test/04-sequence.csv',[featTime,sequence]);
end

% verify
if(0)
	% it appears in the epoched files, the first 4284 rows of sequence+stimuli correspond to the 4284 rows of
	% the rawdata file
	
	rawdata = load('S1_cnt_mrk.mat');
	S1 = load('S1.mat'); % epo
	seq = load('sequence.mat');
	assert(max(max(abs(rawdata.mrk.stimuli - S1.epo.stimuli(:,1:4284))))==0);
	assert(max(max(abs(rawdata.mrk.sequence' - seq.sequence(1:4284))))==0);
end


