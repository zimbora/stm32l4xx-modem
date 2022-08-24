/*
 * modem.cpp

 *
 *  Created on: Nov 12, 2018
 *      Author: jsilva
 */
#include <modem.h>

Serial *serial_modem = NULL;

//extern LOGGING logging;

#ifdef BG95
  extern Modem_BGxx* modem;
#elif defined BC660
	extern Modem_BC660* modem;
#endif


// init modem class
MODEM::MODEM(){

	response_size = 0;
	if(serial_modem == NULL)
		serial_modem = new Serial();
	#ifdef DEBUG_MODEM_INFO
		logging.println("modem","baudrate: ",(long)settings.modem.baudrate);
		logging.println("modem","uart: ",(int)settings.modem.uart);
	#endif
  serial_modem->begin(settings.modem.uart,settings.modem.baudrate);
}

void MODEM::restart(){
	#ifdef DEBUG_MODEM_INFO
		logging.println("modem","baudrate: ",(long)settings.modem.baudrate);
		logging.println("modem","uart: ",(int)settings.modem.uart);
	#endif
	serial_modem->begin(settings.modem.uart,settings.modem.baudrate);
}

bool MODEM::send(const char* data, uint16_t size){

	checkMessages();

	sendData((char*)data,size);

	uint8_t i = 0;
	while(i < 10){
		HAL_Delay(100);
		checkMessages();
		if(response_size > 0){
			const char* s = (const char*) response;
			string r = string(s);
			int index = find(r,"SEND OK");
			if(index > -1)
				return true;
		}
		i++;
	}
	return false;
}


void MODEM::sendAtCommand(string command){
	sendAtCommand((const char*)command.c_str());
}

void MODEM::sendAtCommand(const char* at_command){

	if(strlen(at_command) > MODEMTXBUFFERSIZE - 10){
		if(DEBUG_MODEM <=  WARNING_DEBUG_LEVEL)
			logging.println("modem","!! at command too large:",(int)strlen(at_command));
		return;
	}

	checkMessages();

	memset(command,0,MODEMTXBUFFERSIZE);
	strcpy(command,at_command);
	strcat(&command[(uint16_t)strlen(at_command)],"\r\n");

	sendData(command,(uint16_t)strlen(at_command)+2);

	HAL_Delay(100);

}

bool MODEM::checkAtCommand(const char* command, const char* response_expected, uint32_t timeout){

	memset(response,0,MODEMRXBUFFERSIZE);

	sendAtCommand(command);
	HAL_Delay(10);
	uint32_t time = timeout + millis();

	while(time > millis()){
		checkMessages();
		if(response_size > 0){
			const char* aa = (const char*) response;
			string s = string(aa);
			if(find(s,string(response_expected)) > -1){
				if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
					logging.log((char*)"response expected","modem");
				return true;
			}else if(find(s,string("ERROR")) > -1){
				if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
					logging.log((char*)response,"modem");
				return false;
			}
		}
		clear_WDT();
		HAL_Delay(10);
	}

	if(DEBUG_MODEM <=  WARNING_DEBUG_LEVEL)
		logging.log((char*)"no response","modem");
	serial_modem->begin(settings.modem.uart,settings.modem.baudrate);

	return false;
}

string MODEM::getAtCommandResponse(const char* command, uint32_t timeout){

	sendAtCommand(command);

	string data = "";
	uint32_t time = timeout + millis();
	while(time > millis()){
		checkMessages();
		if(response_size > 0){
			const char* s = (const char*) response;
			string r = string(s);
			int index = find(r,"ERROR");
			if(index == -1){  // check if no error was returned
				index = find(r,"OK");
				if(index > -1){
					data += r.substr(0,index);
					return data;
				}else
					data += r;
			}else{
				return "";
			}
		}
		clear_WDT();
		HAL_Delay(100);
	}

	return (string)"";
}

string MODEM::getAtCommandResponse(const char* command, const char* filter, uint32_t timeout){

	sendAtCommand(command);

	string data = "";
	uint32_t time = timeout + millis();
	while(time > millis()){
		checkMessages();
		if(response_size > 0){
			const char* s = (const char*) response;
			int index = find(s,"ERROR");
			if(index == -1){  // check if no error was returned
				string r = string(s);
				string f = string(filter);
				int index = find(r,f);
				if(index > -1){
					string d = r.substr(index+f.length());
					index = find(d,"OK");
					if(index > -1){
						d = d.substr(0,index);
						return d;
					}else
						d += r;
				}
			}else{
				return "";
			}
		}
		clear_WDT();
		HAL_Delay(10);
	}

	return (string)"";
}

