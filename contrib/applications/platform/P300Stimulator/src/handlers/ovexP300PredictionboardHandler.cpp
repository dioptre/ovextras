#include <vector>

#include "ovexP300PredictionboardHandler.h"
#include "ovexUndoHandler.h"
#include "ovexP300ResultAreaHandler.h"
#include "ovexP300KeyboardHandler.h"
#include "../visualisation/glGButton.h"
#include "../visualisation/glGSymbol.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

//class P300KeyboardHandler;
//class P300ResultAreaHandler;
//class P300UndoHandler;

void P300PredictionboardHandler::update(GObservable* observable, const void * pUserData) 
{
	P300ResultAreaHandler* l_pResultHandler = dynamic_cast<P300ResultAreaHandler*>(observable);
	P300UndoHandler* l_pUndoHandler = dynamic_cast<P300UndoHandler*>(observable);
	//P300KeyboardHandler* l_pKeyboardHandler = dynamic_cast<P300KeyboardHandler*>(observable);
	
	/*if (l_pKeyboardHandler!=NULL)
	{
		GButton* l_pGButton = static_cast< uint32* >(pUserData);
	}*/
	if (l_pResultHandler!=NULL)
	{
		const char* l_pCharString = static_cast< const char* >(pUserData);
		
		if (l_pCharString!=NULL)
		{
			//std::cout << "PredictionHandler: received new letter sequence " << l_pCharString << "\n";
			//m_sSpelledLetters += std::string(l_pCharString);
			std::cout << "P300PredictionboardHandler notification of result handler, result is now " << m_sSpelledLetters << "\n";
			//this->trimLetterSequence();
			//std::cout << "PredictionHandler: current letter sequence " << m_sSpelledLetters.c_str() << "\n";
			std::vector<std::string>* l_lWords = m_pWordPredictionEngine->getMostProbableWords(m_sSpelledLetters, m_ui32NPredictions);
			//std::cout << "First word " << l_lWords->at(0).c_str() << "\n";

			this->notifyObservers(l_lWords);
		}
	}
	else if (l_pUndoHandler!=NULL)
	{
		/*uint32 l_ui32Tail = *static_cast< const uint32* >(pUserData);
		if (!m_sSpelledLetters.empty() && m_sSpelledLetters.length()-l_ui32Tail>=0)
			m_sSpelledLetters.erase(m_sSpelledLetters.length()-l_ui32Tail, l_ui32Tail);*/
		std::cout << "P300PredictionboardHandler notification of undo handler, result is now " << m_sSpelledLetters << "\n";
		std::vector<std::string>* l_lWords = m_pWordPredictionEngine->getMostProbableWords(m_sSpelledLetters, m_ui32NPredictions);
		//do something with predicted words;
		this->notifyObservers(l_lWords);
	}
}

/*void P300PredictionboardHandler::trimLetterSequence()
{
	//int l_iLastSpaceIndex = m_sSpelledLetters.length();	
	size_t l_iSpaceIndex = (m_sSpelledLetters[m_sSpelledLetters.length()-1]==' ') ? m_sSpelledLetters.length()-1 : m_sSpelledLetters.length();
	uint32 l_ui32WordCounter = 0;
	while (l_iSpaceIndex!=std::string::npos)
	{
		//l_iLastSpaceIndex = l_iSpaceIndex;
		l_ui32WordCounter++;
		//std::cout << "PredictionHandler: space index " << l_iSpaceIndex  << ", " << std::string::npos << "\n";
		//std::cout << m_sSpelledLetters << "\n";
		l_iSpaceIndex = m_sSpelledLetters.rfind(" ",l_iSpaceIndex-1);	
		std::cout << "PredictionHandler: space index " << l_iSpaceIndex  << ", " << std::string::npos << "\n";
		//break;
	}
	std::cout << "P300PredictionboardHandler:: word count " << l_ui32WordCounter << "\n";
	if (l_ui32WordCounter>m_ui32MemorySize)
	{
		l_iSpaceIndex = m_sSpelledLetters.find(" ");	
		m_sSpelledLetters.erase(0, l_iSpaceIndex+1);
		std::cout << "P300PredictionboardHandler:: Deleting a word from buffer, result " << m_sSpelledLetters << "\n";
	}
}*/

/*void splitString()
{
	int l_iLastSpaceIndex;
	if (m_sSpelledLetters[m_sSpelledLetters.length()-1]==" ")
		l_iLastSpaceIndex = m_sSpelledLetters.length()-1;
	else
		l_iLastSpaceIndex = m_sSpelledLetters.length();	
	int l_iSpaceIndex = m_sSpelledLetters.rfind(" ",l_iLastSpaceIndex);

	while (l_iSpaceIndex!=std::string::npos)
	{
		uint32 l_ui32WordLength = l_iLastSpaceIndex-l_iSpaceIndex;
		std::string l_sWord = m_sSpelledLetters.substr(l_iSpaceIndex+1,l_ui32WordLength);
		m_vWordArray->push(l_sWord);
		l_iLastSpaceIndex = l_iSpaceIndex;
		l_iSpaceIndex = m_sSpelledLetters.rfind(" ",l_iLastSpaceIndex);
	}
	m_vWordArray->push(m_sSpelledLetters.substr(0,l_iLastSpaceIndex));
	
	while (m_vWordArray->size()>m_ui32MemorySize)
		m_vWordArray->pop();
}*/