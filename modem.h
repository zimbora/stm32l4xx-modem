/*
 * modem.h
 *
 *  Created on: Nov 12, 2018
 *      Author: jsilva
 */

#ifndef INC_MODEM_H_
#define INC_MODEM_H_


#include <timestamp.h>
#include "stm32l4xx_hal.h"
#include "malloc.h"
#include "string.h"
#include "strings.h"
#include "strstream" // to use string import this
#include "queue"

//#include "package.h"
#include "serial.h"
#include "logging.h"


#ifdef BG95
  #include "Modem_BGxx.h"
#elif defined BC660
  #include "Modem_BC660.h"
#endif

using namespace std;

//#define MODEMRXBUFFERSIZE 255
#define MODEMRXBUFFERSIZE RXBUFFERSIZE
#define MODEMTXBUFFERSIZE 255

#ifdef __cplusplus
 extern "C" {
#endif


class MODEM{
public:
  MODEM();
  void restart();
  bool send(const char* command, uint16_t size);
  void sendAtCommand(const char* command);
  void sendAtCommand(string command);
  bool checkAtCommand(const char* command, const char* response, uint32_t timeout);
  string getAtCommandResponse(const char* command, uint32_t timeout);
  string getAtCommandResponse(const char* command, const char* filter, uint32_t timeout);
  string getAtCommandResponseSMS(const char* command, const char* filter, uint32_t timeout);
  string getAtCommandResponseNoOK(const char* command, const char* filter, uint32_t timeout);
  bool getUnsolicitedCode(const char* filter,uint32_t timeout);
  bool getRequest(const char* filter,uint32_t timeout);
  uint16_t check_request(uint8_t connect_id,uint16_t length);
  uint16_t read_buffer(uint8_t* data,uint16_t length);
  uint16_t read_line(uint8_t* data, uint16_t length);
  void checkMessages();
  void checkMessagesNoParse();
  void checkMessagesSMS();

private:

  void parseMessage();
  void parseMessageSMS();
  void evaluateMessage(char* p_data);
  void sendData(char* data, uint16_t size);

  int find(string text, string word);

  uint8_t response[MODEMRXBUFFERSIZE];
  uint16_t response_size;
  uint16_t tail;
  char command[MODEMTXBUFFERSIZE];
  uint8_t command_size;
};

//void logging_loop(uint32_t interval);

/*
void log(String text);
void log(String key, uint16_t value);
void log(String key, uint8_t data[], uint8_t len);
void log(String key, char* value);
void log_progress();
void log_progress_end();

String pad2(int value);
String date();
String dec2hex(uint8_t n);
*/

#ifdef __cplusplus
}
#endif


#endif /* INC_MODEM_H_ */
