#define bootX 1
/***DEBUG_PORT***/
#define DEBUG 1
#ifdef DEBUG
#define DEBUG_PORT Serial 
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_WRITE(x) Serial.write(x)
#define DEBUG_PRINTx(x, y) Serial.print(x, y)
#define DEBUG_PRINTDEC(x) Serial.print(x, DEC)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTLNx(x, y) Serial.println(x, y)
#else
#define DEBUG_PORT
#define DEBUG_WRITE(x)
#define DEBUG_PRINT(x)
#define DEBUG_PRINTx(x, y)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTLNx(x, y)
#endif
////////////////
#include "EEPROM.h" 
#include <RTClib.h>
RTC_Millis rtcSoft;
#include <SPI.h>
#include "FS.h"
#include "SPIFFS.h"
// #define ssl
// #ifdef ssl
// #include <WiFiClientSecure.h>
//#include <credentials.h>
//WiFiClientSecure client1;
//WiFiClientSecure client_1;
//#else
#include <WiFi.h>
WiFiClient client1;
//WiFiClient client_1;
//#endif
#include <ESP32WebServer.h>
#define AP
ESP32WebServer server(80); //web server
/**SPIFF**/
#define fun_spiff
File logFile; 
/**** REGISTERS ****/
//#define devBoard //comment if is final board
#ifdef  devBoard
#define wifiled 5                    //32 final board
#define NFCPOWER 27 // 5 final board
#else
#define wifiled 32                    //32 final board
#define NFCPOWER 5 // 5 final board
#endif
#define RetardoAntirebotePulsador 25 
#define salidabajoactiva
#ifdef salidabajoactiva 
#define OFF_bit 1
#define ON_bit 0
#define OFF HIGH
#define ON LOW
#endif
#ifdef salidaaltoactiva 
#define OFF_bit 0
#define ON_bit 1
#define OFF LOW
#define ON HIGH
#endif
/****VARIABLES****/
/****fun_web Variables****/
bool noAP = 1;
int tnoAP = 0;
String webpage = "";     
bool SD_present = false; 
/*************/
unsigned long actualizar_hora = 0;
unsigned long t_ultima_accion = 0;
unsigned long cuentaSegundos = 0;
uint8_t count = 0;
uint8_t countRtc = 0;
bool connected = 0;
//----------------------------------------------//
//           TIME Variables                    //
//----------------------------------------------//
char Dia_semana[][10] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"}; /*Su=1 Mo=2 Tu=3 We=4 Th=5 Fr=6 Sa=7 */
int dia_semana_int = 0;
uint8_t segundo; /* seconds */
uint8_t minuto;  /* minutes */
uint8_t hora;    /* hours */
uint8_t dia;     /* day of the week */
uint8_t mes;     /* month */
int16_t ano;     /* year */
uint8_t prevmes; /* month */
int16_t prevano; /* year */
/*Auxiliares*/
uint8_t segundo1; /* seconds */
uint8_t minuto1;  /* minutes */
uint8_t hora1;    /* hours */
uint8_t dia1;     /* day of the week */
uint8_t mes1;     /* month */
int16_t ano1;     /* year */
DateTime now;
//----------------------------------------------//
//             General purpose variables         //
//----------------------------------------------//
int prevmin = 0, prevday = 0;
int rtcrestart = 0; //
unsigned long timeout1 = 0, timeout2 = 0;
bool webwork = 0; 
bool envia = 0;
int change = 0;  //if changes happens sends data
int modo_automatico = 0; 
int numero_horarios = 0;
bool sal = 0;
int ciclo_actual = 0;
int Sistema_apagado = 0; //
String ch, ch1, ch2, ch3;
int chlength = 0;
String formdata, vacio = "";
///-------------Set wifi network parameters---------------------
/****WIFI****/
char ssid[60] = "orbittas";     //WIFI SSID
char password[60] = "20075194"; //WIFI PASSWORD
char ssid2[60] = "Speaker";       //AP SSID
char password2[60] = "12345678"; //AP PASSWORD
String WRSSI;
/****SPIFF****/
long int lastposition2 = 0; 
long int lastposition = 0;  
long int lastposition3 = 0; 
long int lastposition4 = 0; 
//////MQTT
#define USEWIFIMQTT        
#include "PubSubClient.h" 
WiFiClient espClient;     
PubSubClient mqttclient(espClient); 
/****Variables WIFI MQTT****/
//char host[120] = "broker.mqttdashboard.com";
char datarecvd[512]; 
int reconnect = 0;   
/****parametros mqtt ****/
char MQTTHost[120] = "broker.mqttdashboard.com"; 
char MQTTPort[6] = "1883";                       
char MQTTClientID[60] = "";                      
char MQTTTopic[60] = "";                         
char MQTTTopic2[60] = "";                        
char MQTTUsername[60] = "*";                     
char MQTTPassword[120] = "*";                    
uint32_t ChipId32, wait = 0, wait2 = 0, wait3 = 0;
uint16_t ChipId16;
uint64_t ChipId;
String chipid = "";
String clientId = "SPEAKER/";
///changes
int modo_automatico_aux;
int modo_nowc = 1;
int modo_nowc_aux = 0;
///changes mqtt
int modo_automatico_aux1;
int modo_nowc1 = 1;
int modo_nowc_aux1 = 0;
char ssid_aux[60] = "";      //WIFI SSID
char password_aux[60] = "";  //WIFI PASSWORD
char ssid2_aux[60] = "";     //AP SSID
char password2_aux[60] = ""; //AP PASSWORD
char host_aux[120] = "";
char MQTTUsername_aux[60] = "";
char MQTTPassword_aux[120] = "";
char MQTTHost_aux[120] = ""; 
char MQTTPort_aux[6] = "";
int changev = 0;  
int changev2 = 0; 
int inicio = 0;
int proximo_ciclo_aux = 0;
int proximo_ciclo_aux1 = 0;
long int mqttdelay = 0, blikDelay = 0;
int env_prox = 0; 
bool solicitud_web = 0;
bool envia_horarios = 0;
bool guardarHorarios = 0;
int subscribed = 0; 
bool rtcFalla = 0;
bool wifiLedState = false;
bool apMode = 0;
bool apActivate=0;
bool guardarAp = 0;
String ipRed = "0.0.0.0";
bool cambioIp = 0;
long int minutosEnApMode = 0;
int minutoAuxAp = 0;
bool cambioFechaHora = false;



