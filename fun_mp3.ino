//**************************************************************************************************
// End of global data section.                                                                     *
//**************************************************************************************************

//**************************************************************************************************
// VS1053 stuff.  Based on maniacbug library.                                                      *
//**************************************************************************************************
// VS1053 class definition.                                                                        *
//**************************************************************************************************
class VS1053
{
private:
  int8_t cs_pin;        // Pin where CS line is connected
  int8_t dcs_pin;       // Pin where DCS line is connected
  int8_t dreq_pin;      // Pin where DREQ line is connected
  int8_t shutdown_pin;  // Pin where the shutdown line is connected
  int8_t shutdownx_pin; // Pin where the shutdown (inversed) line is connected
  uint8_t curvol;       // Current volume setting 0..100%
  const uint8_t vs1053_chunk_size = 32;
  // SCI Register
  const uint8_t SCI_MODE = 0x0;
  const uint8_t SCI_STATUS = 0x1;
  const uint8_t SCI_BASS = 0x2;
  const uint8_t SCI_CLOCKF = 0x3;
  const uint8_t SCI_AUDATA = 0x5;
  const uint8_t SCI_WRAM = 0x6;
  const uint8_t SCI_WRAMADDR = 0x7;
  const uint8_t SCI_AIADDR = 0xA;
  const uint8_t SCI_VOL = 0xB;
  const uint8_t SCI_AICTRL0 = 0xC;
  const uint8_t SCI_AICTRL1 = 0xD;
  const uint8_t SCI_num_registers = 0xF;
  // SCI_MODE bits
  const uint8_t SM_SDINEW = 11; // Bitnumber in SCI_MODE always on
  const uint8_t SM_RESET = 2;   // Bitnumber in SCI_MODE soft reset
  const uint8_t SM_CANCEL = 3;  // Bitnumber in SCI_MODE cancel song
  const uint8_t SM_TESTS = 5;   // Bitnumber in SCI_MODE for tests
  const uint8_t SM_LINE1 = 14;  // Bitnumber in SCI_MODE for Line input
  SPISettings VS1053_SPI;       // SPI settings for this slave
  uint8_t endFillByte;          // Byte to send when stopping song
  bool okay = true;             // VS1053 is working
protected:
  inline void await_data_request() const
  {
    while ((dreq_pin >= 0) &&
           (!digitalRead(dreq_pin)))
    {
      NOP(); // Very short delay
    }
  }

  inline void control_mode_on() const
  {
    SPI.beginTransaction(VS1053_SPI); // Prevent other SPI users
    digitalWrite(cs_pin, LOW);
  }

  inline void control_mode_off() const
  {
    digitalWrite(cs_pin, HIGH); // End control mode
    SPI.endTransaction();       // Allow other SPI users
  }

  inline void data_mode_on() const
  {
    SPI.beginTransaction(VS1053_SPI); // Prevent other SPI users
    //digitalWrite ( cs_pin, HIGH ) ;           // Bring slave in data mode
    digitalWrite(dcs_pin, LOW);
  }

  inline void data_mode_off() const
  {
    digitalWrite(dcs_pin, HIGH); // End data mode
    SPI.endTransaction();        // Allow other SPI users
  }

  uint16_t read_register(uint8_t _reg) const;
  void write_register(uint8_t _reg, uint16_t _value) const;
  inline bool sdi_send_buffer(uint8_t *data, size_t len);
  void sdi_send_fillers(size_t length);
  void wram_write(uint16_t address, uint16_t data);
  uint16_t wram_read(uint16_t address);
  void output_enable(bool ena); // Enable amplifier through shutdown pin(s)

public:
  // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
  VS1053(int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin,
         int8_t _shutdown_pin, int8_t _shutdownx_pin);
  void begin(); // Begin operation.  Sets pins correctly,
  // and prepares SPI bus.
  void startSong(); // Prepare to start playing. Call this each
  // time a new song starts.
  inline bool playChunk(uint8_t *data, // Play a chunk of data.  Copies the data to
                        size_t len);   // the chip.  Blocks until complete.
  // Returns true if more data can be added
  // to fifo
  void stopSong(); // Finish playing a song. Call this after
  // the last playChunk call.
  void setVolume(uint8_t vol); // Set the player volume.Level from 0-100,
  // higher is louder.
  void setTone(uint8_t *rtone); // Set the player baas/treble, 4 nibbles for
  // treble gain/freq and bass gain/freq
  inline uint8_t getVolume() const // Get the current volume setting.
  {                                // higher is louder.
    return curvol;
  }
  void printDetails(const char *header); // Print config details to serial output
  void softReset();                      // Do a soft reset
  bool testComm(const char *header);     // Test communication with module
  inline bool data_request() const
  {
    return (digitalRead(dreq_pin) == HIGH);
  }
  void AdjustRate(long ppm2); // Fine tune the datarate
};

//**************************************************************************************************
// VS1053 class implementation.                                                                    *
//**************************************************************************************************

VS1053::VS1053(int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin,
               int8_t _shutdown_pin, int8_t _shutdownx_pin) : cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin), shutdown_pin(_shutdown_pin),
                                                              shutdownx_pin(_shutdownx_pin)
{
}

uint16_t VS1053::read_register(uint8_t _reg) const
{
  uint16_t result;

  control_mode_on();
  SPI.write(3);    // Read operation
  SPI.write(_reg); // Register to write (0..0xF)
  // Note: transfer16 does not seem to work
  result = (SPI.transfer(0xFF) << 8) | // Read 16 bits data
           (SPI.transfer(0xFF));
  await_data_request(); // Wait for DREQ to be HIGH again
  control_mode_off();
  return result;
}

void VS1053::write_register(uint8_t _reg, uint16_t _value) const
{
  control_mode_on();
  SPI.write(2);        // Write operation
  SPI.write(_reg);     // Register to write (0..0xF)
  SPI.write16(_value); // Send 16 bits data
  await_data_request();
  control_mode_off();
}

bool VS1053::sdi_send_buffer(uint8_t *data, size_t len)
{
  size_t chunk_length; // Length of chunk 32 byte or shorter
  //Serial.print("data=");

  data_mode_on();
  while (len) // More to do?
  {
    chunk_length = len;
    if (len > vs1053_chunk_size)
    {
      chunk_length = vs1053_chunk_size;
    }
    len -= chunk_length;
    await_data_request(); // Wait for space available
    SPI.writeBytes(data, chunk_length);
    data += chunk_length;
  }
  data_mode_off();
  return data_request(); // True if more data can de stored in fifo
}

void VS1053::sdi_send_fillers(size_t len)
{
  size_t chunk_length; // Length of chunk 32 byte or shorter

  data_mode_on();
  while (len) // More to do?
  {
    await_data_request(); // Wait for space available
    chunk_length = len;
    if (len > vs1053_chunk_size)
    {
      chunk_length = vs1053_chunk_size;
    }
    len -= chunk_length;
    while (chunk_length--)
    {
      SPI.write(endFillByte);
    }
  }
  data_mode_off();
}

void VS1053::wram_write(uint16_t address, uint16_t data)
{
  write_register(SCI_WRAMADDR, address);
  write_register(SCI_WRAM, data);
}

uint16_t VS1053::wram_read(uint16_t address)
{
  write_register(SCI_WRAMADDR, address); // Start reading from WRAM
  return read_register(SCI_WRAM);        // Read back result
}

bool VS1053::testComm(const char *header)
{
  // Test the communication with the VS1053 module.  The result wille be returned.
  // If DREQ is low, there is problably no VS1053 connected.  Pull the line HIGH
  // in order to prevent an endless loop waiting for this signal.  The rest of the
  // software will still work, but readbacks from VS1053 will fail.
  int i; // Loop control
  uint16_t r1, r2, cnt = 0;
  uint16_t delta = 300;                              // 3 for fast SPI
  const uint16_t vstype[] = {1001, 1011, 1002, 1003, // Possible chip versions
                             1053, 1033, 0000, 1103};

  dbgprint(header); // Show a header
  if (!digitalRead(dreq_pin))
  {
    dbgprint("VS1053 not properly installed!");
    // Allow testing without the VS1053 module
    pinMode(dreq_pin, INPUT_PULLUP); // DREQ is now input with pull-up
    return false;                    // Return bad result
  }
  // Further TESTING.  Check if SCI bus can write and read without errors.
  // We will use the volume setting for this.
  // Will give warnings on serial output if DEBUG is active.
  // A maximum of 20 errors will be reported.
  if (strstr(header, "Fast"))
  {
    delta = 3; // Fast SPI, more loops
  }
  for (i = 0; (i < 0xFFFF) && (cnt < 20); i += delta)
  {
    write_register(SCI_VOL, i);         // Write data to SCI_VOL
    r1 = read_register(SCI_VOL);        // Read back for the first time
    r2 = read_register(SCI_VOL);        // Read back a second time
    if (r1 != r2 || i != r1 || i != r2) // Check for 2 equal reads
    {
      dbgprint("VS1053 SPI error. SB:%04X R1:%04X R2:%04X", i, r1, r2);
      cnt++;
      delay(10);
    }
  }
  okay = (cnt == 0); // True if working correctly
  // Further testing: is it the right chip?
  r1 = (read_register(SCI_STATUS) >> 4) & 0x7; // Read status to get the version
  if (r1 != 4)                                 // Version 4 is a genuine VS1053
  {
    dbgprint("This is not a VS1053, " // Report the wrong chip
             "but a VS%d instead!",
             vstype[r1]);
    okay = false;
  }
  return (okay); // Return the result
}

