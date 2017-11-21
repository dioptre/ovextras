% p300_Process.m
% -------------------------------
% Author : Jussi T. Lindgren / Inria
% Date   : 
%
%

function box_out = p300_Process(box_in)	
	
	haveChunk = (OV_getNbPendingInputChunk(box_in,1)>0) &&  (OV_getNbPendingInputChunk(box_in,2)>0) &&  (OV_getNbPendingInputChunk(box_in,3)>0);
	
	if(~haveChunk)
		box_out = box_in;
		return;
	end

	global gnr_ept;
	global gfeatures;
	global gflashed;
	global gsequence;
	global gC;
	global gTotal;
	global gTrials;
	global gCnt;
	
	% As the current EM code gets slow when enough data is accumulated,
	% here we always work just 'maxTrials' last trials worth of data.
	maxTrials = 15;
	maxFeatures = maxTrials*gnr_ept;
	
	[box_in, start_time, end_time, features] = OV_popInputBuffer(box_in,1);
 	[box_in, start_time, end_time, flashed] = OV_popInputBuffer(box_in,2);
 	[box_in, start_time, end_time, sequence] = OV_popInputBuffer(box_in,3);
	
	% we get in a chunk of data, turn it into feature vector
	%features = jumpavg(features);
	%features = reshape(features, ...);
	
	if(isempty(gfeatures))
		gCnt = 0;
		gTrials = 0;
		gTotal = 0;
		gfeatures = zeros(maxFeatures,size(features,2));
		gflashed = zeros(maxFeatures,size(flashed,2));
		gsequence = zeros(maxFeatures,size(sequence,2));
	end
	
	if(gCnt==maxFeatures)
		% treat as a fifo
		gfeatures=circshift(gfeatures,[-1 0]);
		gflashed=circshift(gflashed,[-1 0]);
		gsequence=circshift(gsequence,[-1 0]);
	else
		gCnt = gCnt + 1;
	end
	
	gfeatures(gCnt,:) = features;
	gflashed(gCnt,:) = flashed;
	gsequence(gCnt,:) = sequence;
		
	gTotal = gTotal + 1;
	
	if(rem(gTotal,gnr_ept)==0)
		gTrials = gTrials + 1;
		
		% @fixme, hardcoded
		gamma = -1;
		A = [3/8 5/8; 2/18 16/18];

		% we've collected features for the whole trial, update model
		gC = simulate_MIX(gC,gfeatures,logical(gflashed'),gsequence,[],gnr_ept,A,gamma,[],false);

		% predict
		% output prediction (2 stim pair?), print letter
		[probs,select]=max(gC.classifier.probs,[],2);
		
		fprintf(1,'Trial %03d: Final sentence so far: %s\n', gTrials, convert_position_to_letter(select));
				
		%if(gTrials==30)
		%	profile off;
		%	profile viewer;
		%end
	end

	box_out = box_in;
end


