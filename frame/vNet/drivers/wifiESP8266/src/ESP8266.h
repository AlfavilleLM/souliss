/**************************************************************************
	Souliss - vNet Virtualized Network
    Copyright (C) 2015  Veseo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
	Originally developed by Steven Sokol
	Included in vNet by		Dario Di Maio
***************************************************************************/
/*!
    \file 
    \ingroup
*/
/**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <Arduino.h>

#ifndef _ESP8266_H
#define _ESP8266_H

/**************************************************************************/
/*!
	Set the default baudrate used to communicate with the ESP8266 module
*/
/**************************************************************************/
#ifndef	ESP8266_BAUDRATE_INSKETCH
#	define	ESP8266_BAUDRATE		9600
#endif
/**************************************************************************/
/*!
	If enabled set a time-out for the USART
	
        Value       Status
        0x0         Disable 
        0x1         Enable	(Default)
*/
/**************************************************************************/	
#define	ESP8266_ENTIMEOUT			1
#define	ESP8266_TIMEOUT				5000

/**************************************************************************/
/*!
	If enabled print the header and payload of incoming, outgoing and routed
	frames.
	
        Value       Status
        0x0         Disable (Default)
        0x1         Enable
		
	This enable the Arduino AltSoftSerial library and need an external USB
	to USART converter (like the one used for programming Arduino Ethernet).
	
	In your sketch you should add the following lines
		[ before setup() ] 	AltSoftSerial myUSARTDRIVER(5, 6);	
		
		[in setup()	]		myUSARTDRIVER.begin(9600);

	The TX and RX pins from the external converter shall be reversed, so TX 
	goes on pin 5 (RX from SoftwareSerial) and RX to 6.
*/
/**************************************************************************/
#define ESP8266_DEBUG  				0

enum wifiModes {
  WIFI_MODE_STA = 1,
  WIFI_MODE_AP,
  WIFI_MODE_APSTA
};

enum wifiErrors {
  WIFI_ERR_NONE = 0,
  WIFI_ERR_AT,
  WIFI_ERR_RESET,
  WIFI_ERR_CONNECT,
  WIFI_ERR_LINK  
};

class ESP8266
{
  public:
    // constructor - set link mode and server port
    ESP8266(uint16_t mode = 1, long baudrate = ESP8266_BAUDRATE, uint16_t debugLevel = 0);
    
    // init / connect / disconnect access point
    uint16_t	initializeWifi();
    uint16_t	connectWifi();
    bool		disconnectWifi();
    void		resetWifi();
	void		setBuffer(uint8_t *pointer, uint8_t len);
	
    // server
    bool		startServer(uint16_t port = 8000, long timeout = 300);
    
    // client
    bool		startClient(char *ip, uint16_t port, long timeout = 300);
	bool		startBroadcast();
    
	// data and connection feedback
	uint8_t		dataAvailable(); 			
	bool 		connectionAvailable();
	uint16_t	dataRetrieve(char* data_pnt);
	uint16_t 	releaseData();
	
    // send data across the link
    bool 		send(char *data);
	uint16_t	send_oFrame(oFrame* frame);
    
    // process wifi messages
    void 		run();

    // informational
    char 		*ip();
    uint16_t 	scan(char *out, uint16_t max);
	
	
  private:
    void clearResults();
    bool sendData(uint16_t chan, char *data);
    bool setLinkMode(uint16_t mode);
    bool startUDPChannel(uint16_t chan, char *address, uint16_t port);
    void processWifiMessage(uint8_t available_bytes);
    bool getIP();
    bool getBroadcast();
    void debug(char *msg);
    bool searchResults(char *target, long timeout, uint16_t dbg = 0);
};

#endif