void VS1053::begin()
{
  pinMode(dreq_pin, INPUT); // DREQ is an input
  pinMode(cs_pin, OUTPUT);  // The SCI and SDI signals
  pinMode(dcs_pin, OUTPUT);
  digitalWrite(dcs_pin, HIGH); // Start HIGH for SCI en SDI
  digitalWrite(cs_pin, HIGH);
  if (shutdown_pin >= 0) // Shutdown in use?
  {
    pinMode(shutdown_pin, OUTPUT);
  }
  if (shutdownx_pin >= 0) // Shutdown (inversed logic) in use?
  {
    pinMode(shutdownx_pin, OUTPUT);
  }
  output_enable(false); // Disable amplifier through shutdown pin(s)
  delay(100);
  // Init SPI in slow mode ( 0.2 MHz )
  VS1053_SPI = SPISettings(200000, MSBFIRST, SPI_MODE0);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  //printDetails ( "Right after reset/startup" ) ;
  delay(20);
  //printDetails ( "20 msec after reset" ) ;
  if (testComm("Slow SPI, Testing VS1053 read/write registers..."))
  {
    // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
    // when playing MP3.  You can modify the board, but there is a more elegant way:
    wram_write(0xC017, 3); // GPIO DDR = 3
    wram_write(0xC019, 0); // GPIO ODATA = 0
    delay(100);
    //printDetails ( "After test loop" ) ;
    softReset(); // Do a soft reset
    // Switch on the analog parts
    write_register(SCI_AUDATA, 44100 + 1); // 44.1kHz + stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register(SCI_CLOCKF, 6 << 12); // Normal clock settings
    // multiplyer 3.0 = 12.2 MHz
    //SPI Clock to 4 MHz. Now you can set high speed SPI clock.
    VS1053_SPI = SPISettings(5000000, MSBFIRST, SPI_MODE0);
    write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_LINE1));
    testComm("Fast SPI, Testing VS1053 read/write registers again...");
    delay(10);
    await_data_request();
    endFillByte = wram_read(0x1E06) & 0xFF;
    dbgprint("endFillByte is %X", endFillByte);
    //printDetails ( "After last clocksetting" ) ;
    delay(100);
  }
}

void VS1053::setVolume(uint8_t vol)
{
  // Set volume.  Both left and right.
  // Input value is 0..100.  100 is the loudest.
  // Clicking reduced by using 0xf8 to 0x00 as limits.
  uint16_t value; // Value to send to SCI_VOL

  if (vol != curvol)
  {
    curvol = vol;                         // Save for later use
    value = map(vol, 0, 100, 0xF8, 0x00); // 0..100% to one channel
    value = (value << 8) | value;
    write_register(SCI_VOL, value); // Volume left and right
    if (vol == 0)                   // Completely silence?
    {
      output_enable(false); // Yes, mute amplifier
    }
    else
    {
      if (datamode != STOPPED)
      {
        output_enable(true); // Enable amplifier if not stopped
      }
    }
    output_enable(vol != 0); // Enable/disable amplifier through shutdown pin(s)
  }
}

void VS1053::setTone(uint8_t *rtone) // Set bass/treble (4 nibbles)
{
  // Set tone characteristics.  See documentation for the 4 nibbles.
  uint16_t value = 0; // Value to send to SCI_BASS
  int i;              // Loop control

  for (i = 0; i < 4; i++)
  {
    value = (value << 4) | rtone[i]; // Shift next nibble in
  }
  write_register(SCI_BASS, value); // Volume left and right
}

void VS1053::startSong()
{
  sdi_send_fillers(10);
  output_enable(true); // Enable amplifier through shutdown pin(s)
}

bool VS1053::playChunk(uint8_t *data, size_t len)
{
  return okay && sdi_send_buffer(data, len); // True if more data can be added to fifo
}

void VS1053::stopSong()
{
  uint16_t modereg; // Read from mode register
  int i;            // Loop control

  sdi_send_fillers(2052);
  output_enable(false); // Disable amplifier through shutdown pin(s)
  delay(10);
  write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_CANCEL));
  for (i = 0; i < 20; i++)
  {
    sdi_send_fillers(32);
    modereg = read_register(SCI_MODE);   // Read mode status
    if ((modereg & _BV(SM_CANCEL)) == 0) // SM_CANCEL will be cleared when finished
    {
      sdi_send_fillers(2052);
      dbgprint("Song stopped correctly after %d msec", i * 10);
      return;
    }
    delay(10);
  }
  printDetails("Song stopped incorrectly!");
}

void VS1053::softReset()
{
  write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
  delay(10);
  await_data_request();
}
// // void VS1053::switchToMp3Mode() {
// //     wram_write(0xC017, 3); // GPIO DDR = 3
// //     wram_write(0xC019, 0); // GPIO ODATA = 0
// //     delay(100);
// //     LOG("Switched to mp3 mode\n");
// //     softReset();
// // }

void VS1053::printDetails(const char *header)
{
  uint16_t regbuf[16];
  uint8_t i;

  dbgprint(header);
  dbgprint("REG   Contents");
  dbgprint("---   -----");
  for (i = 0; i <= SCI_num_registers; i++)
  {
    regbuf[i] = read_register(i);
  }
  for (i = 0; i <= SCI_num_registers; i++)
  {
    delay(5);
    dbgprint("%3X - %5X", i, regbuf[i]);
  }
}

void VS1053::output_enable(bool ena) // Enable amplifier through shutdown pin(s)
{
  if (shutdown_pin >= 0) // Shutdown in use?
  {
    digitalWrite(shutdown_pin, !ena); // Shut down or enable audio output
  }
  if (shutdownx_pin >= 0) // Shutdown (inversed logic) in use?
  {
    digitalWrite(shutdownx_pin, ena); // Shut down or enable audio output
  }
}

void VS1053::AdjustRate(long ppm2) // Fine tune the data rate
{
  write_register(SCI_WRAMADDR, 0x1e07);
  write_register(SCI_WRAM, ppm2);
  write_register(SCI_WRAM, ppm2 >> 16);
  // oldClock4KHz = 0 forces  adjustment calculation when rate checked.
  write_register(SCI_WRAMADDR, 0x5b1c);
  write_register(SCI_WRAM, 0);
  // Write to AUDATA or CLOCKF checks rate and recalculates adjustment.
  write_register(SCI_AUDATA, read_register(SCI_AUDATA));
}

// The object for the MP3 player
VS1053 *vs1053player;

//**************************************************************************************************
// End VS1053 stuff.                                                                               *
//**************************************************************************************************

//**************************************************************************************************
//                                      N V S O P E N                                              *
//**************************************************************************************************
// Open Preferences with my-app namespace. Each application module, library, etc.                  *
// has to use namespace name to prevent key name collisions. We will open storage in               *
// RW-mode (second parameter has to be false).                                                     *
//**************************************************************************************************
void nvsopen()
{
  if (!nvshandle) // Opened already?
  {
    nvserr = nvs_open(NAME, NVS_READWRITE, &nvshandle); // No, open nvs
    if (nvserr)
    {
      dbgprint("nvs_open failed!");
    }
  }
}

//**************************************************************************************************
//                                      N V S C L E A R                                            *
//**************************************************************************************************
// Clear all preferences.                                                                          *
//**************************************************************************************************
esp_err_t nvsclear()
{
  nvsopen();                       // Be sure to open nvs
  return nvs_erase_all(nvshandle); // Clear all keys
}

//**************************************************************************************************
//                                      N V S G E T S T R                                          *
//**************************************************************************************************
// Read a string from nvs.                                                                         *
//**************************************************************************************************
String nvsgetstr(const char *key)
{
  static char nvs_buf[NVSBUFSIZE]; // Buffer for contents
  size_t len = NVSBUFSIZE;         // Max length of the string, later real length

  nvsopen();         // Be sure to open nvs
  nvs_buf[0] = '\0'; // Return empty string on error
  nvserr = nvs_get_str(nvshandle, key, nvs_buf, &len);
  if (nvserr)
  {
    dbgprint("nvs_get_str failed %X for key %s, keylen is %d, len is %d!",
             nvserr, key, strlen(key), len);
    dbgprint("Contents: %s", nvs_buf);
  }
  return String(nvs_buf);
}

//**************************************************************************************************
//                                      N V S S E T S T R                                          *
//**************************************************************************************************
// Put a key/value pair in nvs.  Length is limited to allow easy read-back.                        *
// No writing if no change.                                                                        *
//**************************************************************************************************
esp_err_t nvssetstr(const char *key, String val)
{
  String curcont;    // Current contents
  bool wflag = true; // Assume update or new key

  //dbgprint ( "Setstring for %s: %s", key, val.c_str() ) ;
  if (val.length() >= NVSBUFSIZE) // Limit length of string to store
  {
    dbgprint("nvssetstr length failed!");
    return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
  }
  if (nvssearch(key)) // Already in nvs?
  {
    curcont = nvsgetstr(key); // Read current value
    wflag = (curcont != val); // Value change?
  }
  if (wflag) // Update or new?
  {
    //dbgprint ( "nvssetstr update value" ) ;
    nvserr = nvs_set_str(nvshandle, key, val.c_str()); // Store key and value
    if (nvserr)                                        // Check error
    {
      dbgprint("nvssetstr failed!");
    }
  }
  return nvserr;
}

