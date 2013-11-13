#if defined(TARGET_HAS_ThirdPartyEIGEN)

#ifndef __WindowFunctions_H__
#define __WindowFunctions_H__

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <Eigen/Dense>
#include <cmath>

class WindowFunctions {

public:

	Eigen::VectorXd bartlett(OpenViBE::uint32 ui32WinSize);
	Eigen::VectorXd hamming(OpenViBE::uint32 ui32WinSize);
	Eigen::VectorXd hann(OpenViBE::uint32 ui32WinSize);
	Eigen::VectorXd parzen(OpenViBE::uint32 ui32WinSize);
	Eigen::VectorXd welch(OpenViBE::uint32 ui32WinSize);

};


#endif //__WindowFunctions_H__
#endif //TARGET_HAS_ThirdPartyEIGEN
