#ifndef __OpenViBEKernel_Kernel_Algorithm_CAlgorithmManager_H__
#define __OpenViBEKernel_Kernel_Algorithm_CAlgorithmManager_H__

#include "../ovkTKernelObject.h"

#include <map>

#include <boost/thread.hpp> // for mutex
#include <boost/thread/condition.hpp>

namespace OpenViBE
{
	namespace Kernel
	{
		class CAlgorithm;
		class CAlgorithmProxy;

		class CAlgorithmManager : public OpenViBE::Kernel::TKernelObject < OpenViBE::Kernel::IAlgorithmManager >
		{
		public:

			CAlgorithmManager(const OpenViBE::Kernel::IKernelContext& rKernelContext);
			virtual ~CAlgorithmManager(void);

			virtual OpenViBE::CIdentifier createAlgorithm(
				const OpenViBE::CIdentifier& rAlgorithmClassIdentifier);
			virtual OpenViBE::boolean releaseAlgorithm(
				const OpenViBE::CIdentifier& rAlgorithmIdentifier);
			virtual OpenViBE::boolean releaseAlgorithm(
				OpenViBE::Kernel::IAlgorithmProxy& rAlgorithm);
			virtual OpenViBE::Kernel::IAlgorithmProxy& getAlgorithm(
				const OpenViBE::CIdentifier& rAlgorithmIdentifier);
			virtual OpenViBE::CIdentifier getNextAlgorithmIdentifier(
				const OpenViBE::CIdentifier& rPreviousIdentifier) const;

			_IsDerivedFromClass_Final_(OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::IAlgorithmManager>, OVK_ClassId_Kernel_Algorithm_AlgorithmManager);

		protected:

			virtual OpenViBE::CIdentifier getUnusedIdentifier(void) const;

		protected:

			std::map < OpenViBE::CIdentifier, std::pair < OpenViBE::Kernel::CAlgorithm*, OpenViBE::Kernel::CAlgorithmProxy* > > m_vAlgorithm;

			mutable boost::mutex m_oLock; // Protects the array for threads
		};
	};
};

#endif // __OpenViBEKernel_Kernel_Algorithm_CAlgorithmManager_H__