bool bussyMqtt = 0;
/////
String Shora = "", Sfecha = "", Sday = "", Smonth = "", Syear = "", Shr = "", Smin = "";
/////server protocol
bool serverPoll = 0; /// 
char devName[11] = "room-x";

///mp3
const char *mp3host = "airspectrum.cdnstream1.com";//"sdrorbittas.sytes.net";
const char *mp3path = "/1648_128";//"/audio/andrew_rayel_impulse.mp3";
int mp3port = 8114;//3412;

// Define the version number, also used for webserver as Last-Modified header and to
// check version for update.  The format must be exactly as specified by the HTTP standard!
#define VERSION "Mon, 19 Oct 2020 14:12:00 GMT"
// ESP32-Radio can be updated (OTA) to the latest version from a remote server.
// The download uses the following server and files:
#define UPDATEHOST "smallenburg.nl"            // Host for software updates
#define BINFILE "/Arduino/Esp32_radio.ino.bin" // Binary file name for update software
#define TFTFILE "/Arduino/ESP32-Radio.tft"     // Binary file name for update NEXTION image


#include <nvs.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <esp_partition.h>
#include <driver/adc.h>
#include <Update.h>
#include <base64.h>

// Number of entries in the queue
#define QSIZ 400
// Debug buffer size
#define DEBUG_BUFFER_SIZE 150
#define NVSBUFSIZE 150
// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Note that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#define NAME "SPEAKER"
// Max number of presets in preferences
#define MAXPRESETS 200
// Maximum number of MQTT reconnects before give-up
#define MAXMQTTCONNECTS 5
// Adjust size of buffer to the longest expected string for nvsgetstr
#define NVSBUFSIZE 150
// Position (column) of time in topline relative to end
#define TIMEPOS -52
// SPI speed for SD card
#define SDSPEED 1000000
// Size of metaline buffer
#define METASIZ 1024
// Max. number of NVS keys in table
#define MAXKEYS 200
// Time-out [sec] for blanking TFT display (BL pin)
#define BL_TIME 45
//
// Subscription topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
// by the the mqttprefix in the preferences.  The next definition will yield the topic
// "ESP32Radio/command" if mqttprefix is "ESP32Radio".
#define MQTT_SUBTOPIC "command" // Command to receive from MQTT
//
#define otaclient mp3client // OTA uses mp3client for connection to host
//**************************************************************************************************
// Forward declaration and prototypes of various functions.                                        *
//**************************************************************************************************
void displaytime(const char *str, uint16_t color = 0xFFFF);
void showstreamtitle(const char *ml, bool full = false);
void handlebyte_ch(uint8_t b);
void handleFSf(const String &pagename);
void handleCmd();
char *dbgprint(const char *format, ...);
const char *analyzeCmd(const char *str);
const char *analyzeCmd(const char *par, const char *val);
void chomp(String &str);
String httpheader(String contentstype);
bool nvssearch(const char *key);
void mp3loop();
void stop_mp3client();

