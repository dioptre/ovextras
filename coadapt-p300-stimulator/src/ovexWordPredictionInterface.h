#ifndef __WordPredictionInterface_H__
#define __WordPredictionInterface_H__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <iostream>
#include <vector>
#include <string>

#include "ova_defines.h"

namespace OpenViBEApplications
{	
	//template<class TContainer>
	class WordPredictionInterface
	{	
	public:
		virtual std::vector<std::string>* getMostProbableWords(const std::string&  prefix, OpenViBE::uint32 nWords)=0;
	};
};
#endif
