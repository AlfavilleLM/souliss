/**************************************************************************
	Souliss - vNet Virtualized Network
    Copyright (C) 2013  Veseo

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
	
	Originally developed by Dario Di Maio
	
***************************************************************************/
/*!
    \file 
    \ingroup

*/
/**************************************************************************/
#include <stdio.h>
#include <string.h>

#include "vNetDriver_esp8266.h"							

// Global variables used in the driver
uint8_t usartframe[USART_FRAME_LEN], l=0;	
uint8_t	busstate=USART_BUSBUSY;
uint16_t myaddress=0;

// Define the wifi module constructor
ESP8266 esp8266(1,115200,0);

// The name of the class that refers to the USART, change it accordingly to the used device
#ifndef USARTDRIVER_INSKETCH
#	define	USARTDRIVER	Serial	

#	if (USART_DEBUG)
#		include "SoftwareSerial.h"
		extern SoftwareSerial myUSARTDRIVER; // RX, TX	
#		define USART_LOG 	myUSARTDRIVER.print
#	endif			
#endif

/**************************************************************************/
/*!
    Init the ESP8266 Module
*/
/**************************************************************************/
void vNet_Init_M3()
{	
	esp8266.setBuffer(usartframe);
	esp8266.initializeWifi();
	esp8266.connectWifi();
	esp8266.startBroadcast();
}

/**************************************************************************/
/*!
	Set the vNet address 
	
	The USART works in broadcast and has no address check, this method is
	here only for compatiblity with upper vNet layers (that has their own
	addressing).
	
	The address is saved in a local register and is used in the collision
	avoidance
*/
/**************************************************************************/
void vNet_SetAddress_M3(uint16_t addr)
{
	myaddress = addr;
}

/**************************************************************************/
/*!
	Send a message via USART
*/
/**************************************************************************/
uint8_t vNet_Send_M3(uint16_t addr, oFrame *frame, uint8_t len)
{

}

/**************************************************************************/
/*!
	Read the incoming data size, use this after vNet_DataAvailable_M3()
*/
/**************************************************************************/
uint8_t vNet_DataSize_M3()
{
	return l;
}

