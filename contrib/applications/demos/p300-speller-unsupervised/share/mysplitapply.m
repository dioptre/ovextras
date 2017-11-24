function res = mysplitapply( fun, data, groups )
% mysplitapply applies function fun to rows of data specified by groups
	
	% [length(groups) size(data,1)]
	
	assert(length(groups)==size(data,1));

	u = unique(groups);
	tmp = fun(data(groups==u(1),:));
	res = zeros(length(u), length(tmp));
	res(1,:) = tmp;
	for i=2:length(u)
		res(i,:) = fun(data(groups==u(i),:));
	end

end

