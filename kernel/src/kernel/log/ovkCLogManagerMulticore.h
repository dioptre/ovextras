#ifndef __OpenViBEKernel_Kernel_Log_CLogManagerMulticore_h__
#define __OpenViBEKernel_Kernel_Log_CLogManagerMulticore_h__

#include "../ovkTKernelObject.h"

#include <vector>
#include <map>

#include <boost/thread.hpp>
#include <boost/any.hpp>


namespace OpenViBE
{
	namespace Kernel
	{
		class CLogManagerMulticore : public OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::ILogManager>
		{
		public:

			CLogManagerMulticore(const OpenViBE::Kernel::IKernelContext& rKernelContext);

			virtual OpenViBE::boolean isActive(OpenViBE::Kernel::ELogLevel eLogLevel);
			virtual OpenViBE::boolean activate(OpenViBE::Kernel::ELogLevel eLogLevel, OpenViBE::boolean bActive);
			virtual OpenViBE::boolean activate(OpenViBE::Kernel::ELogLevel eStartLogLevel, OpenViBE::Kernel::ELogLevel eEndLogLevel, OpenViBE::boolean bActive);
			virtual OpenViBE::boolean activate(OpenViBE::boolean bActive);


			virtual void log(const OpenViBE::time64 time64Value);

			virtual void log(const OpenViBE::uint64 ui64Value);
			virtual void log(const OpenViBE::uint32 ui32Value);
			virtual void log(const OpenViBE::uint16 ui16Value);
			virtual void log(const OpenViBE::uint8 ui8Value);

			virtual void log(const OpenViBE::int64 i64Value);
			virtual void log(const OpenViBE::int32 i32Value);
			virtual void log(const OpenViBE::int16 i16Value);
			virtual void log(const OpenViBE::int8 i8Value);

			virtual void log(const OpenViBE::float64 f64Value);
			virtual void log(const OpenViBE::float32 f32Value);

			virtual void log(const OpenViBE::boolean bValue);

			virtual void log(const OpenViBE::CIdentifier& rValue);

			virtual void log(const OpenViBE::CString& rValue);
			virtual void log(const char* pValue);

			virtual void log(const OpenViBE::Kernel::ELogLevel eLogLevel);
			virtual void log(const OpenViBE::Kernel::ELogColor eLogColor);

			virtual OpenViBE::boolean addListener(OpenViBE::Kernel::ILogListener* pListener);
			virtual OpenViBE::boolean removeListener(OpenViBE::Kernel::ILogListener* pListener);

			_IsDerivedFromClass_Final_(OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::ILogManager>, OVK_ClassId_Kernel_Log_LogManager);

		protected:

			// Caches per-thread buffer & its loglevel
			struct LogBuffer
			{
				std::vector<boost::any> m_vBuffer;
				ELogLevel m_eCurrentLogLevel;
			};

			// Get the buffer of the current thread
			LogBuffer& getBuffer();
			// Flush the buffer of the current thread
			OpenViBE::boolean flushBuffer();

			// Insert a log item into the current threads buffer
			template <class T> void pushToBuffer(const T oValue)
			{
				// note here we lose the ref on purpose on oValue since the caller may release the data before we flush

				LogBuffer& l_oMyBuffer = getBuffer();	

				if(l_oMyBuffer.m_eCurrentLogLevel!=LogLevel_None && this->isActive(l_oMyBuffer.m_eCurrentLogLevel))
				{
					l_oMyBuffer.m_vBuffer.push_back(oValue);
				}
			}

		protected:


			boost::mutex m_oBufferLock;	// Guards the LogBuffer map
			boost::mutex m_oOutputLock; // Guards the calls to the listeners

			std::map<boost::thread::id, LogBuffer> m_vLogBuffer;

			std::vector<OpenViBE::Kernel::ILogListener*> m_vListener;
			std::map<OpenViBE::Kernel::ELogLevel, OpenViBE::boolean> m_vActiveLevel;

		};
	};
};

#endif // __OpenViBEKernel_Kernel_Log_CLogManagerMulticore_h__
