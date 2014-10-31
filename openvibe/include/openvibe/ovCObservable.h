#ifndef OVCOBSERVABLE_H
#define OVCOBSERVABLE_H

#include "ovIObserver.h"
#include "ov_base.h"

namespace OpenViBE
{
	class IObserver;

	class OV_API CObservable
	{
	public:
		CObservable(void);
		virtual ~CObservable(void);

		virtual void addObserver(IObserver *o);
		virtual void deleteObserver(IObserver *o);

	protected:
		virtual void setChanged();
		virtual void clearChanged();
		virtual OpenViBE::boolean hasChanged();

		virtual void notifyObservers(void* data = NULL);

	private:
		struct ObserverList;
		ObserverList* m_pObserverList;
		OpenViBE::boolean m_bHasChanged;
	};
}


#endif // OVCOBSERVABLE_H