//**************************************************************************************************
//                                      N V S C H K E Y                                            *
//**************************************************************************************************
// Change a keyname in in nvs.                                                                     *
//**************************************************************************************************
void nvschkey(const char *oldk, const char *newk)
{
  String curcont; // Current contents

  if (nvssearch(oldk)) // Old key in nvs?
  {
    curcont = nvsgetstr(oldk);      // Read current value
    nvs_erase_key(nvshandle, oldk); // Remove key
    nvssetstr(newk, curcont);       // Insert new
  }
}

//**************************************************************************************************
//                                      C L A I M S P I                                            *
//**************************************************************************************************
// Claim the SPI bus.  Uses FreeRTOS semaphores.                                                   *
// If the semaphore cannot be claimed within the time-out period, the function continues without   *
// claiming the semaphore.  This is incorrect but allows debugging.                                *
//**************************************************************************************************
void claimSPI(const char *p)
{
  const TickType_t ctry = 10;         // Time to wait for semaphore
  uint32_t count = 0;                 // Wait time in ticks
  static const char *old_id = "none"; // ID that holds the bus

  while (xSemaphoreTake(SPIsem, ctry) != pdTRUE) // Claim SPI bus
  {
    if (count++ > 25)
    {
      dbgprint("SPI semaphore not taken within %d ticks by CPU %d, id %s",
               count * ctry,
               xPortGetCoreID(),
               p);
      dbgprint("Semaphore is claimed by %s", old_id);
    }
    if (count >= 100)
    {
      return; // Continue without semaphore
    }
  }
  old_id = p; // Remember ID holding the semaphore
}

//**************************************************************************************************
//                                   R E L E A S E S P I                                           *
//**************************************************************************************************
// Free the the SPI bus.  Uses FreeRTOS semaphores.                                                *
//**************************************************************************************************
void releaseSPI()
{
  xSemaphoreGive(SPIsem); // Release SPI bus
}

//**************************************************************************************************
//                                      Q U E U E F U N C                                          *
//**************************************************************************************************
// Queue a special function for the play task.                                                     *
//**************************************************************************************************
void queuefunc(int func)
{
  qdata_struct specchunk; // Special function to queue

  specchunk.datatyp = func;                      // Put function in datatyp
  xQueueSendToFront(dataqueue, &specchunk, 200); // Send to queue (First Out)
}

//**************************************************************************************************
//                                      N V S S E A R C H                                          *
//**************************************************************************************************
// Check if key exists in nvs.                                                                     *
//**************************************************************************************************
bool nvssearch(const char *key)
{
  size_t len = NVSBUFSIZE; // Length of the string

  nvsopen();                                        // Be sure to open nvs
  nvserr = nvs_get_str(nvshandle, key, NULL, &len); // Get length of contents
  return (nvserr == ESP_OK);                        // Return true if found
}

//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// UTF8-Decoder: convert UTF8-string to extended ASCII.                                            *
// Convert a single Character from UTF8 to Extended ASCII.                                         *
// Return "0" if a byte has to be ignored.                                                         *
//**************************************************************************************************
char utf8ascii(char ascii)
{
  static const char lut_C3[] = {"AAAAAAACEEEEIIIIDNOOOOO#0UUUU###"
                                "aaaaaaaceeeeiiiidnooooo##uuuuyyy"};
  static char c1;  // Last character buffer
  char res = '\0'; // Result, default 0

  if (ascii <= 0x7F) // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0;
    res = ascii; // Return unmodified
  }
  else
  {
    switch (c1) // Conversion depending on first UTF8-character
    {
    case 0xC2:
      res = '~';
      break;
    case 0xC3:
      res = lut_C3[ascii - 128];
      break;
    case 0x82:
      if (ascii == 0xAC)
      {
        res = 'E'; // Special case Euro-symbol
      }
    }
    c1 = ascii; // Remember actual character
  }
  return res; // Otherwise: return zero, if character has to be ignored
}

//**************************************************************************************************
//                                U T F 8 A S C I I _ I P                                          *
//**************************************************************************************************
// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!).                          *
//**************************************************************************************************
void utf8ascii_ip(char *s)
{
  int i, k = 0; // Indexes for in en out string
  char c;

  for (i = 0; s[i]; i++) // For every input character
  {
    c = utf8ascii(s[i]); // Translate if necessary
    if (c)               // Good translation?
    {
      s[k++] = c; // Yes, put in output string
    }
  }
  s[k] = 0; // Take care of delimeter
}

//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// Conversion UTF8-String to Extended ASCII String.                                                *
//**************************************************************************************************
String utf8ascii(const char *s)
{
  int i; // Index for input string
  char c;
  String res = ""; // Result string

  for (i = 0; s[i]; i++) // For every input character
  {
    c = utf8ascii(s[i]); // Translate if necessary
    if (c)               // Good translation?
    {
      res += String(c); // Yes, put in output string
    }
  }
  return res;
}

//**************************************************************************************************
//                                          D B G P R I N T                                        *
//**************************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.        *
// Print only if DEBUG flag is true.  Always returns the formatted string.                         *
//**************************************************************************************************
char *dbgprint(const char *format, ...)
{
  static char sbuf[DEBUG_BUFFER_SIZE]; // For debug lines
  va_list varArgs;                     // For variable number of params

  va_start(varArgs, format);                      // Prepare parameters
  vsnprintf(sbuf, sizeof(sbuf), format, varArgs); // Format the message
  va_end(varArgs);                                // End of using parameters
  if (DEBUG)                                      // DEBUG on?
  {
    Serial.print("D: ");  // Yes, print prefix
    Serial.println(sbuf); // and the info
  }
  return sbuf; // Return stored string
}

//**************************************************************************************************
//                                     G E T E N C R Y P T I O N T Y P E                           *
//**************************************************************************************************
// Read the encryption type of the network and return as a 4 byte name                             *
//**************************************************************************************************
const char *getEncryptionType(wifi_auth_mode_t thisType)
{
  switch (thisType)
  {
  case WIFI_AUTH_OPEN:
    return "OPEN";
  case WIFI_AUTH_WEP:
    return "WEP";
  case WIFI_AUTH_WPA_PSK:
    return "WPA_PSK";
  case WIFI_AUTH_WPA2_PSK:
    return "WPA2_PSK";
  case WIFI_AUTH_WPA_WPA2_PSK:
    return "WPA_WPA2_PSK";
  case WIFI_AUTH_MAX:
    return "MAX";
  default:
    break;
  }
  return "????";
}

//**************************************************************************************************
//                                        L I S T N E T W O R K S                                  *
//**************************************************************************************************
// List the available networks.                                                                    *
// Acceptable networks are those who have an entry in the preferences.                             *
// SSIDs of available networks will be saved for use in webinterface.                              *
//**************************************************************************************************
void listNetworks()
{
  WifiInfo_t winfo;            // Entry from wifilist
  wifi_auth_mode_t encryption; // TKIP(WPA), WEP, etc.
  const char *acceptable;      // Netwerk is acceptable for connection
  int i, j;                    // Loop control

  dbgprint("Scan Networks"); // Scan for nearby networks
  numSsid = WiFi.scanNetworks();
  dbgprint("Scan completed");
  if (numSsid <= 0)
  {
    dbgprint("Couldn't get a wifi connection");
    return;
  }
  // print the list of networks seen:
  dbgprint("Number of available networks: %d",
           numSsid);
  // Print the network number and name for each network found and
  for (i = 0; i < numSsid; i++)
  {
    acceptable = "";                      // Assume not acceptable
    for (j = 0; j < wifilist.size(); j++) // Search in wifilist
    {
      winfo = wifilist[j];                       // Get one entry
      if (WiFi.SSID(i).indexOf(winfo.ssid) == 0) // Is this SSID acceptable?
      {
        acceptable = "Acceptable";
        break;
      }
    }
    encryption = WiFi.encryptionType(i);
    dbgprint("%2d - %-25s Signal: %3d dBm, Encryption %4s, %s",
             i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
             getEncryptionType(encryption),
             acceptable);
    // Remember this network for later use
    networks += WiFi.SSID(i) + String("|");
  }
  dbgprint("End of list");
}

//**************************************************************************************************
//                                          T I M E R 1 0 S E C                                    *
//**************************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                                       *
// If totalcount has not been changed, there is a problem and playing will stop.                   *
// Note that calling timely procedures within this routine or in called functions will             *
// cause a crash!                                                                                  *
//**************************************************************************************************
void IRAM_ATTR timer10sec()
{
  static uint32_t oldtotalcount = 7321; // Needed for change detection
  static uint8_t morethanonce = 0;      // Counter for succesive fails
  uint32_t bytesplayed;                 // Bytes send to MP3 converter

  if (datamode & (INIT | HEADER | DATA | // Test op playing
                  METADATA | PLAYLISTINIT |
                  PLAYLISTHEADER |
                  PLAYLISTDATA))
  {
    bytesplayed = totalcount - oldtotalcount; // Nunber of bytes played in the 10 seconds
    oldtotalcount = totalcount;               // Save for comparison in next cycle
    if (bytesplayed == 0)                     // Still playing?
    {
      if (morethanonce > 10) // No! Happened too many times?
      {
        ESP.restart(); // Reset the CPU, probably no return
      }
      if (datamode & (PLAYLISTDATA | // In playlist mode?
                      PLAYLISTINIT |
                      PLAYLISTHEADER))
      {
        playlist_num = 0; // Yes, end of playlist
      }
      if ((morethanonce > 0) || // Happened more than once?
          (playlist_num > 0))   // Or playlist active?
      {
        datamode = STOPREQD;   // Stop player
        ini_block.newpreset++; // Yes, try next channel
      }
      morethanonce++; // Count the fails
    }
    else
    {
      //                                          // Data has been send to MP3 decoder
      // Bitrate in kbits/s is bytesplayed / 10 / 1000 * 8
      mbitrate = (bytesplayed + 625) / 1250; // Measured bitrate
      morethanonce = 0;                      // Data seen, reset failcounter
    }
  }
}

