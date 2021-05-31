//NOTE! This example is intended for the ESP32-CAM from Ai-Thinker. Other ESP32-based camera boards are supported,
//but will require modification to this example. This is just a bare-bones example of Micro-RTSP, no web server
//or other features are provided. Although this uses the Arduino framework it is intended to be built with
//PlatformIO.

#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <OV2640Streamer.h>
#include <CRtspSession.h>

//device specific settings
#define DEVICE_NAME "ESP32CAM1"
#define ENABLE_OTA

//WIFI settings
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

//common camera settings, see below for other camera options
#define CAM_FRAMESIZE FRAMESIZE_VGA //enough resolution for a simple security cam
#define CAM_FRAMEBUFFERS 2 //need double-buffers for reasonable framerate
#define CAM_QUALITY 12 //0 to 63, with higher being lower-quality (and less bandwidth), 12 seems reasonable
#define CAM_FRAMERATE 10 //frames per second, only going to get about 10fps with the ESP32
#define CAM_VERTICAL_FLIP 1 //1 to enable
#define CAM_HORIZONTAL_MIRROR 1 //1 to enable

#ifdef ENABLE_OTA
//Arduino OTA Config
#define OTA_NAME DEVICE_NAME
#define OTA_PORT 3232 //default
#define OTA_PASS NULL

#include <ArduinoOTA.h>
#endif

OV2640 cam;
CStreamer *streamer = NULL;
CRtspSession *session = NULL;
WiFiClient client;
WiFiServer rtspServer(8554);
const uint32_t msecPerFrame = (1000 / CAM_FRAMERATE);
uint32_t lastFrameTime = 0;

// cppcheck-suppress unusedFunction
void setup()
{
    Serial.begin(115200);

    //turn off the camera LED
    pinMode(4, OUTPUT);
    digitalWrite(4, 0); //off

    camera_config_t config = esp32cam_aithinker_config;
    config.frame_size = CAM_FRAMESIZE;
    config.jpeg_quality = CAM_QUALITY; 
    config.fb_count = CAM_FRAMEBUFFERS;
    cam.init(config);

    //setup other camera options
    sensor_t * s = esp_camera_sensor_get();
    //s->set_brightness(s, 0);     // -2 to 2
    //s->set_contrast(s, 0);       // -2 to 2
    //s->set_saturation(s, 0);     // -2 to 2
    //s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 0);       // 0 = disable , 1 = enable //auto white-balance seems to be more trouble than its worth
    //s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    //s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    //s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    //s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    //s->set_ae_level(s, 0);       // -2 to 2
    //s->set_aec_value(s, 300);    // 0 to 1200
    //s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    //s->set_agc_gain(s, 0);       // 0 to 30
    //s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    //s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    //s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    //s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    //s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, CAM_HORIZONTAL_MIRROR); // 0 = disable , 1 = enable
    s->set_vflip(s, CAM_VERTICAL_FLIP);       // 0 = disable , 1 = enable
    //s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    //s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    
    IPAddress ip;

    Serial.print(F("WiFi start..."));
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_NAME);
    esp_wifi_set_ps(WIFI_PS_NONE); //helps stability and performance
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }

    ip = WiFi.localIP();
    Serial.println(F("WiFi connected"));
    Serial.println("");
    Serial.println(ip);
    Serial.print(F("Stream Link: rtsp://"));
    Serial.print(ip);
    Serial.println(F(":8554/mjpeg/1"));

#ifdef ENABLE_OTA
    //Setup OTA
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(OTA_NAME);
    ArduinoOTA.setPassword(OTA_PASS);
    ArduinoOTA.begin();
#endif

    lastFrameTime = millis();
    rtspServer.begin();
}

// cppcheck-suppress unusedFunction
void loop()
{
#ifdef ENABLE_OTA
    ArduinoOTA.handle();
#endif

    // If we have an active client connection, just service that until gone
    if(streamer)
    {
        session->handleRequests(0); // we don't use a timeout here,
        // instead we send only if we have new enough frames

        uint32_t now = millis();
        if((now > (lastFrameTime + msecPerFrame)) || (now < lastFrameTime))
        {
            session->broadcastCurrentFrame(now);
            lastFrameTime = now;
        }

        if(session->m_stopped)
        {
            delete session;
            delete streamer;
            session = NULL;
            streamer = NULL;
        }
    }
    else
    {
        client = rtspServer.accept();

        if(client)
        {
            streamer = new OV2640Streamer(&client, cam);
            session = new CRtspSession(&client, streamer);
        }
    }
}
