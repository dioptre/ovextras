function exportcsv20( fn, mat, sampleTime, isInt )
% Can be read as feature vectors in openvibe 2.0

	h=fopen(fn,'w');
	fprintf(h,'Time:%d,End Time',size(mat,2));
	for i=1:size(mat,2)
		fprintf(h,',f%03d',i);
	end
	fprintf(h,',Event Id,Event Date,Event Duration\n');

	duration=sampleTime(2)-sampleTime(1);
	sampleTime=[sampleTime;sampleTime(end)+duration];
	
	% can't use dlmwrite as we need 'empty' events... ouch slowness
	if(isInt)
		for i=1:size(mat,1)
			fprintf(h,'%f,%f',sampleTime(i),sampleTime(i+1));
			for j=1:size(mat,2)
				fprintf(h,',%d',mat(i,j));
			end
			fprintf(h,',,,\n');
		end
	else
		for i=1:size(mat,1)
			fprintf(h,'%f,%f',sampleTime(i),sampleTime(i+1));			
			for j=1:size(mat,2)
				fprintf(h,',%f',mat(i,j));
			end
			fprintf(h,',,,\n');
		end
	end
	
	fclose(h);
	
end