void playtask(void *parameter); // Task to play the stream
void spftask(void *parameter);  // Task for special functions
void gettime();
void reservepin(int8_t rpinnr);
void claimSPI(const char *p); // Claim SPI bus for exclusive access
void releaseSPI();            // Release the claim
char utf8ascii(char ascii);   // Convert UTF8 char to normal char
void utf8ascii_ip(char *s);   // In place conversion full string
String utf8ascii(const char *s);
uint32_t ssconv(const uint8_t *bytes);

//**************************************************************************************************
// Several structs and enums.                                                                      *
//**************************************************************************************************
//

enum fs_type
{
  FS_USB,
  FS_SD
}; // USB- or SD interface

struct scrseg_struct // For screen segments
{
  bool update_req; // Request update of screen
  uint16_t color;  // Textcolor
  uint16_t y;      // Begin of segment row
  uint16_t height; // Height of segment
  String str;      // String to be displayed
};

enum qdata_type
{
  QDATA,
  QSTARTSONG,
  QSTOPSONG
}; // datatyp in qdata_struct
struct qdata_struct
{
  int datatyp;                                 // Identifier
  __attribute__((aligned(4))) uint8_t buf[32]; // Buffer for chunk
};

struct ini_struct
{
  String mqttbroker;       // The name of the MQTT broker server
  String mqttprefix;       // Prefix to use for topics
  uint16_t mqttport;       // Port, default 1883
  String mqttuser;         // User for MQTT authentication
  String mqttpasswd;       // Password for MQTT authentication
  uint8_t reqvol;          // Requested volume
  uint8_t rtone[4];        // Requested bass/treble settings
  int16_t newpreset;       // Requested preset
  String clk_server;       // Server to be used for time of day clock
  int8_t clk_offset;       // Offset in hours with respect to UTC
  int8_t clk_dst;          // Number of hours shift during DST
  int8_t ir_pin;           // GPIO connected to output of IR decoder
  int8_t enc_clk_pin;      // GPIO connected to CLK of rotary encoder
  int8_t enc_dt_pin;       // GPIO connected to DT of rotary encoder
  int8_t enc_sw_pin;       // GPIO connected to SW of rotary encoder
  int8_t tft_cs_pin;       // GPIO connected to CS of TFT screen
  int8_t tft_dc_pin;       // GPIO connected to D/C or A0 of TFT screen
  int8_t tft_scl_pin;      // GPIO connected to SCL of i2c TFT screen
  int8_t tft_sda_pin;      // GPIO connected to SDA of I2C TFT screen
  int8_t tft_bl_pin;       // GPIO to activate BL of display
  int8_t tft_blx_pin;      // GPIO to activate BL of display (inversed logic)
  int8_t sd_cs_pin;        // GPIO connected to CS of SD card
  int8_t vs_cs_pin;        // GPIO connected to CS of VS1053
  int8_t vs_dcs_pin;       // GPIO connected to DCS of VS1053
  int8_t vs_dreq_pin;      // GPIO connected to DREQ of VS1053
  int8_t vs_shutdown_pin;  // GPIO to shut down the amplifier
  int8_t vs_shutdownx_pin; // GPIO to shut down the amplifier (inversed logic)
  int8_t spi_sck_pin;      // GPIO connected to SPI SCK pin
  int8_t spi_miso_pin;     // GPIO connected to SPI MISO pin
  int8_t spi_mosi_pin;     // GPIO connected to SPI MOSI pin
  int8_t ch376_cs_pin;     // GPIO connected to CH376 SS
  int8_t ch376_int_pin;    // GPIO connected to CH376 INT
  uint16_t bat0;           // ADC value for 0 percent battery charge
  uint16_t bat100;         // ADC value for 100 percent battery charge
};