/**************************************************************************/
/*!
	If data are available store in the temporary buffer
*/
/**************************************************************************/
uint8_t vNet_DataAvailable_M3()
{
	uint8_t i=0, ll=0;

	if(l == USART_FRAME_LEN)
	{
		#if(USART_DEBUG)	
		USART_LOG("(USART)<Read> Frame reset\r\n");
		#endif	
			
		// Reset data, this avoid fake reads due to old data
		for(i=0;i<USART_FRAME_LEN;i++)
			usartframe[i]=0;
	
		l=0;		// Buffer is full just before retrieve data, remove junk
	}
	
	// Record the number of bytes that are actually in the frame
	ll = l;
	
	// From the USART we get one byte per time, so before has to be identified
	// if there is an incoming frame	
	while(USARTDRIVER.available() && (l < USART_FRAME_LEN))	
		usartframe[l++] = USARTDRIVER.read();	
	
	// If there are no incoming data
	if((l == 0) || (l == ll))
	{
		BusFreeSlot();			// Note that there was a free slot
		return USART_FAIL;
	}

	#if(USART_DEBUG)	
	USART_LOG("(USART)<Read> Data aval\r\n");
	#endif	
	
	// There were bytes on the bus
	BusIsBusy();
	
	// If there are few bytes, the frame is still incomplete
	if(l < (USART_PREAMBLE_LEN+USART_POSTAMBLE_LEN+USART_HEADERLEN+USART_CRCLEN))
		return USART_FAIL;	// Nothing to parse

	#if(USART_DEBUG)	
	USART_LOG("(USART)<Read> Length ok\r\n");
	#endif	
			
	// If the length exceed the buffer size
	if(l > USART_FRAME_LEN)
	{
		#if(USART_DEBUG)	
		USART_LOG("(USART)<Read> Clear junk\r\n");
		#endif	
		
		// Reset data, this avoid fake reads due to old data
		for(i=0;i<USART_FRAME_LEN;i++)
			usartframe[i]=0;
			
		l=0;		// Buffer is full just before retrieve data, remove junk
		
		return USART_FAIL;
	}
	
	// Analyze the retreived frame to findout a vNet message
	for(i=0;i<l-USART_PREAMBLE_LEN;i++)
	{
		// Look for vNet preamble used in vNet for Ethernet Raw mode 
		if( (usartframe[i]   == USART_PREAMBLE) &&
			(usartframe[i+1] == USART_PREAMBLE) &&
			(usartframe[i+2] == USART_PREAMBLE) &&
			(usartframe[i+3] == USART_PREAMBLE) &&
			(usartframe[i+4] == USART_PREAMBLE) &&				
			(usartframe[i+5] == USART_PREAMBLE))
		{
			// There is a preamble, look for postamble
			#if(USART_DEBUG)	
			USART_LOG("(USART)<Read> Preamble ok\r\n");
			USART_LOG("(USART)<Read> Index 0x");
			USART_LOG(i,HEX);
			USART_LOG("\r\n");
			#endif	
							
			// If is a valid vNet message, after the preamble there is the frame length
			uint8_t vNetLen = usartframe[i+USART_PREAMBLE_LEN];		
			
			// Check the complete frame			
			if((vNetLen)&& ((i+USART_PREAMBLE_LEN+vNetLen) < USART_FRAME_LEN) &&
				(usartframe[i+USART_PREAMBLE_LEN+vNetLen] == USART_POSTAMBLE) &&
				(usartframe[i+USART_PREAMBLE_LEN+vNetLen+1] == USART_POSTAMBLE) &&
				(usartframe[i+USART_PREAMBLE_LEN+vNetLen+2] == USART_POSTAMBLE) &&
				(usartframe[i+USART_PREAMBLE_LEN+vNetLen+3] == USART_POSTAMBLE) &&
				(usartframe[i+USART_PREAMBLE_LEN+vNetLen+4] == USART_POSTAMBLE) &&
				(usartframe[i+USART_PREAMBLE_LEN+vNetLen+5] == USART_POSTAMBLE))
			{

				#if(USART_DEBUG)	
				USART_LOG("(USART)<Read> Postamble ok\r\n");
				#endif	
				
				// Save the transmitter crc
				in_crc = *(uint16_t*)(usartframe+i+USART_PREAMBLE_LEN+vNetLen-USART_CRCLEN);	
				
				// The frame is a valid vNet message, remove the preamble
				memmove(usartframe, &usartframe[i+USART_PREAMBLE_LEN], vNetLen);		
				l=l-i;
				
				return vNetLen;			// Return message length
			}
			else if((l-i)<(vNetLen+USART_PREAMBLE_LEN+USART_POSTAMBLE_LEN+USART_HEADERLEN+USART_CRCLEN))
			{
				// If we are here, the frame is incomplete just wait for next data
				BusIsRecev();			// We are receiving data on the bus cannot send	

				#if(USART_DEBUG)	
				USART_LOG("(USART)<Read> Incomplete\r\n");
				#endif	

				// If the frame doesn't start at zero, free up the space
				if(i)
				{
					// Clean up the buffer from not used data
					memmove(usartframe, &usartframe[i], USART_FRAME_LEN-i);	
					l=l-i;
				
					// If we are here, the frame is incomplete just wait for next data
					BusIsRecev();			// We are receiving data on the bus cannot send	

					#if(USART_DEBUG)	
					USART_LOG("(USART)<Read> Defragment\r\n");
					#endif						
				}
				
				return USART_FAIL;	
			}
			else
			{
				// If we are here, the frame was corrupted
				BusIsBusy();	// Bus busy

				#if(USART_DEBUG)	
				USART_LOG("(USART)<Read> Corrupted\r\n");
				#endif				
				
				// Reset data, this avoid fake reads due to old data
				for(i=0;i<USART_FRAME_LEN;i++)
					usartframe[i]=0;
					
				l=0;		// Buffer is full just before retrieve data, remove junk
		
				return USART_FAIL;		
			}
		}
	}
	
	// Next try
	#if(USART_DEBUG)	
	USART_LOG("(USART)<Read> Next try\r\n");
	#endif		
	
	return USART_FAIL;		
}

