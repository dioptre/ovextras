#include "ovCMemoryBuffer.h"

#include <string.h> // memcpy

using namespace OpenViBE;

namespace OpenViBE
{
	namespace
	{
		class CMemoryBufferImpl : public IMemoryBuffer
		{
		public:

			CMemoryBufferImpl(void);
			CMemoryBufferImpl(const IMemoryBuffer& rMemoryBuffer);
			CMemoryBufferImpl(const uint8* pMemoryBuffer, const uint64 ui64BufferSize);
			virtual ~CMemoryBufferImpl(void);

			virtual boolean setSize(const uint64 ui64Size, const boolean bDiscard);
			virtual uint64 getSize(void) const;
			virtual uint8* getDirectPointer(void);
			virtual const uint8* getDirectPointer(void) const;

			_IsDerivedFromClass_Final_(IMemoryBuffer, OV_ClassId_MemoryBufferImpl);

		protected:

			uint8* m_pBuffer;
			uint64 m_ui64BufferSize;
			uint64 m_ui64AllocatedSize;
		};
	};
};

// ________________________________________________________________________________________________________________
//

CMemoryBufferImpl::CMemoryBufferImpl(void)
	:m_pBuffer(NULL)
	,m_ui64BufferSize(0)
	,m_ui64AllocatedSize(0)
{
}

CMemoryBufferImpl::CMemoryBufferImpl(const IMemoryBuffer& rMemoryBuffer)
	:m_pBuffer(NULL)
	,m_ui64BufferSize(0)
	,m_ui64AllocatedSize(0)
{
	m_pBuffer=new uint8[static_cast<size_t>(rMemoryBuffer.getSize())]; // $$$
	if(m_pBuffer)
	{
		m_ui64BufferSize=rMemoryBuffer.getSize();
		m_ui64AllocatedSize=rMemoryBuffer.getSize();
		if(rMemoryBuffer.getDirectPointer())
		{
			::memcpy(m_pBuffer, rMemoryBuffer.getDirectPointer(), static_cast<size_t>(m_ui64BufferSize)); // $$$
		}
		m_pBuffer[m_ui64AllocatedSize]=0;
	}
}

CMemoryBufferImpl::CMemoryBufferImpl(const uint8* pMemoryBuffer, const uint64 ui64BufferSize)
	:m_pBuffer(NULL)
	,m_ui64BufferSize(0)
	,m_ui64AllocatedSize(0)
{
	m_pBuffer=new uint8[static_cast<size_t>(ui64BufferSize)]; // $$$
	if(m_pBuffer)
	{
		m_ui64BufferSize=ui64BufferSize;
		m_ui64AllocatedSize=ui64BufferSize;
		if(pMemoryBuffer)
		{
			::memcpy(m_pBuffer, pMemoryBuffer, static_cast<size_t>(m_ui64BufferSize)); // $$$
		}
		m_pBuffer[m_ui64AllocatedSize]=0;
	}
}

CMemoryBufferImpl::~CMemoryBufferImpl(void)
{
	delete [] m_pBuffer;
}

uint8* CMemoryBufferImpl::getDirectPointer(void)
{
	return m_pBuffer;
}

const uint8* CMemoryBufferImpl::getDirectPointer(void) const
{
	return m_pBuffer;
}

uint64 CMemoryBufferImpl::getSize(void) const
{
	return m_ui64BufferSize;
}

boolean CMemoryBufferImpl::setSize(
	const uint64 ui64Size,
	const boolean bDiscard)
{
	if(ui64Size>m_ui64AllocatedSize)
	{
		uint8* l_pSavedBuffer=m_pBuffer;
		m_pBuffer=new uint8[static_cast<size_t>(ui64Size+1)]; // $$$
		if(!bDiscard)
		{
			::memcpy(m_pBuffer, l_pSavedBuffer, static_cast<size_t>(m_ui64BufferSize)); // $$$
		}
		delete [] l_pSavedBuffer;
		m_ui64AllocatedSize=ui64Size;
		m_pBuffer[m_ui64AllocatedSize]=0;
	}
	m_ui64BufferSize=ui64Size;
	return true;
}

// ________________________________________________________________________________________________________________
//

CMemoryBuffer::CMemoryBuffer(void)
	:m_pMemoryBufferImpl(NULL)
{
	m_pMemoryBufferImpl=new CMemoryBufferImpl();
}

CMemoryBuffer::CMemoryBuffer(const IMemoryBuffer& rMemoryBuffer)
	:m_pMemoryBufferImpl(NULL)
{
	m_pMemoryBufferImpl=new CMemoryBufferImpl(rMemoryBuffer);
}

CMemoryBuffer::CMemoryBuffer(const uint8* pMemoryBuffer, const uint64 ui64BufferSize)
	:m_pMemoryBufferImpl(NULL)
{
	m_pMemoryBufferImpl=new CMemoryBufferImpl(pMemoryBuffer, ui64BufferSize);
}

CMemoryBuffer::~CMemoryBuffer(void)
{
	delete m_pMemoryBufferImpl;
}

boolean CMemoryBuffer::setSize(const uint64 ui64Size, const boolean bDiscard)
{
	return m_pMemoryBufferImpl->setSize(ui64Size, bDiscard);
}

uint64 CMemoryBuffer::getSize(void) const
{
	return m_pMemoryBufferImpl->getSize();
}

uint8* CMemoryBuffer::getDirectPointer(void)
{
	return m_pMemoryBufferImpl->getDirectPointer();
}

const uint8* CMemoryBuffer::getDirectPointer(void) const
{
	return m_pMemoryBufferImpl->getDirectPointer();
}
