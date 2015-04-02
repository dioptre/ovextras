#ifndef OVPCALGORITHMLDACOMPUTATIONHELPER_H
#define OVPCALGORITHMLDACOMPUTATIONHELPER_H

#define TARGET_HAS_ThirdPartyEIGEN
#if defined TARGET_HAS_ThirdPartyEIGEN

#include <Eigen/Eigenvalues>

#include "ovpCAlgorithmClassifierLDA.h"
#include <xml/IXMLNode.h>

namespace OpenViBEPlugins
{
	namespace Classification
	{
		//The purpose of this class is to
		class CAlgorithmLDAComputationHelper
		{
		public:
			CAlgorithmLDAComputationHelper();

			void setWeight(Eigen::MatrixXd &rWeigth);
			void setBias(OpenViBE::float64 f64Bias);

			OpenViBE::float64 getValue(Eigen::MatrixXd &rFeatureVector);

			OpenViBE::boolean loadConfiguration(XML::IXMLNode* pConfiguration);
			XML::IXMLNode* getConfiguration();

		private:
			OpenViBE::uint32 m_ui32NumCol;
			OpenViBE::float64 m_f64Bias;
			Eigen::MatrixXd   m_oWeight;

		};
	}
}

#endif
#endif // OVPCALGORITHMLDACOMPUTATIONHELPER_H