/**************************************************************************/
/*!
	Retrieve the complete vNet frame from the incoming buffer
*/
/**************************************************************************/
uint8_t vNet_RetrieveData_M3(uint8_t *data)
{
	uint8_t len=*(usartframe)-USART_HEADERLEN-USART_CRCLEN;			// Retrieve the first byte of the message
	uint16_t c_crc;													// Calculate the CRC from the frame

	// Retrieve the complete message
	if(len > 0 && len <= l)
	{
		#if(USART_DEBUG)	
		USART_LOG("(USART)<Read> Retrieve data\r\n");
		#endif	
		
		uint8_t k=len;							// The CRC isn't calculated for the USART_HEADER
		
		#if(USART_CRCCHECK)	
		c_crc = 0xFFFF;
		for(uint8_t k=0;k<len;k++)
		{
			c_crc = c_crc ^ (*(usartframe+k+1));
			for (uint8_t j=0; j < 8; j++) 
			{
				if (c_crc & 0x0001)
					c_crc = (c_crc >> 1) ^ 0xA001;
				else
					c_crc = c_crc >> 1;
			}
		}		
			
		// Finalize the CRC
		c_crc = (c_crc << 8  | c_crc >> 8);
			
		// Frame is corrupted
		if(in_crc != c_crc)
		{
			#if(USART_DEBUG)	
			USART_LOG("(USART)<Read> CRC Failed\r\n");
			#endif	
					
			return USART_FAIL;
		}	
		#endif
		
		// The bus is a broadcast media and every time that we get a byte we consider
		// it as busy, if the frame is for us, we can consider it free because is our
		// time to give an answer.
		if((*(U16*)(usartframe+3))==myaddress)
		{
			setBusFree();
			
			#if(USART_DEBUG)	
			USART_LOG("(USART) Set bus free\r\n");
			#endif	
		}
		else if(((*(U16*)(usartframe+3)) > VNET_ADDR_NULL) && ((*(U16*)(usartframe+3)) <= VNET_ADDR_BRDC)) 
		{
			// If is a broadcast or unicast frame we are supposed to give an answer, but
			// all nodes will probably do the same. In order to avoid collision, we wait a bit
			// before processing the data
			waitSend();
			
			// Now we set bus as free and proceed processing data
			setBusFree();
			
			#if(USART_DEBUG)	
			USART_LOG("(USART) Set bus delay\r\n");
			#endif	
		}
			
		// Send data to the top layer
		memcpy(data, usartframe+1, len);

		// The vNet USART driver support small payloads, cut of payload can happen before sending
		// at this stage we verify the original length and fill the missing with zeros.
		if(*(usartframe+1) > len)
			for(uint8_t i=0; i<(*(usartframe+1)-len); i++)
				*(data+len+i) = 0;
			
		// Move forward not parsed data
		memmove(usartframe, usartframe+len, USART_FRAME_LEN-len);
		if(l>(USART_FRAME_LEN-len))
			l-=USART_FRAME_LEN-len;				// Reset the length
		else
			l = 0;
	}
	else
	{

		#if(USART_DEBUG)	
		USART_LOG("(USART)<Read> Retrieve length failed\r\n");
		#endif	
			
		l = 0;										// Reset the length
		return ETH_FAIL;							// Data corrupted
	}
		
	// Return length of the data
	return len;
}

/**************************************************************************/
/*!
    Get the source address of the most recently received frame
*/
/**************************************************************************/
uint16_t vNet_GetSourceAddress_M3()
{
	return 0;
}