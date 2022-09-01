
#include "OV2640Streamer.h"
#include <assert.h>



OV2640Streamer::OV2640Streamer(SOCKET aClient, OV2640 &cam) : CStreamer(aClient, cam.getWidth(), cam.getHeight()), m_cam(cam)
{
    DEBUG_PRINT("Created streamer width=%d, height=%d\n", cam.getWidth(), cam.getHeight());
}

void OV2640Streamer::streamImage(uint32_t curMsec, camera_fb_t *frame)
{
     m_cam.run();// queue up a read for next time
     streamFrame(frame->buf, frame->len, curMsec);
     m_cam.done();
}
