#include <vector>

#include "ovexUndoHandler.h"
#include "ovexP300ResultAreaHandler.h"
#include "ovexP300KeyboardHandler.h"
#include "../visualisation/glGButton.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

//class P300KeyboardHandler;
//class P300ResultAreaHandler;

void P300UndoHandler::update(GObservable* observable, const void * pUserData)
{
	P300ResultAreaHandler* l_pResultHandler = dynamic_cast<P300ResultAreaHandler*>(observable);
	GButton* l_pButtonHandler = dynamic_cast<GButton*>(observable);
	//std::vector<GLabel*>* l_sNewLabelVector = static_cast< std::vector<GLabel*>* >(pUserData);
	const char * l_sNewString = static_cast< const char* >(pUserData);
	//if undo button is clicked
	if (l_pButtonHandler!=NULL)
	{
		const GButton* l_pButton = static_cast<const GButton*>(pUserData);
		if (l_pButton->getState()==GButton_Clicked && m_sUndoStack->size()>0)
		{
			uint32 l_ui32UndoSize = m_sUndoStack->back().length();
			m_sUndoStack->pop_back();		
			this->notifyObservers(&l_ui32UndoSize);
		}
	}
	//if a result is added to the result area, those letters should also be added to the undo stack
	else if (l_pResultHandler!=NULL && l_sNewString!=NULL)
	{
		//std::stack<GLabel*>* l_sNewLabelStack = new std::stack<GLabel*>();
		//for (uint32 i=0; i<l_sNewLabelVector->size(); i++)
		//{
		//	l_sNewLabelStack->push(l_sNewLabelVector->at(i)->clone());
			//std::cout << "Adding " << l_sNewLabelStack->top()->toString() << " to the undo stack, current stack size " << m_sUndoStack->size()+1 << "\n";
		//}
		m_sUndoStack->push_back(std::string(l_sNewString));
		if (m_sUndoStack->size()>15)
			m_sUndoStack->pop_front();
	}
}