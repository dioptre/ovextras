#include "OpenGLResourceManager.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

OpenGLResourceManager::OpenGLResourceManager(OpenViBE::uint32 n) : m_uiNumberOfGLResources(n)
{
	m_uiGLResourceIds = new GLuint[m_uiNumberOfGLResources];
}

OpenGLResourceManager::OpenGLResourceManager(OpenGLResourceManager* gl_manager)
{
	m_uiNumberOfGLResources = gl_manager->m_uiNumberOfGLResources;
	m_uiGLResourceIds = new GLuint[m_uiNumberOfGLResources];
	std::map<GLuint, uint32>& l_mIdCoutMap = gl_manager->getResourceMap();
	for (uint32 i=0; i<m_uiNumberOfGLResources; i++)
	{
		this->m_uiGLResourceIds[i] = gl_manager->m_uiGLResourceIds[i];
		l_mIdCoutMap[ gl_manager->m_uiGLResourceIds[i] ]++;
	}
}

OpenGLResourceManager::~OpenGLResourceManager()
{
	delete[] m_uiGLResourceIds;
}

void OpenGLResourceManager::createResources(OpenGLResourceManager* context)
{
	//context->m_uiNumberOfGLResources = n;
	//context->m_uiGLResourceIds = new GLuint[context->m_uiNumberOfGLResources];
	context->_createResources();
	
	std::map<GLuint, uint32>& l_mIdCoutMap = context->getResourceMap();
	std::map<GLuint, uint32>::iterator l_Iterator;
	for (uint32 i=0; i<context->m_uiNumberOfGLResources; i++)
	{
		l_Iterator = l_mIdCoutMap.find(context->m_uiGLResourceIds[i]);
		if (l_Iterator!=l_mIdCoutMap.end())
		{
			std::cout << "Creating new opengl resource, resource id " << context->m_uiGLResourceIds[i] << " should not exist\n";
			l_mIdCoutMap[ context->m_uiGLResourceIds[i] ]++;
		}
		else
			l_mIdCoutMap.insert(std::pair<GLuint,uint32>(context->m_uiGLResourceIds[i],1));
	}	
}
void OpenGLResourceManager::deleteResources(OpenGLResourceManager* context)
{
	std::map<GLuint, uint32>& l_mIdCoutMap = context->getResourceMap();
	std::map<GLuint, uint32>::iterator l_Iterator;
	for (uint32 i=0; i<context->m_uiNumberOfGLResources; i++)
	{
		l_Iterator = l_mIdCoutMap.find(context->m_uiGLResourceIds[i]);
		if (l_Iterator!=l_mIdCoutMap.end())
		{
			//std::cout << "OpenGLResourceManager:: Deleting opengl resources with id " << context->m_uiGLResourceIds[i] 
			//<< " and count " << l_mIdCoutMap[ context->m_uiGLResourceIds[i] ] << "\n";
			if (--l_mIdCoutMap[ context->m_uiGLResourceIds[i] ] == 0)
			{
				context->_deleteResource(context->m_uiGLResourceIds+i);
				l_mIdCoutMap.erase(l_Iterator);
			}
		}
		else
			context->_deleteResource(context->m_uiGLResourceIds+i);
	}	
}