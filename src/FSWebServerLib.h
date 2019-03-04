// FSWebServerLib.h

#ifndef _FSWEBSERVERLIB_h
#define _FSWEBSERVERLIB_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <TimeLib.h>
#include "NtpClientLib.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#define RELEASE  // Comment to enable debug output

#define DBG_OUTPUT_PORT Serial

#ifndef RELEASE
#define DEBUGLOG(...) DBG_OUTPUT_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

#define CONNECTION_LED -1 // Connection LED pin (Built in). -1 to disable
#define AP_ENABLE_BUTTON -1 // Button pin to enable AP during startup for configuration. -1 to disable

// #define HIDE_CONFIG
#define CONFIG_FILE "/config.json"
#define USER_CONFIG_FILE "/userconfig.json"
#define GENERIC_CONFIG_FILE "/genericconfig.json"
#define SECRET_FILE "/secret.json"

#define JSON_CALLBACK_SIGNATURE std::function<void(AsyncWebServerRequest *request)> jsoncallback
#define REST_CALLBACK_SIGNATURE std::function<void(AsyncWebServerRequest *request)> restcallback
#define POST_CALLBACK_SIGNATURE std::function<void(AsyncWebServerRequest *request)> postcallback

typedef struct {
    String ssid;
    String password;
    IPAddress  ip;
    IPAddress  netmask;
    IPAddress  gateway;
    IPAddress  dns;
    bool dhcp;
    String ntpServerName;
    long updateNTPTimeEvery;
    long timezone;
    bool daylight;
    String deviceName;
} strConfig;

typedef struct {
    String APssid = "ESP"; // ChipID is appended to this name
    String APpassword = "12345678";
    bool APenable = false; // AP disabled by default
} strApConfig;

typedef struct {
    bool auth;
    String wwwUsername;
    String wwwPassword;
} strHTTPAuth;

class AsyncFSWebServer : public AsyncWebServer {
public:
    AsyncFSWebServer(uint16_t port);
    void begin(FS* fs);
    void handle();
    const char* getHostName();

	AsyncFSWebServer& setJSONCallback(JSON_CALLBACK_SIGNATURE);
	AsyncFSWebServer& setRESTCallback(REST_CALLBACK_SIGNATURE);
	AsyncFSWebServer& setPOSTCallback(POST_CALLBACK_SIGNATURE);
	void setUSERVERSION(String Version);

	bool save_user_config(String name, String value);
	bool load_user_config(String name, String &value);
	bool save_user_config(String name, int value);
	bool load_user_config(String name, int &value);
	bool save_user_config(String name, float value);
	bool load_user_config(String name, float &value);
	bool save_user_config(String name, long value);
	bool load_user_config(String name, long &value);
	static String urldecode(String input); // (based on https://code.google.com/p/avr-netino/)


private:
	JSON_CALLBACK_SIGNATURE;
	REST_CALLBACK_SIGNATURE;
	POST_CALLBACK_SIGNATURE;

protected:
    strConfig _config; // General and WiFi configuration
    strApConfig _apConfig; // Static AP config settings
    strHTTPAuth _httpAuth;
    FS* _fs;
    long wifiDisconnectedSince = 0;
    String _browserMD5 = "";
    uint32_t _updateSize = 0;

	WiFiEventHandler onStationModeConnectedHandler, onStationModeDisconnectedHandler, onStationModeGotIPHandler;

    //uint currentWifiStatus;

    Ticker _secondTk;
    bool _secondFlag;

    AsyncEventSource _evs = AsyncEventSource("/events");

    void sendTimeData();
    bool load_config();
    void defaultConfig();
    bool save_config();
	// bool load_generic_config()
    bool loadHTTPAuth();
    bool saveHTTPAuth();
    void configureWifiAP();
    void configureWifi();
    void ConfigureOTA(String password);
    void serverInit();

    void onWiFiConnected(WiFiEventStationModeConnected data);
	void onWiFiDisconnected(WiFiEventStationModeDisconnected data);
	void onWiFiConnectedGotIP(WiFiEventStationModeGotIP data);

    static void s_secondTick(void* arg);

    String getMacAddress();

    bool checkAuth(AsyncWebServerRequest *request);
    void handleFileList(AsyncWebServerRequest *request);
    //void handleFileRead_edit_html(AsyncWebServerRequest *request);
    bool handleFileRead(String path, AsyncWebServerRequest *request);
    void handleFileCreate(AsyncWebServerRequest *request);
    void handleFileDelete(AsyncWebServerRequest *request);
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void send_general_configuration_values_html(AsyncWebServerRequest *request);
    void send_network_configuration_values_html(AsyncWebServerRequest *request);
    void send_connection_state_values_html(AsyncWebServerRequest *request);
    void send_information_values_html(AsyncWebServerRequest *request);
    void send_NTP_configuration_values_html(AsyncWebServerRequest *request);
    void send_network_configuration_html(AsyncWebServerRequest *request);
    void send_general_configuration_html(AsyncWebServerRequest *request);
    void send_NTP_configuration_html(AsyncWebServerRequest *request);
    void restart_esp(AsyncWebServerRequest *request);
    void send_wwwauth_configuration_values_html(AsyncWebServerRequest *request);
    void send_wwwauth_configuration_html(AsyncWebServerRequest *request);
    void send_update_firmware_values_html(AsyncWebServerRequest *request);
    void setUpdateMD5(AsyncWebServerRequest *request);
    void updateFirmware(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
	void handle_rest_config(AsyncWebServerRequest *request);
	void post_rest_config(AsyncWebServerRequest *request);


 //   static String urldecode(String input); // (based on https://code.google.com/p/avr-netino/)
    static unsigned char h2int(char c);
    static boolean checkRange(String Value);
};

extern AsyncFSWebServer ESPHTTPServer;

#endif // _FSWEBSERVERLIB_h
