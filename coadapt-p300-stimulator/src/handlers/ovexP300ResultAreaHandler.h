#ifndef __P300ResultAreaHandler_H__
#define __P300ResultAreaHandler_H__

#include <cstring>

#include "../visualisation/glGTable.h"
#include "../visualisation/glGLabel.h"
#include "../properties/ovexP300ScreenLayoutReader.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <vector>

namespace OpenViBEApplications
{	
	//template<class TContainer>
	class P300ResultAreaHandler : public GObserver, public GObservable
	{	

	public:
		
		P300ResultAreaHandler(GTable* container, P300ScreenLayoutReader* propertyObject);
		
		virtual ~P300ResultAreaHandler()
		{
			#ifdef OUTPUT_TIMING
			fclose(timingFile);
			#endif
			//for (OpenViBE::uint32 i=0; i<m_vBlankLabel.size(); i++)
			//	delete m_vBlankLabel[i];
		}
		
		//inherited from GObserver
		virtual void update(GObservable* observable, const void * pUserData);
		std::string& getResultBuffer() { return m_sSpelledLetters; };
		
	private:
		void moveSymbolsLeft(OpenViBE::uint32 nshift);
		void trimPrefix(std::string& textLabel);
		void updateResultBuffer();
		std::string eraseLastCharacter();
		
	protected:
		GTable* m_pSymbolContainer;
		
	private:
		OpenViBE::uint32 m_ui32State;
		P300ScreenLayoutReader* m_pScreenLayoutObject;
		std::string m_sSpelledLetters;
		OpenViBE::uint32 m_ui32ResultCounter;
		#ifdef OUTPUT_TIMING
		FILE * timingFile;
		#endif
		//std::vector<GLabel*> m_vBlankLabel;
		GLabel* m_oLastAddedLabel;
		OpenViBE::float64 m_f32LastFontSize;
	};
};
#endif
