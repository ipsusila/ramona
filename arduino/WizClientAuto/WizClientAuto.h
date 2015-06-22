#ifndef WizClientAuto_h
#define WizClientAuto_h

#include <Client.h>
#include <IPAddress.h>

class WizClientAuto : public Client {
public:
	WizClientAuto();
	void setup(uint32_t baud, const char *ssid, const char *key);
	int connect(IPAddress ip, uint16_t port);
	int connect(const char *host, uint16_t port);
	size_t write(uint8_t);
	size_t write(const uint8_t *buf, size_t size);
	int available();
	int read();
	int read(uint8_t *buf, size_t size);
	int peek();
	void flush();
	void stop();
	uint8_t connected();
	operator bool();
protected:
	bool valid;
};

#endif