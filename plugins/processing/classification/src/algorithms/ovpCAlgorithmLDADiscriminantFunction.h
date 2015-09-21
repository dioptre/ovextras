#ifndef OVPCALGORITHMLDACOMPUTATIONHELPER_H
#define OVPCALGORITHMLDACOMPUTATIONHELPER_H

#if defined TARGET_HAS_ThirdPartyEIGEN

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>

#include <Eigen/Eigenvalues>

#include "ovpCAlgorithmClassifierLDA.h"
#include <xml/IXMLNode.h>

namespace OpenViBEPlugins
{
	namespace Classification
	{
		//The purpose of this class is to compute the "membership" of a vector
		class CAlgorithmLDADiscriminantFunction
		{
		public:
			CAlgorithmLDADiscriminantFunction();

			void setWeight(const Eigen::VectorXd &rWeigth);
			void setBias(OpenViBE::float64 f64Bias);

			//Return the class membership of the feature vector
			OpenViBE::float64 getValue(const Eigen::VectorXd &rFeatureVector);
			OpenViBE::uint32 getWeightVectorSize(void);

			OpenViBE::boolean loadConfiguration(const XML::IXMLNode* pConfiguration);
			XML::IXMLNode* getConfiguration(void);

		private:
			OpenViBE::uint32 m_ui32NumCol;
			OpenViBE::float64 m_f64Bias;
			Eigen::VectorXd   m_oWeight;

		};
	}
}

#endif
#endif // OVPCALGORITHMLDACOMPUTATIONHELPER_H
