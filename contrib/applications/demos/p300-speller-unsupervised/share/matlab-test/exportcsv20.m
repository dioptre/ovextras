function exportcsv20( fn, mat, sampleTime, isInt, freq, stims)
% Can be read as feature vectors in openvibe 2.0

	epochSize = 32;
	
	if(nargin<4)
		isInt = false;
	end
	if(nargin<5)
		freq = 0;
	end
	if(nargin<6)
		stims = [];
	end
	
	h=fopen(fn,'w');
	if(freq>0)
		fprintf(h,'Time:%dHz,Epoch',freq);
		
		numEpochs = floor(size(mat,1)/epochSize);
		
		endTime = repmat((0:(numEpochs-1)),[epochSize 1]);
		endTime = endTime(:);
		diffSize = size(mat,1)-size(endTime,1);
		if(diffSize>0)
			endTime = [endTime;repmat(numEpochs,[diffSize 1])];
		end
		timePattern = '%.4f,%d';
	else
		fprintf(h,'Time:%d,End Time',size(mat,2));		
		
		% Assumes equispaced vectors in time
		duration=sampleTime(2)-sampleTime(1);
		endTime = sampleTime + duration;
		
		timePattern = '%.4f,%.4f';	
	end
	for i=1:size(mat,2)
		fprintf(h,',f%03d',i);
	end
	fprintf(h,',Event Id,Event Date,Event Duration\n');

	if(isInt)
		pattern = ',%d';
	else
		pattern = ',%.4f';
	end
	
	% can't use dlmwrite as we need 'empty' events... ouch slowness
	stimCnt=1;
	for i=1:size(mat,1)
		fprintf(h,timePattern,sampleTime(i),endTime(i));			
		for j=1:size(mat,2)
			fprintf(h,pattern,mat(i,j));
		end
		if(isempty(stims) || stimCnt>size(stims,1))
			fprintf(h,',,,\n');
		else
			if(stims(stimCnt,1)<=sampleTime(i))
				fprintf(h,',%d,%.4f,0\n',stims(stimCnt,2),stims(stimCnt,1));
				stimCnt = stimCnt + 1;
			else
				fprintf(h,',,,\n');	
			end
		end
	end
	
	fclose(h);
	
end