//**************************************************************************************************
//                                          T I M E R 1 0 0                                        *
//**************************************************************************************************
// Called every 100 msec on interrupt level, so must be in IRAM and no lengthy operations          *
// allowed.                                                                                        *
//**************************************************************************************************
void IRAM_ATTR timer100()
{
  sv int16_t count10sec = 0;    // Counter for activatie 10 seconds process
  sv int16_t eqcount = 0;       // Counter for equal number of clicks
  sv int16_t oldclickcount = 0; // To detect difference

  if (++count10sec == 100) // 10 seconds passed?
  {
    timer10sec();   // Yes, do 10 second procedure
    count10sec = 0; // Reset count
  }
  if ((count10sec % 10) == 0) // One second over?
  {
    scaniocount = scanios; // TEST*TEST*TEST
    scanios = 0;
    if (++timeinfo.tm_sec >= 60) // Yes, update number of seconds
    {
      timeinfo.tm_sec = 0; // Wrap after 60 seconds
      if (++timeinfo.tm_min >= 60)
      {
        timeinfo.tm_min = 0; // Wrap after 60 minutes
        if (++timeinfo.tm_hour >= 24)
        {
          timeinfo.tm_hour = 0; // Wrap after 24 hours
        }
      }
    }
    time_req = true;          // Yes, show current time request
    if (++bltimer == BL_TIME) // Time to blank the TFT screen?
    {
      bltimer = 0; // Yes, reset counter
    }
  }
}

//**************************************************************************************************
//                                S H O W S T R E A M T I T L E                                    *
//**************************************************************************************************
// Show artist and songtitle if present in metadata.                                               *
// Show always if full=true.                                                                       *
//**************************************************************************************************
void showstreamtitle(const char *ml, bool full)
{
  char *p1;
  char *p2;
  char streamtitle[150]; // Streamtitle from metadata

  if (strstr(ml, "StreamTitle="))
  {
    dbgprint("Streamtitle found, %d bytes", strlen(ml));
    dbgprint(ml);
    p1 = (char *)ml + 12;       // Begin of artist and title
    if ((p2 = strstr(ml, ";"))) // Search for end of title
    {
      if (*p1 == '\'') // Surrounded by quotes?
      {
        p1++;
        p2--;
      }
      *p2 = '\0'; // Strip the rest of the line
    }
    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy(streamtitle, p1, sizeof(streamtitle));
    streamtitle[sizeof(streamtitle) - 1] = '\0';
  }
  else if (full)
  {
    // Info probably from playlist
    strncpy(streamtitle, ml, sizeof(streamtitle));
    streamtitle[sizeof(streamtitle) - 1] = '\0';
  }
  else
  {
    icystreamtitle = ""; // Unknown type
    return;              // Do not show
  }
  // Save for status request from browser and for MQTT
  icystreamtitle = streamtitle;
  if ((p1 = strstr(streamtitle, " - "))) // look for artist/title separator
  {
    p2 = p1 + 3; // 2nd part of text at this position
    if (displaytype == T_NEXTION)
    {
      *p1++ = '\\'; // Found: replace 3 characters by "\r"
      *p1++ = 'r';  // Found: replace 3 characters by "\r"
    }
    else
    {
      *p1++ = '\n'; // Found: replace 3 characters by newline
    }
    if (*p2 == ' ') // Leading space in title?
    {
      p2++;
    }
    strcpy(p1, p2); // Shift 2nd part of title 2 or 3 places
  }
}

//**************************************************************************************************
//                                    S E T D A T A M O D E                                        *
//**************************************************************************************************
// Change the datamode and show in debug for testing.                                              *
//**************************************************************************************************
void setdatamode(datamode_t newmode)
{
  //dbgprint ( "Change datamode from 0x%03X to 0x%03X",
  //           (int)datamode, (int)newmode ) ;
  datamode = newmode;
}

//**************************************************************************************************
//                                    S T O P _ M P 3 C L I E N T                                  *
//**************************************************************************************************
// Disconnect from the server.                                                                     *
//**************************************************************************************************
void stop_mp3client()
{

  while (mp3client.connected())
  {
    dbgprint("Stopping client"); // Stop connection to host
    mp3client.flush();
    mp3client.stop();
    delay(500);
  }
  mp3client.flush(); // Flush stream client
  mp3client.stop();  // Stop stream client
  statusPlay = 2;
}

//**************************************************************************************************
//                                    C O N N E C T T O H O S T                                    *
//**************************************************************************************************
// Connect to the Internet radio server specified by newpreset.                                    *
//**************************************************************************************************
bool connecttohost(String host, String path, uint16_t port)
{
  stop_mp3client(); // Disconnect if still connected
  dbgprint("Connect to new host %s", host.c_str());

  setdatamode(INIT); // Start default in metamode
  chunked = false;   // Assume not chunked

  dbgprint("Connect to %s on port %d, extension %s",
           host.c_str(), port, path.c_str());
  if (mp3client.connect(host.c_str(), port))
  {
    dbgprint("Connected to server");

    mp3client.print(String("GET ") +
                    path +
                    String(" HTTP/1.0\r\n") +
                    String("Host: ") +
                    host +
                    String("\r\n") +
                    String("Connection: close\r\n\r\n"));
    
    return true;
  }
  dbgprint("Request %s failed!", host.c_str());
  statusPlay = 2;
  return false;
}

//**************************************************************************************************
//                                      S S C O N V                                                *
//**************************************************************************************************
// Convert an array with 4 "synchsafe integers" to a number.                                       *
// There are 7 bits used per byte.                                                                 *
//**************************************************************************************************
uint32_t ssconv(const uint8_t *bytes)
{
  uint32_t res = 0; // Result of conversion
  uint8_t i;        // Counter number of bytes to convert

  for (i = 0; i < 4; i++) // Handle 4 bytes
  {
    res = res * 128 + bytes[i]; // Convert next 7 bits
  }
  return res; // Return the result
}

//**************************************************************************************************
//                                       R E S E R V E P I N                                       *
//**************************************************************************************************
// Set I/O pin to "reserved".                                                                      *
// The pin is than not available for a programmable function.                                      *
//**************************************************************************************************
void reservepin(int8_t rpinnr)
{
  uint8_t i = 0; // Index in progpin/touchpin array
  int8_t pin;    // Pin number in progpin array

  while ((pin = progpin[i].gpio) >= 0) // Find entry for requested pin
  {
    if (pin == rpinnr) // Entry found?
    {
      if (progpin[i].reserved) // Already reserved?
      {
        dbgprint("Pin %d is already reserved!", rpinnr);
      }
      //dbgprint ( "GPIO%02d unavailabe for 'gpio_'-command", pin ) ;
      progpin[i].reserved = true; // Yes, pin is reserved now
      break;                      // No need to continue
    }
    i++; // Next entry
  }
  // Also reserve touchpin numbers
  i = 0;
  while ((pin = touchpin[i].gpio) >= 0) // Find entry for requested pin
  {
    if (pin == rpinnr) // Entry found?
    {
      //dbgprint ( "GPIO%02d unavailabe for 'touch'-command", pin ) ;
      touchpin[i].reserved = true; // Yes, pin is reserved now
      break;                       // No need to continue
    }
    i++; // Next entry
  }
}

//**************************************************************************************************
//                                       R E A D I O P R E F S                                     *
//**************************************************************************************************
// Scan the preferences for IO-pin definitions.                                                    *
//**************************************************************************************************
void readIOprefs()
{
  struct iosetting
  {
    const char *gname; // Name in preferences
    int8_t *gnr;       // GPIO pin number
    int8_t pdefault;   // Default pin
  };
  struct iosetting klist[] = {
      // List of I/O related keys
      {"pin_ir", &ini_block.ir_pin, -1},
      {"pin_enc_clk", &ini_block.enc_clk_pin, -1},
      {"pin_enc_dt", &ini_block.enc_dt_pin, -1},
      {"pin_enc_sw", &ini_block.enc_sw_pin, -1},
      {"pin_tft_cs", &ini_block.tft_cs_pin, -1},   // Display SPI version
      {"pin_tft_dc", &ini_block.tft_dc_pin, -1},   // Display SPI version
      {"pin_tft_scl", &ini_block.tft_scl_pin, -1}, // Display I2C version
      {"pin_tft_sda", &ini_block.tft_sda_pin, -1}, // Display I2C version
      {"pin_tft_bl", &ini_block.tft_bl_pin, -1},   // Display backlight
      {"pin_tft_blx", &ini_block.tft_blx_pin, -1}, // Display backlight (inversed logic)
      {"pin_sd_cs", &ini_block.sd_cs_pin, -1},
      {"pin_ch376_cs", &ini_block.ch376_cs_pin, -1},      // CH376 CS for USB interface
      {"pin_ch376_int", &ini_block.ch376_int_pin, -1},    // CH376 INT for USB interfce
      {"pin_vs_cs", &ini_block.vs_cs_pin, 15},             // VS1053 pins 5
      {"pin_vs_dcs", &ini_block.vs_dcs_pin, 25},          //25
      {"pin_vs_dreq", &ini_block.vs_dreq_pin, 33},         //4
      {"pin_shutdown", &ini_block.vs_shutdown_pin, -1},   // Amplifier shut-down pin
      {"pin_shutdownx", &ini_block.vs_shutdownx_pin, -1}, // Amplifier shut-down pin (inversed logic)
      {"pin_spi_sck", &ini_block.spi_sck_pin, 18},
      {"pin_spi_miso", &ini_block.spi_miso_pin, 19},
      {"pin_spi_mosi", &ini_block.spi_mosi_pin, 23},
      {NULL, NULL, 0} // End of list
  };
  int i;         // Loop control
  int count = 0; // Number of keys found
  String val;    // Contents of preference entry
  int8_t ival;   // Value converted to integer
  int8_t *p;     // Points to variable

  for (i = 0; klist[i].gname; i++) // Loop trough all I/O related keys
  {
    p = klist[i].gnr;              // Point to target variable
    ival = klist[i].pdefault;      // Assume pin number to be the default
    if (nvssearch(klist[i].gname)) // Does it exist?
    {
      val = nvsgetstr(klist[i].gname); // Read value of key
      if (val.length())                // Parameter in preference?
      {
        count++;            // Yes, count number of filled keys
        ival = val.toInt(); // Convert value to integer pinnumber
        reservepin(ival);   // Set pin to "reserved"
      }
    }
    *p = ival;               // Set pinnumber in ini_block
    dbgprint("%s set to %d", // Show result
             klist[i].gname,
             ival);
  }
}

