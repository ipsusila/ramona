#include <Arduino.h>
#include "WizClientAuto.h"

/*
atv0 - disable ascii
atv0 - 

at
at
atc0
at+wd
at+ndhcp=1
at+wwpa=monLink2015
at+wa=MonLink
at&w0

//connect
at+nctcp=<ip>,port
at+ncloseall
at+nclose=<cid>
*/


static int _resp;

#define ESC				0x1B
#define WAITSERIAL(us) 	do { if (Serial.available() > 0) break; delayMicroseconds(us); } while(1)

static inline void sendAT(const char *cmd) {
	int resp = -1;
	do {
		Serial.print(cmd);
		WAITSERIAL(5);
		resp = Serial.read();
	} while (resp != '0');
}

static inline void sendAT2(const char *cmd, const char *data) {
	int resp = -1;
	do {
		Serial.print(cmd);
		Serial.print(data);
		Serial.print('\r');
		WAITSERIAL(5);
		resp = Serial.read();
	} while (resp != '0');
}

static inline void dischargeASCII() {
	int resp;
	bool ok = false;
	bool lf = false;
	do {
		if (Serial.available() > 0) {
			resp = Serial.read();
			if (lf) 
				ok = resp == '\n';
			else if (resp == ']')
				lf = true;
		}
	} while (!ok);
}


WizClientAuto::WizClientAuto()
{
	valid = false;
}
void WizClientAuto::setup(uint32_t baud, const char *ssid, const char *key)
{
	Serial.begin(baud);
	//AT mode
	//disable ascii response
	Serial.print("atv0\r");
	WAITSERIAL(5);
	while(Serial.available() > 0)
		Serial.read();
	
	//disable echo
	Serial.print("ate0\r");
	WAITSERIAL(5);
	while(Serial.available() > 0)
		Serial.read();
	
	sendAT("atv0\r");
	
	//disable autoconnect
	sendAT("atc0\r");
	
	//disassociate
	sendAT("at+wd\r");
	
	//setup dhcp
	sendAT("at+ndhcp=1\r");
	
	//setup wpa
	sendAT2("at+wwpa=", key);
	
	//asciimode associate
	Serial.print("atv1\r");
	dischargeASCII();
	Serial.print("at+wa=");
	Serial.print(ssid);
	Serial.print('\r');
	dischargeASCII();
}
int WizClientAuto::connect(IPAddress ip, uint16_t port)
{
	char host[16];
	sprintf(host, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return connect(host, port);
}
int WizClientAuto::connect(const char *host, uint16_t port)
{
	valid = false;
	
	//enable ascii
	Serial.print("atv1\r");
	dischargeASCII();
	
	Serial.print("at+nauto=0,1,");
	Serial.print(host);
	Serial.print(',');
	Serial.print(port);
	Serial.print('\r');		
	dischargeASCII();
	
	Serial.print("ata2\r");
	dischargeASCII();
	valid = true; 
}
size_t WizClientAuto::write(uint8_t d)
{
	size_t sz = Serial.write(d);
	Serial.flush();
	return sz;
}
size_t WizClientAuto::write(const uint8_t *buf, size_t size)
{
	size_t sz = Serial.write(buf, size);
	Serial.flush();
	return sz;
}
int WizClientAuto::available()
{
	return Serial.available();
}
int WizClientAuto::read()
{
	return Serial.read();
}
int WizClientAuto::read(uint8_t *buf, size_t size)
{
	return Serial.readBytes(buf, size);
}
int WizClientAuto::peek()
{
	return Serial.peek();
}
void WizClientAuto::flush()
{
	Serial.flush();
}
void WizClientAuto::stop()
{
	if (valid) {
		Serial.write("+++");
		delay(2200);
		sendAT("atv0\r");
		sendAT("at+ncloseall\r");
		valid = false;
	}
}
uint8_t WizClientAuto::connected()
{
	return valid;
}
WizClientAuto::operator bool()
{
	return valid;
}