%
% CSV export datasets downloaded from
%
% https://zenodo.org/record/192684
%
% for initial testing of the P300 in OV
%

clear;

% Change to the current folder and add required paths to the Matlab repository
addpath('../matlab/helpers')

haveNonEpoched = false;
haveEpoched = false;

if(exist('S1_cnt_mrk.mat','file'))
	haveNonEpoched = true;
end
if(exist('S1.mat','file'))
	haveEpoched = true;
end
assert(haveEpoched || haveNonEpoched);

if(haveNonEpoched)
	rawdata = load('S1_cnt_mrk.mat');
	
	sampleTimes = (0:(size(rawdata.cnt.x,1)-1))'/rawdata.cnt.fs;
	featTime = rawdata.mrk.time'/1000; % orig time is in milliseconds		
	
	stims = [0,32769,0];t=0;
	for i=1:size(rawdata.mrk.time,2);
		stims=[stims;featTime(i),32779,0; featTime(i)+0.1,32780,0];
	end
	stims = [stims;featTime(end)+2,32770,0];
	
	exportcsv20('../signals-test/01-raw-data.csv',rawdata.cnt.x, sampleTimes, false, rawdata.cnt.fs);
	exportcsv('../signals-test/02-flashes.csv',stims);	% 20's csv reader has issues atm for stims
	exportcsv20('../signals-test/03-groups.csv',rawdata.mrk.stimuli', featTime, true);
	exportcsv20('../signals-test/04-sequence.csv',rawdata.mrk.sequence', featTime, true);

	if(haveEpoched)
		load('S1.mat');
		fv = proc_jumpingMeans(epo,[50 120; 121 200; 201 280;281 380;381 530; 531 700]);
		data = reshape(fv.x,31*6,12852)';	
		data = data(1:size(featTime,1),:);
		exportcsv20('../signals-test/01-epoched-data.csv',data,featTime, false); 
	end
end

if(haveEpoched && ~haveNonEpoched)
	load(fullfile(your_data_path,'S1.mat'));
	load(fullfile(your_data_path,'sequence.mat'));

	fv = proc_jumpingMeans(epo,[50 120; 121 200; 201 280;281 380;381 530; 531 700]);

	data = reshape(fv.x,31*6,12852)';

	stimuli = epo.stimuli;

	% We lack the original flash onsets, make up
	flashDuration = 0.1;
	featTime = (0:(size(data,1)-1))'*flashDuration;

	stims = [0,32769,0];t=0;
	for i=1:size(rawdata.mrk.time,2);
		stims=[stims;featTime(i),32779,0; featTime(i)+0.1,32780,0];
	end
	stims = [stims;featTime(end)+2,32770,0];
	
	exportcsv20('../signals-test/01-epoched-data.csv',data,featTime,false);
	exportcsv('../signals-test/02-flashes.csv',stims);	
	exportcsv20('../signals-test/03-stimuli.csv',stimuli',featTime,true);
	exportcsv20('../signals-test/04-sequence.csv',sequence,featTime,true);
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