//**************************************************************************************************
//                                       R E A D P R E F S                                         *
//**************************************************************************************************
// Read the preferences and interpret the commands.                                                *
// If output == true, the key / value pairs are returned to the caller as a String.                *
//**************************************************************************************************
String readprefs(bool output)
{
  uint16_t i;             // Loop control
  String val;             // Contents of preference entry
  String cmd;             // Command for analyzCmd
  String outstr = "";     // Outputstring
  char *key;              // Point to nvskeys[i]
  uint8_t winx;           // Index in wifilist
  uint16_t last2char = 0; // To detect paragraphs

  i = 0;
  while (*(key = nvskeys[i])) // Loop trough all available keys
  {
    val = nvsgetstr(key); // Read value of this key
    cmd = String(key) +   // Yes, form command
          String(" = ") +
          val;
    if (strstr(key, "wifi_")) // Is it a wifi ssid/password?
    {
      winx = atoi(key + 5);           // Get index in wifilist
      if ((winx < wifilist.size()) && // Existing wifi spec in wifilist?
          (val.indexOf(wifilist[winx].ssid) == 0))
      {
        val = String(wifilist[winx].ssid) + // Yes, hide password
              String("/*******");
      }
      cmd = String(""); // Do not analyze this
    }
    else if (strstr(key, "mqttpasswd")) // Is it a MQTT password?
    {
      val = String("*******"); // Yes, hide it
    }
    if (output)
    {
      if ((i > 0) &&
          (*(uint16_t *)key != last2char)) // New paragraph?
      {
        outstr += String("#\n"); // Yes, add separator
      }
      last2char = *(uint16_t *)key; // Save 2 chars for next compare
      outstr += String(key) +       // Add to outstr
                String(" = ") +
                val +
                String("\n"); // Add newline
    }
    else
    {
      //analyzeCmd(cmd.c_str()); // Analyze it
    }
    i++; // Next key
  }
  if (i == 0)
  {
    outstr = String("No preferences found.\n"
                    "Use defaults or run Esp32_radio_init first.\n");
  }
  return outstr;
}

//**************************************************************************************************
//                                   F I N D N S I D                                               *
//**************************************************************************************************
// Find the namespace ID for the namespace passed as parameter.                                    *
//**************************************************************************************************
uint8_t FindNsID(const char *ns)
{
  esp_err_t result = ESP_OK; // Result of reading partition
  uint32_t offset = 0;       // Offset in nvs partition
  uint8_t i;                 // Index in Entry 0..125
  uint8_t bm;                // Bitmap for an entry
  uint8_t res = 0xFF;        // Function result

  while (offset < nvs->size)
  {
    result = esp_partition_read(nvs, offset, // Read 1 page in nvs partition
                                &nvsbuf,
                                sizeof(nvsbuf));
    if (result != ESP_OK)
    {
      dbgprint("Error reading NVS!");
      break;
    }
    i = 0;
    while (i < 126)
    {

      bm = (nvsbuf.Bitmap[i / 4] >> ((i % 4) * 2)); // Get bitmap for this entry,
      bm &= 0x03;                                   // 2 bits for one entry
      if ((bm == 2) &&
          (nvsbuf.Entry[i].Ns == 0) &&
          (strcmp(ns, nvsbuf.Entry[i].Key) == 0))
      {
        res = nvsbuf.Entry[i].Data & 0xFF; // Return the ID
        offset = nvs->size;                // Stop outer loop as well
        break;
      }
      else
      {
        if (bm == 2)
        {
          i += nvsbuf.Entry[i].Span; // Next entry
        }
        else
        {
          i++;
        }
      }
    }
    offset += sizeof(nvs_page); // Prepare to read next page in nvs
  }
  return res;
}

//**************************************************************************************************
//                            B U B B L E S O R T K E Y S                                          *
//**************************************************************************************************
// Bubblesort the nvskeys.                                                                         *
//**************************************************************************************************
void bubbleSortKeys(uint16_t n)
{
  uint16_t i, j;   // Indexes in nvskeys
  char tmpstr[16]; // Temp. storage for a key

  for (i = 0; i < n - 1; i++) // Examine all keys
  {
    for (j = 0; j < n - i - 1; j++) // Compare to following keys
    {
      if (strcmp(nvskeys[j], nvskeys[j + 1]) > 0) // Next key out of order?
      {
        strcpy(tmpstr, nvskeys[j]);         // Save current key a while
        strcpy(nvskeys[j], nvskeys[j + 1]); // Replace current with next key
        strcpy(nvskeys[j + 1], tmpstr);     // Replace next with saved current
      }
    }
  }
}

//**************************************************************************************************
//                                      F I L L K E Y L I S T                                      *
//**************************************************************************************************
// File the list of all relevant keys in NVS.                                                      *
// The keys will be sorted.                                                                        *
//**************************************************************************************************
void fillkeylist()
{
  esp_err_t result = ESP_OK; // Result of reading partition
  uint32_t offset = 0;       // Offset in nvs partition
  uint16_t i;                // Index in Entry 0..125.
  uint8_t bm;                // Bitmap for an entry
  uint16_t nvsinx = 0;       // Index in nvskey table

  keynames.clear(); // Clear the list
  while (offset < nvs->size)
  {
    result = esp_partition_read(nvs, offset, // Read 1 page in nvs partition
                                &nvsbuf,
                                sizeof(nvsbuf));
    if (result != ESP_OK)
    {
      dbgprint("Error reading NVS!");
      break;
    }
    i = 0;
    while (i < 126)
    {
      bm = (nvsbuf.Bitmap[i / 4] >> ((i % 4) * 2)); // Get bitmap for this entry,
      bm &= 0x03;                                   // 2 bits for one entry
      if (bm == 2)                                  // Entry is active?
      {
        if (nvsbuf.Entry[i].Ns == namespace_ID) // Namespace right?
        {
          strcpy(nvskeys[nvsinx], nvsbuf.Entry[i].Key); // Yes, save in table
          if (++nvsinx == MAXKEYS)
          {
            nvsinx--; // Prevent excessive index
          }
        }
        i += nvsbuf.Entry[i].Span; // Next entry
      }
      else
      {
        i++;
      }
    }
    offset += sizeof(nvs_page); // Prepare to read next page in nvs
  }
  nvskeys[nvsinx][0] = '\0'; // Empty key at the end
  dbgprint("Read %d keys from NVS", nvsinx);
  bubbleSortKeys(nvsinx); // Sort the keys
}

