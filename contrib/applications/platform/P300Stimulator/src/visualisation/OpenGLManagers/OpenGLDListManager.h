#ifndef __OpenGLDListManager_OV_H__
#define __OpenGLDListManager_OV_H__

#include "OpenGLResourceManager.h"

namespace OpenViBEApplications
{
	class OpenGLDListManager : public OpenGLResourceManager {
	public:
		OpenGLDListManager(OpenViBE::uint32 n);// : OpenGLResourceManager(n) {}
		OpenGLDListManager(OpenGLDListManager* gl_manager) : OpenGLResourceManager(gl_manager) {}
		virtual ~OpenGLDListManager();
		virtual OpenGLDListManager* clone() const { return new OpenGLDListManager((OpenGLDListManager*)this); }
		
		//virtual void createResources();
		virtual void deleteResources();
		
	protected:
		virtual std::map<GLuint, OpenViBE::uint32>& getResourceMap();
		virtual void _createResources();
		virtual void _deleteResource(GLuint* resource_id);		
		//static void forwarder(OpenGLResourceManager* context);
		
	protected:
		static std::map<GLuint, OpenViBE::uint32> m_mIdCountMap;
	};

};

#endif