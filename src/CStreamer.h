#pragma once

#include <esp_camera.h>
#include "platglue.h"

#define RTPBUF_SIZE 2048

typedef unsigned const char *BufPtr;

class CStreamer
{
public:
    CStreamer(SOCKET aClient, u_short width, u_short height);
    virtual ~CStreamer();

    void    InitTransport(u_short aRtpPort, u_short aRtcpPort, bool TCP);
    u_short GetRtpServerPort();
    u_short GetRtcpServerPort();

    virtual void streamImage(uint32_t curMsec, camera_fb_t *frame) = 0; // send a new image to the client
    void setURI( String hostport, String pres = "mjpeg", String stream = "1" ); // set URI parts for sessions to use.
    String getURIHost(){ return m_URIHost; }; // for getting things back by sessions
    String getURIPresentation(){ return m_URIPresentation; };
    String getURIStream(){ return m_URIStream; };

protected:

    void    streamFrame(unsigned const char *data, uint32_t dataLen, uint32_t curMsec);

    String  m_URIHost; // Host:port URI part that client should use to connect. also it is reported in session answers where appropriate.
    String  m_URIPresentation; // name of presentation part of URI. sessions will check if client used correct one
    String  m_URIStream; // stream part of the URI.

private:
    int    SendRtpPacket(unsigned const char *jpeg, int jpegLen, int fragmentOffset, BufPtr quant0tbl = NULL, BufPtr quant1tbl = NULL);// returns new fragmentOffset or 0 if finished with frame

    UDPSOCKET m_RtpSocket;           // RTP socket for streaming RTP packets to client
    UDPSOCKET m_RtcpSocket;          // RTCP socket for sending/receiving RTCP packages

    uint16_t m_RtpClientPort;      // RTP receiver port on client (in host byte order!)
    uint16_t m_RtcpClientPort;     // RTCP receiver port on client (in host byte order!)
    IPPORT m_RtpServerPort;      // RTP sender port on server
    IPPORT m_RtcpServerPort;     // RTCP sender port on server

    u_short m_SequenceNumber;
    uint32_t m_Timestamp;
    int m_SendIdx;
    bool m_TCPTransport;
    SOCKET m_Client;
    uint32_t m_prevMsec;

    u_short m_width; // image data info
    u_short m_height;
        
    char * RtpBuf;
};



// When JPEG is stored as a file it is wrapped in a container
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped
// returns true if the file seems to be valid jpeg
// If quant tables can be found they will be stored in qtable0/1
bool decodeJPEGfile(BufPtr *start, uint32_t *len, BufPtr *qtable0, BufPtr *qtable1);
bool findJPEGheader(BufPtr *start, uint32_t *len, uint8_t marker);

// Given a jpeg ptr pointing to a pair of length bytes, advance the pointer to
// the next 0xff marker byte
void nextJpegBlock(BufPtr *start);