//**************************************************************************************************
//                                           S E T U P                                             *
//**************************************************************************************************
// Setup for the program.                                                                          *
//**************************************************************************************************
void mp3Setup()
{
  int i;     // Loop control
  int pinnr; // Input pinnumber
  const char *p;
  byte mac[6];                  // WiFi mac address
  char tmpstr[20];              // For version and Mac address
  const char *partname = "nvs"; // Partition with NVS info
  esp_partition_iterator_t pi;  // Iterator for find
  const char *dtyp = "Display type is %s";
  const char *wvn = "Include file %s_html has the wrong version number! "
                    "Replace header file.";

  Serial.begin(115200); // For debug
  Serial.println();
  // Version tests for some vital include files
  if (about_html_version < 170626)
    dbgprint(wvn, "about");
  if (config_html_version < 180806)
    dbgprint(wvn, "config");
  if (index_html_version < 180102)
    dbgprint(wvn, "index");

  if (defaultprefs_version < 180816)
    dbgprint(wvn, "defaultprefs");
  // Print some memory and sketch info
  //setCpuFrequencyMhz(160);
  dbgprint("Starting ESP32-radio running on CPU %d at %d MHz.  Version %s.  Free memory %d",
           xPortGetCoreID(),
           ESP.getCpuFreqMHz(),
           VERSION,
           ESP.getFreeHeap()); // Normally about 170 kB

  maintask = xTaskGetCurrentTaskHandle(); // My taskhandle
  SPIsem = xSemaphoreCreateMutex();
  ;                                                  // Semaphore for SPI bus
  pi = esp_partition_find(ESP_PARTITION_TYPE_DATA,   // Get partition iterator for
                          ESP_PARTITION_SUBTYPE_ANY, // the NVS partition
                          partname);
  if (pi)
  {
    nvs = esp_partition_get(pi);        // Get partition struct
    esp_partition_iterator_release(pi); // Release the iterator
    dbgprint("Partition %s found, %d bytes",
             partname,
             nvs->size);
  }
  else
  {
    dbgprint("Partition %s not found!", partname); // Very unlikely...
    while (true)
      ; // Impossible to continue
  }
  namespace_ID = FindNsID(NAME);            // Find ID of our namespace in NVS
  fillkeylist();                            // Fill keynames with all keys
  memset(&ini_block, 0, sizeof(ini_block)); // Init ini_block
  ini_block.mqttport = 1883;                // Default port for MQTT
  ini_block.mqttprefix = "";                // No prefix for MQTT topics seen yet
  ini_block.clk_server = "pool.ntp.org";    // Default server for NTP
  ini_block.clk_offset = 1;                 // Default Amsterdam time zone
  ini_block.clk_dst = 1;                    // DST is +1 hour
  ini_block.bat0 = 0;                       // Battery ADC levels not yet defined
  ini_block.bat100 = 0;
  readIOprefs();                                   // Read pins used for SPI, TFT, VS1053, IR,
                                                   // Rotary encoder
  for (i = 0; (pinnr = progpin[i].gpio) >= 0; i++) // Check programmable input pins
  {
    pinMode(pinnr, INPUT_PULLUP); // Input for control button
    delay(10);
    // Check if pull-up active
    if ((progpin[i].cur = digitalRead(pinnr)) == HIGH)
    {
      p = "HIGH";
    }
    else
    {
      p = "LOW, probably no PULL-UP"; // No Pull-up
    }
    dbgprint("GPIO%d is %s", pinnr, p);
  }

  SPI.begin(ini_block.spi_sck_pin, // Init VSPI bus with default or modified pins
            ini_block.spi_miso_pin,
            ini_block.spi_mosi_pin);
  vs1053player = new VS1053(ini_block.vs_cs_pin, // Make instance of player
                            ini_block.vs_dcs_pin,
                            ini_block.vs_dreq_pin,
                            ini_block.vs_shutdown_pin,
                            ini_block.vs_shutdownx_pin);

  vs1053player->begin(); // Initialize VS1053 player
  delay(10);
  //vs1053player->switchToMp3Mode();
  vs1053player->setVolume(80);

  timer = timerBegin(0, 80, true);              // User 1st timer with prescaler 80
  timerAttachInterrupt(timer, &timer100, true); // Call timer100() on timer alarm
  timerAlarmWrite(timer, 100000, true);         // Alarm every 100 msec
  timerAlarmEnable(timer);                      // Enable the timer
  delay(1000);                                  // Show IP for a while
  // configTime(ini_block.clk_offset * 3600,
  //            ini_block.clk_dst * 3600,
  //            ini_block.clk_server.c_str()); // GMT offset, daylight offset in seconds
  // timeinfo.tm_year = 0;                     // Set TOD to illegal

  outchunk.datatyp = QDATA; // This chunk dedicated to QDATA
  // adc1_config_width(ADC_WIDTH_12Bit);
  // adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_0db);
  dataqueue = xQueueCreate(QSIZ, // Create queue for communication
                           sizeof(qdata_struct));
  xTaskCreatePinnedToCore(
      playtask,   // Task to play data in dataqueue.
      "Playtask", // name of task.
      20000,      // Stack size of task
      NULL,       // parameter of the task
      2,          // priority of the task
      &xplaytask, // Task handle to keep track of created task
      0);         // Run on CPU 0
  xTaskCreate(
      spftask,    // Task to handle special functions.
      "Spftask",  // name of task.
      20000,      // Stack size of task
      NULL,       // parameter of the task
      1,          // priority of the task
      &xspftask); // Task handle to keep track of created task
}

//**************************************************************************************************
//                                        R I N B Y T                                              *
//**************************************************************************************************
// Read next byte from http inputbuffer.  Buffered for speed reasons.                              *
//**************************************************************************************************
uint8_t rinbyt(bool forcestart)
{
  static uint8_t buf[1024]; // Inputbuffer
  static uint16_t i;        // Pointer in inputbuffer
  static uint16_t len;      // Number of bytes in buf
  uint16_t tlen;            // Number of available bytes
  uint16_t trycount = 0;    // Limit max. time to read

  if (forcestart || (i == len)) // Time to read new buffer
  {
    while (cmdclient.connected()) // Loop while the client's connected
    {
      tlen = cmdclient.available(); // Number of bytes to read from the client
      len = tlen;                   // Try to read whole input
      if (len == 0)                 // Any input available?
      {
        if (++trycount > 3) // Not for a long time?
        {
          dbgprint("HTTP input shorter than expected");
          return '\n'; // Error! No input
        }
        delay(10); // Give communication some time
        continue;  // Next loop of no input yet
      }
      if (len > sizeof(buf)) // Limit number of bytes
      {
        len = sizeof(buf);
      }
      len = cmdclient.read(buf, len); // Read a number of bytes from the stream
      i = 0;                          // Pointer to begin of buffer
      break;
    }
  }
  return buf[i++];
}

//**************************************************************************************************
//                                        W R I T E P R E F S                                      *
//**************************************************************************************************
// Update the preferences.  Called from the web interface.                                         *
//**************************************************************************************************
void writeprefs()
{
  int inx;              // Position in inputstr
  uint8_t winx;         // Index in wifilist
  char c;               // Input character
  String inputstr = ""; // Input regel
  String key, contents; // Pair for Preferences entry
  String dstr;          // Contents for debug

  timerAlarmDisable(timer); // Disable the timer
  nvsclear();               // Remove all preferences
  while (true)
  {
    c = rinbyt(false); // Get next inputcharacter
    if (c == '\n')     // Newline?
    {
      if (inputstr.length() == 0)
      {
        dbgprint("End of writing preferences");
        break; // End of contents
      }
      if (!inputstr.startsWith("#")) // Skip pure comment lines
      {
        inx = inputstr.indexOf("=");
        if (inx >= 0) // Line with "="?
        {
          key = inputstr.substring(0, inx); // Yes, isolate the key
          key.trim();
          contents = inputstr.substring(inx + 1); // and contents
          contents.trim();
          dstr = contents;                 // Copy for debug
          if ((key.indexOf("wifi_") == 0)) // Sensitive info?
          {
            winx = key.substring(5).toInt(); // Get index in wifilist
            if ((winx < wifilist.size()) &&  // Existing wifi spec in wifilist?
                (contents.indexOf(wifilist[winx].ssid) == 0) &&
                (contents.indexOf("/****") > 0)) // Hidden password?
            {
              contents = String(wifilist[winx].ssid) + // Retrieve ssid and password
                         String("/") +
                         String(wifilist[winx].passphrase);
              dstr = String(wifilist[winx].ssid) +
                     String("/*******"); // Hide in debug line
            }
          }
          if ((key.indexOf("mqttpasswd") == 0)) // Sensitive info?
          {
            if (contents.indexOf("****") == 0) // Hidden password?
            {
              contents = ini_block.mqttpasswd; // Retrieve mqtt password
            }
            dstr = String("*******"); // Hide in debug line
          }
          dbgprint("writeprefs setstr %s = %s",
                   key.c_str(), dstr.c_str());
          nvssetstr(key.c_str(), contents); // Save new pair
        }
      }
      inputstr = "";
    }
    else
    {
      if (c != '\r') // Not newline.  Is is a CR?
      {
        inputstr += String(c); // No, normal char, add to string
      }
    }
  }
  timerAlarmEnable(timer); // Enable the timer
  fillkeylist();           // Update list with keys
}

//**************************************************************************************************
//                                          X M L P A R S E                                        *
//**************************************************************************************************
// Parses line with XML data and put result in variable specified by parameter.                    *
//**************************************************************************************************
void xmlparse(String &line, const char *selstr, String &res)
{
  String sel = "</"; // Will be like "</status-code"
  int inx;           // Position of "</..." in line

  sel += selstr;          // Form searchstring
  if (line.endsWith(sel)) // Is this the line we are looking for?
  {
    inx = line.indexOf(sel);      // Get position of end tag
    res = line.substring(0, inx); // Set result
  }
}

