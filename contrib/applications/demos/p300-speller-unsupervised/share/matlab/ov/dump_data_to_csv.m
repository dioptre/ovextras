%
%
% CSV export datasets downloaded from
%
% https://zenodo.org/record/192684
%
% for initial testing of the P300 in OV
%

% !! Set your data path !!
your_data_path = 'data';

% Change to the current folder and add required paths to the Matlab repository
addpath('EM')
addpath('helpers')

% Load stimuli and epoch data for subject 1 (out of 13)
load(fullfile(your_data_path,'S1.mat'));

% Load the sequence data for LLP indicating whether epoch k was part of
% sequence 1 or sequence 2. It is the same for all subjects.
load(fullfile(your_data_path,'sequence.mat'));

% Extract a feature vector where the features are the mean amplitudes in
% the 6 given intervals
fv = proc_jumpingMeans(epo,[50 120; 121 200; 201 280;281 380;381 530; 531 700]);

% Bring feature matrix in the shape [N * feat_dim] 
% where feat_dim = n_channels * n_time_intervals
data = reshape(fv.x,31*6,12852)';

% Next, look at "stimuli" which encodes which symbols are highlighted for
% each visual highlighting event (stimulus)
% Shape: [feat_dim * N] with entry(i,j)=1 if symbol i was highlighted
% during epoch j and 0 else.
stimuli = epo.stimuli;

% Write openvibe readable CSV files for testing
flashDuration = 0.1;
tdata = [(1:size(data,1))'*flashDuration, data];
tstimuli = [tdata(:,1),stimuli'];
tsequence = [tdata(:,1),sequence];

exportcsv('../signals/01-data.csv',tdata);
exportcsv('../signals/02-stimuli.csv',tstimuli);
exportcsv('../signals/03-sequence.csv',tsequence);
