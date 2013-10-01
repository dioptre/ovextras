#include <vector>

#include "ovexUndoHandler.h"
#include "ovexBackspaceHandler.h"
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
	std::cout << "undo handler\n";
	P300ResultAreaHandler* l_pResultHandler = dynamic_cast<P300ResultAreaHandler*>(observable);
	GButton* l_pButtonHandler = dynamic_cast<GButton*>(observable);
	//std::vector<GLabel*>* l_sNewLabelVector = static_cast< std::vector<GLabel*>* >(pUserData);
	const char * l_sNewString = static_cast< const char* >(pUserData);
	//if undo button is clicked
	if (l_pButtonHandler!=NULL)
	{
		const GButton* l_pButton = static_cast<const GButton*>(pUserData);
		ButtonAction l_eAction = l_pButton->getAction();


		//clicking undo button
		if (l_pButton->getState()==GButton_Clicked && m_sUndoStack->size()>0 && l_eAction==GButton_Undo)
		{
			std::string l_sStringToUndo = m_sUndoStack->back();
			std::cout <<"string to undo" <<  l_sStringToUndo << "\n";
			uint32 l_ui32UndoSize = m_sUndoStack->back().length();			
			//what is undone goes to the redo stack
			m_sUndoStack->pop_back();
			m_sRedoStack->push_back(l_sStringToUndo);
			std::string l_CharToRestore;
			//if we undo backspace
			if (l_sStringToUndo.find("<")!=std::string::npos)
			{
				std::cout <<"undo handler undiing <\n";
				l_CharToRestore = l_sStringToUndo[l_sStringToUndo.size()-1];
				std::cout <<"restoring |"  << l_CharToRestore << "|\n";
				l_ui32UndoSize = 0;
			}

			std::cout <<"undo handler notifying\n";
			std::pair<uint32, std::string> l_oUserData(l_ui32UndoSize,l_CharToRestore);
			this->notifyObservers(&l_oUserData);
			std::cout <<"done ...\n";
		}
		else if (l_pButton->getState()==GButton_Clicked && m_sRedoStack->size()>0 && l_eAction==GButton_Redo)
		{
			std::cout <<"redoing\n";
			std::string l_sStringToRedo = m_sRedoStack->back();
			int32 l_i64RedoSize = -l_sStringToRedo.size();
			//what is redone goes to the undo stack
			m_sRedoStack->pop_back();
			m_sUndoStack->push_back(l_sStringToRedo);
			std::cout <<"redoing" << l_sStringToRedo << "\n";

			std::cout <<"undo handler (redo) notifying\n";
			std::pair<int32, std::string> l_oUserData(l_i64RedoSize,l_sStringToRedo);
			this->notifyObservers(&l_oUserData);
			std::cout <<"done ...\n";

		}


	}
	//if a result is added to the result area, those letters should also be added to the undo stack
	else if (l_pResultHandler!=NULL && l_sNewString!=NULL)
	{
		std::cout << "undo handler adding " << l_sNewString << " to undo stack\n";
		//std::stack<GLabel*>* l_sNewLabelStack = new std::stack<GLabel*>();
		//for (uint32 i=0; i<l_sNewLabelVector->size(); i++)
		//{
		//	l_sNewLabelStack->push(l_sNewLabelVector->at(i)->clone());
			//std::cout << "Adding " << l_sNewLabelStack->top()->toString() << " to the undo stack, current stack size " << m_sUndoStack->size()+1 << "\n";
		//}
		m_sUndoStack->push_back(std::string(l_sNewString));
		if (m_sUndoStack->size()>15)
			m_sUndoStack->pop_front();
		if (m_sRedoStack->size()>15)
			m_sRedoStack->pop_front();
	}
}