//**************************************************************************************************
//                                          X M L G E T H O S T                                    *
//**************************************************************************************************
// Parses streams from XML data.                                                                   *
// Example URL for XML Data Stream:                                                                *
// http://playerservices.streamtheworld.com/api/livestream?version=1.5&mount=IHR_TRANAAC&lang=en   *
//**************************************************************************************************
String xmlgethost(String mount)
{
  const char *xmlhost = "playerservices.streamtheworld.com"; // XML data source
  const char *xmlget = "GET /api/livestream"                 // XML get parameters
                       "?version=1.5"                        // API Version of IHeartRadio
                       "&mount=%sAAC"                        // MountPoint with Station Callsign
                       "&lang=en";                           // Language

  String stationServer = ""; // Radio stream server
  String stationPort = "";   // Radio stream port
  String stationMount = "";  // Radio stream Callsign
  uint16_t timeout = 0;      // To detect time-out
  String sreply = "";        // Reply from playerservices.streamtheworld.com
  String statuscode = "200"; // Assume good reply
  char tmpstr[200];          // Full GET command, later stream URL
  String urlout;             // Result URL

  stop_mp3client(); // Stop any current wificlient connections.
  dbgprint("Connect to new iHeartRadio host: %s", mount.c_str());
  setdatamode(INIT);                      // Start default in metamode
  chunked = false;                        // Assume not chunked
  sprintf(tmpstr, xmlget, mount.c_str()); // Create a GET commmand for the request
  dbgprint("%s", tmpstr);
  if (mp3client.connect(xmlhost, 80)) // Connect to XML stream
  {
    dbgprint("Connected to %s", xmlhost);
    mp3client.print(String(tmpstr) + " HTTP/1.1\r\n"
                                     "Host: " +
                    xmlhost + "\r\n"
                              "User-Agent: Mozilla/5.0\r\n"
                              "Connection: close\r\n\r\n");
    while (mp3client.available() == 0)
    {
      delay(200);         // Give server some time
      if (++timeout > 25) // No answer in 5 seconds?
      {
        dbgprint("Client Timeout !");
      }
    }
    dbgprint("XML parser processing...");
    while (mp3client.available())
    {
      sreply = mp3client.readStringUntil('>');
      sreply.trim();
      // Search for relevant info in in reply and store in variable
      xmlparse(sreply, "status-code", statuscode);
      xmlparse(sreply, "ip", stationServer);
      xmlparse(sreply, "port", stationPort);
      xmlparse(sreply, "mount", stationMount);
      if (statuscode != "200") // Good result sofar?
      {
        dbgprint("Bad xml status-code %s", // No, show and stop interpreting
                 statuscode.c_str());
        tmpstr[0] = '\0'; // Clear result
        break;
      }
    }
    if ((stationServer != "") && // Check if all station values are stored
        (stationPort != "") &&
        (stationMount != ""))
    {
      sprintf(tmpstr, "%s:%s/%s_SC", // Build URL for ESP-Radio to stream.
              stationServer.c_str(),
              stationPort.c_str(),
              stationMount.c_str());
      dbgprint("Found: %s", tmpstr);
    }
  }
  else
  {
    dbgprint("Can't connect to XML host!"); // Connection failed
    tmpstr[0] = '\0';
  }
  mp3client.stop();
  return String(tmpstr); // Return final streaming URL.
}

//**************************************************************************************************
//                                           M P 3 L O O P                                         *
//**************************************************************************************************
// Called from the mail loop() for the mp3 functions.                                              *
// A connection to an MP3 server is active and we are ready to receive data.                       *
// Normally there is about 2 to 4 kB available in the data stream.  This depends on the sender.    *
//**************************************************************************************************
void mp3Loop()
{
  uint32_t maxchunk; // Max number of bytes to read
  int res = 0;       // Result reading from mp3 stream
  uint32_t av = 0;   // Available in stream
  String nodeID;     // Next nodeID of track on SD
  uint32_t timing;   // Startime and duration this function
  uint32_t qspace;   // Free space in data queue
  // Serial.print("datamode:");
  // Serial.println(datamode);
  // Try to keep the Queue to playtask filled up by adding as much bytes as possible
  if (datamode & (INIT | HEADER | DATA | // Test op playing
                  METADATA | PLAYLISTINIT |
                  PLAYLISTHEADER |
                  PLAYLISTDATA))
  {                                              //Serial.print("...,");
    timing = millis();                           // Start time this function
    maxchunk = sizeof(tmpbuff);                  // Reduce byte count for this mp3loop()
    qspace = uxQueueSpacesAvailable(dataqueue) * // Compute free space in data queue
             sizeof(qdata_struct);

    av = mp3client.available(); // Available from stream
    if (av < maxchunk)          // Limit read size
    {
      maxchunk = av;
    }
    if (maxchunk > qspace) // Enough space in queue?
    {
      maxchunk = qspace; // No, limit to free queue space
    }
    if (maxchunk) // Anything to read?
    {
      res = mp3client.read(tmpbuff, maxchunk); // Read a number of bytes from the stream
    }

    if (maxchunk == 0)
    {
      if (datamode == PLAYLISTDATA) // End of playlist
      {
        playlist_num = 1; // Yes, restart playlist
        dbgprint("End of playlist seen");
        setdatamode(STOPPED);
        ini_block.newpreset++; // Go to next preset
      }
    }
    for (int i = 0; i < res; i++)
    {
      handlebyte_ch(tmpbuff[i]); // Handle one byte
    }
    timing = millis() - timing;    // Duration this function
    if (timing > max_mp3loop_time) // New maximum found?
    {
      max_mp3loop_time = timing;               // Yes, set new maximum
      dbgprint("Duration mp3loop %d", timing); // and report it
    }
  }
  if (datamode == STOPREQD) // STOP requested?
  {
    dbgprint("STOP requested");

    stop_mp3client(); // Disconnect if still connected

    chunked = false;      // Not longer chunked
    datacount = 0;        // Reset datacount
    outqp = outchunk.buf; // and pointer
    queuefunc(QSTOPSONG); // Queue a request to stop the song
    metaint = 0;          // No metaint known now
    setdatamode(STOPPED); // Yes, state becomes STOPPED
    return;
  }

  if (hostreq) //
  {
    hostreq = false;
    currentpreset = ini_block.newpreset; // Remember current preset

    connecttohost(mp3hostS, mp3pathS, mp3port); // Switch to new host
  }
}

//**************************************************************************************************
//                                    C H K H D R L I N E                                          *
//**************************************************************************************************
// Check if a line in the header is a reasonable headerline.                                       *
// Normally it should contain something like "icy-xxxx:abcdef".                                    *
//**************************************************************************************************
bool chkhdrline(const char *str)
{
  char b;      // Byte examined
  int len = 0; // Lengte van de string

  while ((b = *str++)) // Search to end of string
  {
    len++;           // Update string length
    if (!isalpha(b)) // Alpha (a-z, A-Z)
    {
      if (b != '-') // Minus sign is allowed
      {
        if (b == ':') // Found a colon?
        {
          return ((len > 5) && (len < 50)); // Yes, okay if length is okay
        }
        else
        {
          return false; // Not a legal character
        }
      }
    }
  }
  return false; // End of string without colon
}

//**************************************************************************************************
//                            S C A N _ C O N T E N T _ L E N G T H                                *
//**************************************************************************************************
// If the line contains content-length information: set clength (content length counter).          *
//**************************************************************************************************
void scan_content_length(const char *metalinebf)
{
  if (strstr(metalinebf, "Content-Length")) // Line contains content length
  {
    clength = atoi(metalinebf + 15);           // Yes, set clength
    dbgprint("Content-Length is %d", clength); // Show for debugging purposes
  }
}

