function exportcsv( fn, mat )
% Can be read as feature vectors in openvibe

	h=fopen(fn,'w');
	fprintf(h,'Time(s)');
	for i=1:size(mat,2)-1
		fprintf(h,';f%03d',i);
	end
	fprintf(h,'\n');
	fclose(h);
	
	dlmwrite(fn,mat,'-append','delimiter',';','precision','%.4f');

end

