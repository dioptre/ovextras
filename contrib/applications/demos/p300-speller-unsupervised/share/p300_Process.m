% p300_Process.m
% -------------------------------
% Author : Jussi T. Lindgren / Inria
% Date   : 
%
%

function box_out = p300_Process(box_in)	
	
	% to use some global variables, we need to declare them
	global OVTK_StimulationId_Target;
	global OVTK_StimulationId_Label_01;
	global OVTK_StimulationId_Label_08;
	global OVTK_StimulationId_NonTarget;
		
	haveChunk = (OV_getNbPendingInputChunk(box_in,1)>0) && ...
		        (OV_getNbPendingInputChunk(box_in,2)>0) && ...
				(OV_getNbPendingInputChunk(box_in,3)>0);
	
	if(~haveChunk)
		box_out = box_in;
		return;
	end

	% As the current EM code gets slow when enough data is accumulated,
	% here we always work just 'maxTrials' last trials worth of data.

	maxFeatures = box_in.user_data.max_trials*box_in.user_data.flashes_per_trial;
	
	[box_in, ~, end_time, features] = OV_popInputBuffer(box_in,1);
 	[box_in, ~, ~, flashed] = OV_popInputBuffer(box_in,2);
 	[box_in, ~, ~, sequence] = OV_popInputBuffer(box_in,3);
	
	% fprintf(1,'%d %d\n',size(features,1),size(features,2));
	
	if(size(features,2)==1 && size(features,1)>1)  
		% fix orientation
		features = features';
		flashed = flashed';
		sequence = sequence';
	end
	
	% [size(features) size(flashed) size(sequence)]
	
	if(size(features,1)>1 && size(features,2)>1)
		% change to [time x chns]
		features = features';
		
		% if we get in a chunk of data, assume its an epoch, turn it into a 
		% feature vector by just taking in 6 equally sized groups. In
		% OV, data matrix is [chns x samples].
		groupIdxs = round(linspace(1,6,size(features,1)))';
		
		% [size(groupIdxs) size(features)]
		
		if(exist('splitapply','file'))
			features = splitapply(@mean, features, groupIdxs);
		else
			features = mysplitapply(@mean, features, groupIdxs);	
		end

		% change to row vector
		features = features(:)';
	end
	
	% size(sequence)
		
	% disp('Matlab initialize function has been called.\n')
	
	if(isempty(box_in.user_data.gfeatures))
		box_in.user_data.gfeatures = zeros(maxFeatures,size(features,2));
		box_in.user_data.gflashed = zeros(maxFeatures,size(flashed,2));
		box_in.user_data.gsequence = zeros(maxFeatures,size(sequence,2));
	end
	
	% size(gfeatures)
	% [size(features) maxFeatures]
	
	if(box_in.user_data.total_flashes==maxFeatures)
		% treat as a fifo
		box_in.user_data.gfeatures = circshift(box_in.user_data.gfeatures,[-1 0]);
		box_in.user_data.gflashed = circshift(box_in.user_data.gflashed,[-1 0]);
		box_in.user_data.gsequence = circshift(box_in.user_data.gsequence,[-1 0]);
	else
		box_in.user_data.total_flashes = box_in.user_data.total_flashes + 1;
	end
	
	% box_in.user_data
	
	box_in.user_data.gfeatures(box_in.user_data.total_flashes,:) = features;
	box_in.user_data.gflashed(box_in.user_data.total_flashes,:) = flashed;
	box_in.user_data.gsequence(box_in.user_data.total_flashes,:) = sequence;

	if(rem(box_in.user_data.total_flashes,box_in.user_data.flashes_per_trial)==0)
		% we've collected features for the whole trial, update model
	
		box_in.user_data.total_trials = box_in.user_data.total_trials + 1;
		
		% @fixme, hardcoded
		gamma = -1;
		A = [3/8 5/8; 2/18 16/18];

		numTrials = min(box_in.user_data.total_trials,box_in.user_data.max_trials);
		
		% box_in.user_data
	 	%numTrials
		
		box_in.user_data.C = simulate_MIX(box_in.user_data.C, ...
			box_in.user_data.gfeatures, logical(box_in.user_data.gflashed'), ...
			box_in.user_data.gsequence, ...
			[],box_in.user_data.flashes_per_trial,A,gamma,1:numTrials,false);

		% predict
		% output prediction (2 stim pair?), print letter
		[~,select]=max(box_in.user_data.C.classifier.probs,[],2);
		
		fprintf(1,'Trial %03d: Final sentence so far: %s\n', box_in.user_data.total_trials, convert_position_to_letter(select));
		
		% select
		% Convert select to stimulations		
		select = select - 1; 
		rowidx = floor(select / box_in.user_data.keyboard_cols);
		colidx = rem(select, box_in.user_data.keyboard_cols);
		
		%[start_time, end_time]
		
		% [select, rowidx, colidx]
		
		stim_set = [double(OVTK_StimulationId_Target); end_time; 0];
		for q=1:length(rowidx)
			stim_set = [stim_set, [rowidx(q) + double(OVTK_StimulationId_Label_01); end_time; 0]];
			stim_set = [stim_set, [colidx(q) + double(OVTK_StimulationId_Label_08); end_time; 0]];
		end
		stim_set = [stim_set, [double(OVTK_StimulationId_NonTarget); end_time; 0]];
		
		% uint64(stim_set)
		
		box_in = OV_addOutputBuffer(box_in,1,box_in.user_data.previous_time,end_time,stim_set);	

		
		%if(gTrials==30)
		%	profile off;
		%	profile viewer;
		%end
	else
		box_in = OV_addOutputBuffer(box_in,1,box_in.user_data.previous_time,end_time,[]);
	end
	
	box_in.user_data.previous_time = end_time;

	% box_in.user_data
	
	box_out = box_in;
end