//**************************************************************************************************
//                                   H A N D L E B Y T E _ C H                                     *
//**************************************************************************************************
// Handle the next byte of data from server.                                                       *
// Chunked transfer encoding aware. Chunk extensions are not supported.                            *
//**************************************************************************************************
void handlebyte_ch(uint8_t b)
{
  static int chunksize = 0;    // Chunkcount read from stream
  static uint16_t playlistcnt; // Counter to find right entry in playlist
  static int LFcount;          // Detection of end of header
  static bool ctseen = false;  // First line of header seen or not

  if (chunked &&
      (datamode & (DATA | // Test op DATA handling
                   METADATA |
                   PLAYLISTDATA)))
  {
    if (chunkcount == 0) // Expecting a new chunkcount?
    {
      if (b == '\r') // Skip CR
      {
        return;
      }
      else if (b == '\n') // LF ?
      {
        chunkcount = chunksize; // Yes, set new count
        chunksize = 0;          // For next decode
        return;
      }
      // We have received a hexadecimal character.  Decode it and add to the result.
      b = toupper(b) - '0'; // Be sure we have uppercase
      if (b > 9)
      {
        b = b - 7; // Translate A..F to 10..15
      }
      chunksize = (chunksize << 4) + b;
      return;
    }
    chunkcount--; // Update count to next chunksize block
  }
  if (datamode == DATA) // Handle next byte of MP3/Ogg data
  {
    *outqp++ = b;
    if (outqp == (outchunk.buf + sizeof(outchunk.buf))) // Buffer full?
    {
      // Send data to playtask queue.  If the buffer cannot be placed within 200 ticks,
      // the queue is full, while the sender tries to send more.  The chunk will be dis-
      // carded it that case.
      xQueueSend(dataqueue, &outchunk, 200); // Send to queue
      outqp = outchunk.buf;                  // Item empty now
    }
    if (metaint) // No METADATA on Ogg streams or mp3 files
    {
      if (--datacount == 0) // End of datablock?
      {
        setdatamode(METADATA);
        metalinebfx = -1; // Expecting first metabyte (counter)
      }
    }
    return;
  }
  if (datamode == INIT) // Initialize for header receive
  {
    ctseen = false; // Contents type not seen yet
    metaint = 0;    // No metaint found
    LFcount = 0;    // For detection end of header
    bitrate = 0;    // Bitrate still unknown
    dbgprint("Switch to HEADER");
    setdatamode(HEADER); // Handle header
    totalcount = 0;      // Reset totalcount
    metalinebfx = 0;     // No metadata yet
    metalinebf[0] = '\0';
  }
  if (datamode == HEADER) // Handle next byte of MP3 header
  {
    if ((b > 0x7F) ||  // Ignore unprintable characters
        (b == '\r') || // Ignore CR
        (b == '\0'))   // Ignore NULL
    {
      // Yes, ignore
    }
    else if (b == '\n') // Linefeed ?
    {
      LFcount++;                      // Count linefeeds
      metalinebf[metalinebfx] = '\0'; // Take care of delimiter
      if (chkhdrline(metalinebf))     // Reasonable input?
      {
        dbgprint("Headerline: %s", // Show headerline
                 metalinebf);
        String metaline = String(metalinebf); // Convert to string
        String lcml = metaline;               // Use lower case for compare
        lcml.toLowerCase();
        if (lcml.startsWith("location: http://")) // Redirection?
        {
          //host = metaline.substring(17); // Yes, get new URL
          hostreq = true; // And request this one
        }
        if (lcml.indexOf("content-type") >= 0) // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true;                      // Yes, remember seeing this
          String ct = metaline.substring(13); // Set contentstype. Not used yet
          ct.trim();
          dbgprint("%s seen.", ct.c_str());
        }
        if (lcml.startsWith("icy-br:"))
        {
          bitrate = metaline.substring(7).toInt(); // Found bitrate tag, read the bitrate
          if (bitrate == 0)                        // For Ogg br is like "Quality 2"
          {
            bitrate = 87; // Dummy bitrate
          }
        }
        else if (lcml.startsWith("icy-metaint:"))
        {
          metaint = metaline.substring(12).toInt(); // Found metaint tag, read the value
        }

        else if (lcml.startsWith("transfer-encoding:"))
        {
          // Station provides chunked transfer
          if (lcml.endsWith("chunked"))
          {
            chunked = true; // Remember chunked transfer mode
            chunkcount = 0; // Expect chunkcount in DATA
          }
        }
      }
      metalinebfx = 0;              // Reset this line
      if ((LFcount == 2) && ctseen) // Content type seen and a double LF?
      {
        dbgprint("Switch to DATA, bitrate is %d" // Show bitrate
                 ", metaint is %d",              // and metaint
                 bitrate, metaint);
        setdatamode(DATA);     // Expecting data now
        datacount = metaint;   // Number of bytes before first metadata
        queuefunc(QSTARTSONG); // Queue a request to start song
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b; // Normal character, put new char in metaline
      if (metalinebfx >= METASIZ)          // Prevent overflow
      {
        metalinebfx--;
      }
      LFcount = 0; // Reset double CRLF detection
    }
    return;
  }
  if (datamode == METADATA) // Handle next byte of metadata
  {
    if (metalinebfx < 0) // First byte of metadata?
    {
      metalinebfx = 0;        // Prepare to store first character
      metacount = b * 16 + 1; // New count for metadata including length byte
      if (metacount > 1)
      {
        dbgprint("Metadata block %d bytes",
                 metacount - 1); // Most of the time there are zero bytes of metadata
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b; // Normal character, put new char in metaline
      if (metalinebfx >= METASIZ)          // Prevent overflow
      {
        metalinebfx--;
      }
    }
    if (--metacount == 0)
    {
      metalinebf[metalinebfx] = '\0'; // Make sure line is limited
      if (strlen(metalinebf))         // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle(metalinebf); // Show artist and title if present in metadata
        //mqttpub.trigger(MQTT_STREAMTITLE); // Request publishing to MQTT
      }
      if (metalinebfx > (METASIZ - 10)) // Unlikely metaline length?
      {
        dbgprint("Metadata block too long! Skipping all Metadata from now on.");
        metaint = 0; // Probably no metadata
      }
      datacount = metaint; // Reset data count
      //bufcnt = 0 ;                                   // Reset buffer count
      setdatamode(DATA); // Expecting data
    }
  }
  if (datamode == PLAYLISTINIT) // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metalinebfx = 0; // Prepare for new line
    LFcount = 0;     // For detection end of header
    if (localfile)   // SD-card mode?
    {
      setdatamode(PLAYLISTDATA); // Yes, no header here
    }
    else
    {
      setdatamode(PLAYLISTHEADER); // Handle playlist header
    }
    playlistcnt = 1;      // Reset for compare
    totalcount = 0;       // Reset totalcount
    clength = 0xFFFFFFFF; // Content-length unknown
    dbgprint("Read from playlist");
  }
  if (datamode == PLAYLISTHEADER) // Read header
  {
    if ((b > 0x7F) ||  // Ignore unprintable characters
        (b == '\r') || // Ignore CR
        (b == '\0'))   // Ignore NULL
    {
      return; // Quick return
    }
    else if (b == '\n') // Linefeed ?
    {
      LFcount++;                      // Count linefeeds
      metalinebf[metalinebfx] = '\0'; // Take care of delimeter
      dbgprint("Playlistheader: %s",  // Show playlistheader
               metalinebf);
      scan_content_length(metalinebf); // Check if it is a content-length line
      metalinebfx = 0;                 // Ready for next line
      if (LFcount == 2)
      {
        dbgprint("Switch to PLAYLISTDATA, " // For debug
                 "search for entry %d",
                 playlist_num);
        setdatamode(PLAYLISTDATA); // Expecting data now
        //mqttpub.trigger(MQTT_PLAYLISTPOS); // Playlistposition to MQTT
        return;
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b; // Normal character, put new char in metaline
      if (metalinebfx >= METASIZ)          // Prevent overflow
      {
        metalinebfx--;
      }
      LFcount = 0; // Reset double CRLF detection
    }
  }
}

//**************************************************************************************************
//                                     G E T C O N T E N T T Y P E                                 *
//**************************************************************************************************
// Returns the contenttype of a file to send.                                                      *
//**************************************************************************************************
String getContentType(String filename)
{
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".mp3"))
    return "audio/mpeg";
  else if (filename.endsWith(".pw"))
    return ""; // Passwords are secret
  return "text/plain";
}

//**************************************************************************************************
//                                         C H O M P                                               *
//**************************************************************************************************
// Do some filtering on de inputstring:                                                            *
//  - String comment part (starting with "#").                                                     *
//  - Strip trailing CR.                                                                           *
//  - Strip leading spaces.                                                                        *
//  - Strip trailing spaces.                                                                       *
//**************************************************************************************************
void chomp(String &str)
{
  int inx; // Index in de input string

  if ((inx = str.indexOf("#")) >= 0) // Comment line or partial comment?
  {
    str.remove(inx); // Yes, remove
  }
  str.trim(); // Remove spaces and CR
}

//**************************************************************************************************
//* Function that are called from spftask.                                                         *
//* Note that some device dependent function are place in the *.h files.                           *
//**************************************************************************************************

//**************************************************************************************************
//                                     P L A Y T A S K                                             *
//**************************************************************************************************
// Play stream data from input queue.                                                              *
// Handle all I/O to VS1053B during normal playing.                                                *
// Handles display of text, time and volume on TFT as well.                                        *
//**************************************************************************************************
void playtask(void *parameter)
{
  while (true)
  {
    if (xQueueReceive(dataqueue, &inchunk, 5))
    {                                       //Serial.println("---,");
      while (!vs1053player->data_request()) // If FIFO is full..
      {
        vTaskDelay(1); // Yes, take a break
      }
      switch (inchunk.datatyp) // What kind of chunk?
      {
      case QDATA:
        // Serial.println("chunk,");
        claimSPI("chunk");                   // Claim SPI bus
        vs1053player->playChunk(inchunk.buf, // DATA, send to player
                                sizeof(inchunk.buf));
        releaseSPI();                      // Release SPI bus
        totalcount += sizeof(inchunk.buf); // Count the bytes
        break;
      case QSTARTSONG:
        playingstat = 1; // Status for MQTT
        //Serial.println("start,");
        //mqttpub.trigger(MQTT_PLAYING); // Request publishing to MQTT
        claimSPI("startsong");     // Claim SPI bus
        vs1053player->startSong(); // START, start player
        releaseSPI();              // Release SPI bus
        break;
      case QSTOPSONG:
        playingstat = 0; // Status for MQTT
        //Serial.println("stop,");
        //mqttpub.trigger(MQTT_PLAYING); // Request publishing to MQTT
        claimSPI("stopsong");        // Claim SPI bus
        vs1053player->setVolume(80); // Mute
        vs1053player->stopSong();    // STOP, stop player
        releaseSPI();                // Release SPI bus
        while (xQueueReceive(dataqueue, &inchunk, 0))
          ;                                   // Flush rest of queue
        vTaskDelay(500 / portTICK_PERIOD_MS); // Pause for a short time
        break;
      default:
        break;
      }
    }
    //esp_task_wdt_reset() ;                                        // Protect against idle cpu
  }
  vTaskDelay(1000);
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}

//**************************************************************************************************
//                                   H A N D L E _ S P E C                                         *
//**************************************************************************************************
// Handle special (non-stream data) functions for spftask.                                         *
//**************************************************************************************************
void handle_spec()
{

  claimSPI("hspec"); // Claim SPI bus
  if (muteflag)      // Mute or not?
  {
    vs1053player->setVolume(80); // Mute
  }
  else
  {
    vs1053player->setVolume(80); //ini_block.reqvol); // Unmute
  }
  if (reqtone) // Request to change tone?
  {
    reqtone = false;
    vs1053player->setTone(ini_block.rtone); // Set SCI_BASS to requested value
  }

  releaseSPI(); // Release SPI bus
}

//**************************************************************************************************
//                                     S P F T A S K                                               *
//**************************************************************************************************
// Handles display of text, time and volume on TFT.                                                *
// Handles ADC meassurements.                                                                      *
// This task runs on a low priority.                                                               *
//**************************************************************************************************
void spftask(void *parameter)
{
  while (true)
  {
    handle_spec();                        // Maybe some special funcs?
    vTaskDelay(100 / portTICK_PERIOD_MS); // Pause for a short time
    // adcval = (15 * adcval +               // Read ADC and do some filtering
    //           adc1_get_raw(ADC1_CHANNEL_0)) /
    //          16;
  }
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}