struct WifiInfo_t // For list with WiFi info
{
  uint8_t inx;      // Index as in "wifi_00"
  char *ssid;       // SSID for an entry
  char *passphrase; // Passphrase for an entry
};

struct nvs_entry
{
  uint8_t Ns;    // Namespace ID
  uint8_t Type;  // Type of value
  uint8_t Span;  // Number of entries used for this item
  uint8_t Rvs;   // Reserved, should be 0xFF
  uint32_t CRC;  // CRC
  char Key[16];  // Key in Ascii
  uint64_t Data; // Data in entry
};

struct nvs_page // For nvs entries
{               // 1 page is 4096 bytes
  uint32_t State;
  uint32_t Seqnr;
  uint32_t Unused[5];
  uint32_t CRC;
  uint8_t Bitmap[32];
  nvs_entry Entry[126];
};

struct keyname_t // For keys in NVS
{
  char Key[16]; // Max length is 15 plus delimeter
};

//**************************************************************************************************
// Global data section.                                                                            *
//**************************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is              *
// saved in the preferences by the webinterface.  On restart the new data will                     *
// de read from these preferences.                                                                 *
// Items in ini_block can be changed by commands from webserver/MQTT/Serial.                       *
//**************************************************************************************************

enum display_t
{
  T_UNDEFINED,
  T_BLUETFT,
  T_OLED, // Various types of display
  T_DUMMYTFT,
  T_LCD1602I2C,
  T_LCD2004I2C,
  T_ILI9341,
  T_NEXTION
};

enum datamode_t
{
  INIT = 0x1,
  HEADER = 0x2,
  DATA = 0x4, // State for datastream
  METADATA = 0x8,
  PLAYLISTINIT = 0x10,
  PLAYLISTHEADER = 0x20,
  PLAYLISTDATA = 0x40,
  STOPREQD = 0x80,
  STOPPED = 0x100
};

// Global variables

int numSsid;                          // Number of available WiFi networks
WiFiMulti wifiMulti;                  // Possible WiFi networks
ini_struct ini_block;                 // Holds configurable data
WiFiServer cmdserver(80);             // Instance of embedded webserver, port 80
WiFiClient mp3client;                 // An instance of the mp3 client, also used for OTA
WiFiClient cmdclient;                 // An instance of the client for commands


