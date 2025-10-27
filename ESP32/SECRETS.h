// --- TCP CLIENT / WIFI CONFIGURATION (NEW) ---
#include <WiFi.h>
#include <WiFiClient.h>

const char* WIFI_SSID = "Galaxy S23 FE EA6B";
const char* WIFI_PASSWORD = "ykje79jk72vchbp";

// IMPORTANT: Replace this IP with your server's actual local IP address
IPAddress SERVER_IP(10,0,0,8);
const int SERVER_PORT = 8080;

