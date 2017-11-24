function exportcsv( fn, mat, isint )
% Can be read as feature vectors in openvibe

	h=fopen(fn,'w');
	fprintf(h,'Time(s)');
	for i=1:size(mat,2)-1
		fprintf(h,';f%03d',i);
	end
	fprintf(h,'\n');
	fclose(h);
	
	if(isint)
		dlmwrite(fn,mat,'-append','delimiter',';');
	else
		dlmwrite(fn,mat,'-append','delimiter',';','precision','%.3f');
		% dlmwrite(fn,mat,'-append','delimiter',';','precision','%.6f');
	end
end

