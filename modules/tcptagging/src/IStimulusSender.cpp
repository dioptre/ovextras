
#include "CStimulusSender.h"

OV_API TCPTagging::IStimulusSender* TCPTagging::createStimulusSender(void)
{
	return new TCPTagging::CStimulusSender();
}

