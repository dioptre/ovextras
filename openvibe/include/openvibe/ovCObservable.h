#ifndef OVCOBSERVABLE_H
#define OVCOBSERVABLE_H

#include "ovIObserver.h"
#include "ov_base.h"

#include <vector>

namespace OpenViBE
{
	class IObserver;

	class CObservable
	{
	public:
		CObservable(void);

		virtual void addObserver(IObserver *o);
		virtual void deleteObserver(IObserver *o);

	protected:
		virtual void setChanged();
		virtual void clearChanged();
		virtual OpenViBE::boolean hasChanged();

		virtual void notifyObservers(void* data = NULL);

	private:
		std::vector<IObserver *> m_vObservers;
		OpenViBE::boolean m_bHasChanged;
	};
}


#endif // OVCOBSERVABLE_H