HardwareSerial *nxtserial = NULL;     // Serial port for NEXTION (if defined)
TaskHandle_t maintask;                // Taskhandle for main task
TaskHandle_t xplaytask;               // Task handle for playtask
TaskHandle_t xspftask;                // Task handle for special functions
SemaphoreHandle_t SPIsem = NULL;      // For exclusive SPI usage
hw_timer_t *timer = NULL;             // For timer
char timetxt[9];                      // Converted timeinfo
char cmd[130];                        // Command from MQTT or Serial
uint8_t tmpbuff[6000];                // Input buffer for mp3 or data stream
QueueHandle_t dataqueue;              // Queue for mp3 datastream
QueueHandle_t spfqueue;               // Queue for special functions
qdata_struct outchunk;                // Data to queue
qdata_struct inchunk;                 // Data from queue
uint8_t *outqp = outchunk.buf;        // Pointer to buffer in outchunk
uint32_t totalcount = 0;              // Counter mp3 data
datamode_t datamode;                  // State of datastream
int metacount;                        // Number of bytes in metadata
int datacount;                        // Counter databytes before metadata
char metalinebf[METASIZ + 1];         // Buffer for metaline/ID3 tags
int16_t metalinebfx;                  // Index for metalinebf
String icystreamtitle;                // Streamtitle from metadata
String icyname;                       // Icecast station name
String ipaddress;                     // Own IP-address
int bitrate;                          // Bitrate in kb/sec
int mbitrate;                         // Measured bitrate
int metaint = 0;                      // Number of databytes between metadata
int16_t currentpreset = -1;           // Preset station playing
//String host;                          // The URL to connect to or file to play
String playlist;                      // The URL of the specified playlist
bool hostreq = false;                 // Request for new host
bool reqtone = false;                 // New tone setting requested
bool muteflag = false;                // Mute output
bool resetreq = false;                // Request to reset the ESP32
bool updatereq = false;               // Request to update software from remote host
bool NetworkFound = false;            // True if WiFi network connected
bool mqtt_on = false;                 // MQTT in use
String networks;                      // Found networks in the surrounding
uint16_t mqttcount = 0;               // Counter MAXMQTTCONNECTS
int8_t playingstat = 0;               // 1 if radio is playing (for MQTT)
int16_t playlist_num = 0;             // Nonzero for selection from playlist
fs_type usb_sd = FS_USB;              // SD or USB interface
uint32_t mp3filelength;               // File length
bool localfile = false;               // Play from local mp3-file or not
bool chunked = false;                 // Station provides chunked transfer
int chunkcount = 0;                   // Counter for chunked transfer
String http_getcmd;                   // Contents of last GET command
String http_rqfile;                   // Requested file
bool http_response_flag = false;      // Response required
uint16_t ir_value = 0;                // IR code
uint32_t ir_0 = 550;                  // Average duration of an IR short pulse
uint32_t ir_1 = 1650;                 // Average duration of an IR long pulse
struct tm timeinfo;                   // Will be filled by NTP server
bool time_req = false;                // Set time requested
uint16_t adcval;                      // ADC value (battery voltage)
uint32_t clength;                     // Content length found in http header
uint32_t max_mp3loop_time = 0;        // To check max handling time in mp3loop (msec)
int16_t scanios;                      // TEST*TEST*TEST
int16_t scaniocount;                  // TEST*TEST*TEST
uint16_t bltimer = 0;                 // Backlight time-out counter
display_t displaytype = T_UNDEFINED;  // Display type
std::vector<WifiInfo_t> wifilist;     // List with wifi_xx info
// nvs stuff
nvs_page nvsbuf;                 // Space for 1 page of NVS info
const esp_partition_t *nvs;      // Pointer to partition struct
esp_err_t nvserr;                // Error code from nvs functions
uint32_t nvshandle = 0;          // Handle for nvs access
uint8_t namespace_ID;            // Namespace ID found
char nvskeys[MAXKEYS][16];       // Space for NVS keys
std::vector<keyname_t> keynames; // Keynames in NVS
// Rotary encoder stuff
#define sv DRAM_ATTR static volatile
sv uint16_t clickcount = 0;     // Incremented per encoder click
sv int16_t rotationcount = 0;   // Current position of rotary switch
sv uint16_t enc_inactivity = 0; // Time inactive
sv bool singleclick = false;    // True if single click detected
sv bool doubleclick = false;    // True if double click detected
sv bool tripleclick = false;    // True if triple click detected
sv bool longclick = false;      // True if longclick detected
enum enc_menu_t
{
  VOLUME,
  PRESET,
  TRACK
};                                 // State for rotary encoder menu
enc_menu_t enc_menu_mode = VOLUME; // Default is VOLUME mode

