#pragma once

#include "CStreamer.h"
#include "JPEGSamples.h"

#ifdef INCLUDE_SIMDATA
class SimStreamer : public CStreamer {
    bool m_showBig;

public:
    SimStreamer(bool showBig);

    virtual void streamImage(uint32_t curMsec);
};
#endif