string MODEM::getAtCommandResponseSMS(const char* command, const char* filter, uint32_t timeout){

	sendAtCommand(command);

	string data = "";
	uint32_t time = timeout + millis();
	while(time > millis()){
		checkMessagesSMS();
		if(response_size > 0){
			const char* s = (const char*) response;
			int index = find(s,"ERROR");
			if(index == -1){  // check if no error was returned
				string r = string(s);
				string f = string(filter);
				int index = find(r,f);
				if(index > -1){
					data = r.substr(index+f.length());
					index = find(data,"OK");
					if(index > -1){
						data = data.substr(0,index);
						return data;
					}else
						data += r;
				}
			}else{
				return "";
			}
		}
		clear_WDT();
		HAL_Delay(10);
	}

	return (string)data;
}

string MODEM::getAtCommandResponseNoOK(const char* command, const char* filter, uint32_t timeout){

	sendAtCommand(command);

	uint32_t time = timeout + millis();
	while(time > millis()){
		checkMessages();
		if(response_size > 0){
			const char* s = (const char*) response;
			int index = find(s,"ERROR");
			if(index == -1){  // check if no error was returned
				string r = string(s);
				string f = string(filter);
				int index = find(r,f);
				if(index > -1){
					string d = r.substr(index+f.length());
					return d;
				}
			}else{
				return "";
			}
		}
		clear_WDT();
		HAL_Delay(10);
	}

	return (string)"";
}

bool MODEM::getUnsolicitedCode(const char* filter,uint32_t timeout){

	uint32_t time = timeout + millis();
	while(time > millis()){
		checkMessages();
		if(response_size > 0){
			const char* s = (const char*) response;
			int index = find(s,"ERROR");
			if(index == -1){  // check if no error was returned
				string r = string(s);
				string f = string(filter);
				int index = find(r,f);
				if(index > -1)
					return true;
			}else
				return false;

		}
		HAL_Delay(10);
	}

	return false;
}

bool MODEM::getRequest(const char* filter,uint32_t timeout){

	uint32_t time = timeout + millis();
	while(time > millis()){
		checkMessagesNoParse();
		if(response_size > 0){
			const char* s = (const char*) response;
			int index = find(s,"ERROR");
			if(index == -1){  // check if no error was returned
				string r = string(s);
				string f = string(filter);
				int index = find(r,f);
				if(index > -1)
					return true;
			}else
				return false;

		}
		HAL_Delay(10);
	}

	return false;
}

uint16_t MODEM::check_request(uint8_t connect_id,uint16_t length){
	checkMessagesNoParse();
	string f = "\r\n+QIRD: ";
	uint16_t len = 0;

	uint32_t timeout = millis()+5000;
	while(timeout > millis()){
		if(response_size > 0){
			const char* s = (const char*) response;
			int index = find(s,"ERROR");
			if(index == -1){  // check if no error was returned
				string r = string(s);
				int index = find(r,f);
				if(index == -1)
					return 0;

				r = r.substr(index+f.length());
				tail += index+f.length();

				index = find(r,"\r\n");
				if(index == -1)
					return 0;

				tail += index+2;
				string bytes = r.substr(0,index);

				if(has_only_digits(bytes)){
					len = (uint16_t)std::stoul(bytes);
					response_size = tail + len;
				}

				/*
				index = find(r,"\r\nOK\r\n");
				if(index > -1){
					response_size -= 6;
				}
				*/

				return len;

			}else
				return 0;

		}
	}
	return 0;
}

uint16_t MODEM::read_buffer(uint8_t* data,uint16_t length){

	uint16_t i = 0;

	if(tail == response_size)
		checkMessagesNoParse();

	if(tail < response_size && i < length){
		while(tail<response_size){
			data[i++] = response[tail++];
		}
	}

	return i;
}

uint16_t MODEM::read_line(uint8_t* data, uint16_t length){

	uint16_t i = 0;

	//logging.println("modem","tail",(int)tail);
	//logging.println("modem","response_size",(int)response_size);

	if(tail == response_size)
		checkMessagesNoParse();

	uint8_t prev = 0x00;
	if(tail < response_size){
		while(tail<response_size && i < length){

			if(i > 0)
				prev = data[i-1];

			data[i++] = response[tail++];

			if(prev == 0x0D && data[i-1] == 0x0A){
				return i;
			}
		}
	}

	return i;
}

// check if there are data available on buffer, in case there are reads that data
void MODEM::checkMessages(){

	memset(response,0,MODEMRXBUFFERSIZE);
	response_size = 0;
	while(serial_modem->available()>0){
	//if(serial_modem->available()>0){
		response_size += serial_modem->read(&response[response_size], MODEMRXBUFFERSIZE-response_size);
		HAL_Delay(20); // this delay ensures that all message is received
		//evaluateMessage();
	}

	if(response_size > 0)
		parseMessage();
}

