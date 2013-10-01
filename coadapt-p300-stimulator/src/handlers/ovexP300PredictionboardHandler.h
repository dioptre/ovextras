#ifndef __P300PredictionboardHandler_H__
#define __P300PredictionboardHandler_H__

#include <queue>
//#include <cstring>

#include "../visualisation/glGContainer.h"
#include "../visualisation/glGTable.h"
#include "../ovexWordPredictionEngine.h"
#include "../ovexPresagePredictionEngine.h"

namespace OpenViBEApplications
{	
	//template<class TContainer>
	class P300PredictionboardHandler : public GObserver, public GObservable
	{	

	public:
		
		P300PredictionboardHandler(GContainer* container, std::string& resultBuffer, OpenViBE::CString nGramDatabaseName) 
		: m_pSymbolContainer(container), m_sSpelledLetters(resultBuffer)//, m_pWordPredictionEngine(predictionEngine)
		{
			//m_vWordArray = new std::queue< std::string >();
			#if defined TARGET_HAS_ThirdPartyPresage
				m_pWordPredictionEngine = new PresagePredictionEngine(nGramDatabaseName);
				std::cout << "Creating presage prediction engine\n";
			#else
				m_pWordPredictionEngine = new WordPredictionEngine();
			#endif
			//m_ui32MemorySize = n; //this also limits the span of the undo function
			GTable* l_pTable = dynamic_cast<GTable*>(m_pSymbolContainer);
			if (l_pTable!=NULL)
				m_ui32NPredictions = l_pTable->getRowDimension()*l_pTable->getColumnDimension();
			else
				m_ui32NPredictions = 10;
		}
		
		~P300PredictionboardHandler()
		{
			//m_vWordArray->clear();
			//delete m_vWordArray;
			delete m_pWordPredictionEngine;
		}
		
		//inherited from GObserver
		virtual void update(GObservable* observable, const  void * pUserData);
		
	private:
		//void trimLetterSequence();
		
	protected:
		GContainer* m_pSymbolContainer;
		std::string& m_sSpelledLetters;
		WordPredictionInterface* m_pWordPredictionEngine;
		//std::queue< std::string >* m_vWordArray;
		//OpenViBE::uint32 m_ui32MemorySize;
		
	private:
		OpenViBE::uint32 m_ui32NPredictions;
	};
};
#endif