//
struct progpin_struct // For programmable input pins
{
  int8_t gpio;    // Pin number
  bool reserved;  // Reserved for connected devices
  bool avail;     // Pin is available for a command
  String command; // Command to execute when activated
                  // Example: "uppreset=1"
  bool cur;       // Current state, true = HIGH, false = LOW
};

progpin_struct progpin[] = // Input pins and programmed function
    {
        {0, false, false, "", false},
        //{  1, true,  false,  "", false },                    // Reserved for TX Serial output
        {2, false, false, "", false},
        //{  3, true,  false,  "", false },                    // Reserved for RX Serial input
        {4, false, false, "", false},
        {5, false, false, "", false},
        //{  6, true,  false,  "", false },                    // Reserved for FLASH SCK
        //{  7, true,  false,  "", false },                    // Reserved for FLASH D0
        //{  8, true,  false,  "", false },                    // Reserved for FLASH D1
        //{  9, true,  false,  "", false },                    // Reserved for FLASH D2
        //{ 10, true,  false,  "", false },                    // Reserved for FLASH D3
        //{ 11, true,  false,  "", false },                    // Reserved for FLASH CMD
        {12, false, false, "", false},
        {13, false, false, "", false},
        {14, false, false, "", false},
        {15, false, false, "", false},
        {16, false, false, "", false}, // May be UART 2 RX for Nextion
        {17, false, false, "", false}, // May be UART 2 TX for Nextion
        {18, false, false, "", false}, // Default for SPI CLK
        {19, false, false, "", false}, // Default for SPI MISO
        //{ 20, true,  false,  "", false },                    // Not exposed on DEV board
        {21, false, false, "", false}, // Also Wire SDA
        {22, false, false, "", false}, // Also Wire SCL
        {23, false, false, "", false}, // Default for SPI MOSI
        //{ 24, true,  false,  "", false },                    // Not exposed on DEV board
        {25, false, false, "", false},
        {26, false, false, "", false},
        {27, false, false, "", false},
        //{ 28, true,  false,  "", false },                    // Not exposed on DEV board
        //{ 29, true,  false,  "", false },                    // Not exposed on DEV board
        //{ 30, true,  false,  "", false },                    // Not exposed on DEV board
        //{ 31, true,  false,  "", false },                    // Not exposed on DEV board
        {32, false, false, "", false},
        {33, false, false, "", false},
        {34, false, false, "", false}, // Note, no internal pull-up
        {35, false, false, "", false}, // Note, no internal pull-up
        //{ 36, true,  false,  "", false },                    // Reserved for ADC battery level
        {39, false, false, "", false}, // Note, no internal pull-up
        {-1, false, false, "", false}  // End of list
};

struct touchpin_struct // For programmable input pins
{
  int8_t gpio;    // Pin number GPIO
  bool reserved;  // Reserved for connected devices
  bool avail;     // Pin is available for a command
  String command; // Command to execute when activated
  // Example: "uppreset=1"
  bool cur;      // Current state, true = HIGH, false = LOW
  int16_t count; // Counter number of times low level
};
touchpin_struct touchpin[] = // Touch pins and programmed function
    {
        {4, false, false, "", false, 0},  // TOUCH0
        {0, true, false, "", false, 0},   // TOUCH1, reserved for BOOT button
        {2, false, false, "", false, 0},  // TOUCH2
        {15, false, false, "", false, 0}, // TOUCH3
        {13, false, false, "", false, 0}, // TOUCH4
        {12, false, false, "", false, 0}, // TOUCH5
        {14, false, false, "", false, 0}, // TOUCH6
        {27, false, false, "", false, 0}, // TOUCH7
        {33, false, false, "", false, 0}, // TOUCH8
        {32, false, false, "", false, 0}, // TOUCH9
        {-1, false, false, "", false, 0}  // End of list
                                          // End of table
};

//**************************************************************************************************
// Pages, CSS and data for the webinterface.                                                       *
//**************************************************************************************************
#include "about_html.h"
#include "config_html.h"
#include "index_html.h"
//#include "mp3play_html.h"
#include "radio_css.h"
#include "favicon_ico.h"
#include "defaultprefs.h"

int statusPlay=0;