// check if there are data available on buffer, in case there are, reads that data
void MODEM::checkMessagesNoParse(){

	memset(response,0,MODEMRXBUFFERSIZE);
	response_size = 0;
	tail = 0;
	while(serial_modem->available()>0 && response_size != MODEMRXBUFFERSIZE){
	//if(serial_modem->available()>0){
		response_size += serial_modem->read(&response[response_size], MODEMRXBUFFERSIZE-response_size);
		//logging.println("modem","reponse_size: ",(int)response_size);
		HAL_Delay(2); // this delay ensures that all message is received
		//evaluateMessage();
	}

}

// check if there are data available on buffer, in case there are reads that data
void MODEM::checkMessagesSMS(){

	memset(response,0,MODEMRXBUFFERSIZE);
	response_size = 0;
	while(serial_modem->available()>0){
	//if(serial_modem->available()>0){
		response_size += serial_modem->read(&response[response_size], MODEMRXBUFFERSIZE-response_size);
		HAL_Delay(2); // this delay ensures that all message is received
		//evaluateMessage();
	}

	if(response_size > 0)
		parseMessageSMS();
}

// -------------------
// private
// -------------------


// if there is data on buffer, it tries to decode that data in messages
void MODEM::parseMessage(){

	//logging.log("parsing message","modem");

	char c = 0;
	char prev_c = 0;
	uint16_t i = 0;
	char* parse = (char*)malloc(response_size);
	char* ptr_tmp = (char*)malloc(response_size);
	uint8_t tmp_index = 0;
	if(ptr_tmp == nullptr)
			return;
	bool init = false;


	if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
		logging.log((char*)response,"modem","<<");

	memcpy(ptr_tmp,response,response_size);

	memset(parse,0,response_size);
	memset(response,0,response_size);
	uint8_t inc = 0;
	while(i < response_size){
		c = ptr_tmp[i];
		if(prev_c == '\r' && c == '\n'){
			if(!init){
			 	init = true;
			}else{
				response[--tmp_index] = c; // remove \r and add \n
				parse[--inc] = c; // remove \r and add \n
				init = false;
				tmp_index++;
				inc++;
				if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
					logging.log((char*)parse,"modem");
        #ifdef BG95 or defined BC660
				modem->parseResponse(string(parse));  // !! implement this function or use BG95 library to do it for you
        #endif
				memset(parse,0,inc);
				inc = 0;
			}
		}else{
			if(init){
				response[tmp_index++] = c;
				parse[inc++] = c;
			}
			prev_c = c;
		}
		i++;
	}

	response_size = tmp_index;

	free(ptr_tmp);
	free(parse);
}

// if there is data on buffer, it tries to decode that data in messages
void MODEM::parseMessageSMS(){

	//logging.log("parsing message","modem");

	char c = 0;
	char prev_c = 0;
	uint8_t i = 0;
	char* parse = (char*)malloc(response_size);
	char* ptr_tmp = (char*)malloc(response_size);
	uint8_t tmp_index = 0;
	if(ptr_tmp == nullptr || parse == nullptr)
			return;
	bool init = false;

	if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
		logging.log((char*)response,"modem","<<");

	memcpy(ptr_tmp,response,response_size);

	memset(parse,0,response_size);
	memset(response,0,response_size);
	uint8_t inc = 0;
	while(i < response_size){
		c = ptr_tmp[i];
		if(prev_c == '\r' && c == '\n'){
			if(!init){
			 	init = true;
			}else{
				response[--tmp_index] = c; // remove \r and add \n
				parse[--inc] = c; // remove \r and add \n
				//init = false;
				tmp_index++;
				inc++;
				if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
					logging.log((char*)parse,"modem");
        #ifdef BG95 or defined BC660
				modem->parseResponse(string(parse));  // !! implement this function or use BG95 library to do it for you
        #endif
				memset(parse,0,inc);
				inc = 0;
			}
		}else{
			if(init){
				response[tmp_index++] = c;
				parse[inc++] = c;
			}
			prev_c = c;
		}
		i++;
	}

	response_size = tmp_index;

	free(ptr_tmp);
	free(parse);
}

void MODEM::sendData(char* data, uint16_t size){

  serial_modem->write(data,size);
  if(DEBUG_MODEM <=  VERBOSE_DEBUG_LEVEL)
		logging.log(data,"modem", ">>");
}

int MODEM::find(string text, string word){
	//logging.log((char*)text.c_str(),"find");
	//logging.log((char*)word.c_str(),"find");
  std::size_t found = text.find(word);
  if (found!=std::string::npos)
    return (int)found;
  else return -1;

}
