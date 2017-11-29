function exportcsv( fn, mat, freq )
% Can be read as feature vectors or signal in openvibe. For signal, provide sampling frequency 'freq'.

	h=fopen(fn,'w');
	fprintf(h,'Time(s)');
	for i=1:size(mat,2)-1
		fprintf(h,';f%03d',i);
	end
	if(nargin==3)
		fprintf(h,';Freq\n');
		for i=1:size(mat,2)
			fprintf(h,'%.4f;',mat(1,i));
		end
		fprintf(h,'%d\n',freq);
		startIdx = 2;	
	else
		fprintf(h,'\n');
		startIdx = 1;
	end
	fclose(h);
	
	dlmwrite(fn,mat(startIdx:end,:),'-append','delimiter',';','precision','%.4f');
	
end

