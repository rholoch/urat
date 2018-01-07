/******************************************************************************************************************
 * urat
 *
 * kusb for the KPOD - (C) 2016 Elecraft Inc - Paul Saffren n6hz - starter code
 * July 12, 2017 - R. Holoch (KY6R) took starter kusb code and morphed it into urat
 *
 * example: urat
 *
 * This program is designed to be compiled under a Linux host system. It also requires wirinPi and oled96 libraries
 *
 * First build the Elecraft kusb, then wiringPi nd oled96 - some have a build file, some a make file. 
 * To compile:
 * gcc urat.c -lwiringPi -loled96 -o urat 
 *
 * ----------------------------------------------------------------------------------------------------------------
 * Operation:
 * 
 * Once the device is found using the KPOD's VID and PID, USB packets of 8 bytes (EP0) are sent and received over 
 * the generic hid (raw) device.  
 * -----------------------------------------------------------------------------------------------------------------
 *
 * Uses HIDRAW USB device
 *
 * This device is a low level USB device that does not include any of the report parsing that other devices use.
 * That means that no driver is required (like a serial port, etc).  The device must be permissioned to be
 * opened by user space.  This is done by inserting a file with the contents of:
 *
 * KERNEL=="hidraw*", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="f15d", MODE="0666"
 *
 * The file goes into: /etc/udev/rules.d
 *
 *
 * 		
 *****************************************************************************************************************/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <linux/types.h>
#include <linux/hidraw.h>
#include <wiringPi.h>
#include "oled96.h"


const int NUM_STEPS = 48;		// number of stepper steps including half steps
int currentStep = 1;			// just assume this starting point


/****************************************************************************
 * Macros
 ***************************************************************************/
#define versionString 		"v1.00"

#define REPORT_LEN			8				// EP0 report packet size
#define VENDOR_ID		0x04d8				// Microchip vid
#define PRODUCT_ID		0xF12D				// Elecraft KPOD product id



#ifndef BYTE
	typedef unsigned char BYTE;
#endif



/******************************************************************************
 * KPOD USB Commands
 *****************************************************************************/
typedef enum 
{
    KPOD_USB_CMD_UPDATE =   'u',            // default packet, return info
    KPOD_USB_CMD_ID     =   '=',            // request status
    KPOD_USB_CMD_RESET  =   'r',            // reset device
    KPOD_USB_CMD_VER    =   'v',            // get version
    KPOD_USB_CMD_OUTS   =   'O',            // LED and AUX out control, first byte of data is RXCMD type data
    KPOD_USB_CMD_BEEP   =   'Z',            // beeper on/off with freq
             
}KPOD_USB_CMD;




/******************************************************************************
 * KPOD command and report packet structures
 * 8 bytes are used to send and receive data to the KPOD USB device stack
 *****************************************************************************/


typedef struct USB_ReportPkt
{
    BYTE		cmd;    				// command reply
    int16_t   	ticks;                  // encoder tick count, signed
    BYTE  	button;                 // button code includes hold or tap bit
    BYTE 	spare[4];        
}__attribute__((packed)) USB_ReportPkt, *USB_RepPktPtr;


typedef struct USB_CmdPacket
{
    BYTE    cmd;                    // command
    BYTE    data[7];                // data
  
}__attribute__((packed)) USB_CmdPacket, *USB_CmdPktPtr;



/****************************************************************************
 * Memory
 ***************************************************************************/



// Devices
int TargetDevice = -1;			// hidraw device


/****************************************************************************
 * Prototypes
 ***************************************************************************/
int findHidRawDevice(void);
void closeAll(void);
int sendCommand(int command, USB_RepPktPtr reptr);



/****************************************************************************
 * findHidRawDevice()
 *
 * Search through the path in
 *
 * Pass: nothing
 * Returns: 0=success
 ***************************************************************************/
int findHidRawDevice(void)
{
	struct hidraw_devinfo info;
	int i;
	char devstr[20];

	for (i=0;i<255;i++)					// should be plenty for hidraw devices
	{
		sprintf(devstr,"/dev/hidraw%d\0",i);	// build device name
		TargetDevice = open(devstr,O_RDWR);
		if (TargetDevice < 0)
			continue;


		memset((void *)&info,0x00,sizeof(info));
		ioctl(TargetDevice, HIDIOCGRAWINFO, &info);
		printf("vid:%x pid:%x\n",(info.vendor & 0xFFFF), (info.product & 0xFFFF));

		if (((info.vendor & 0xFFFF) == VENDOR_ID) && ((info.product & 0xFFFF) == PRODUCT_ID))		// device is Elecraft KPOD?
		{
			printf("found kpod at %s\n",devstr);
			return (0);
		}

		else
			close(TargetDevice);
	}
	return (1);
}


/*****************************************************************************
* closeAll()
*
* Close all comports and file handles
*
* Pass: nothing
* Returns: nothing
*****************************************************************************/
void closeAll(void)
{
    oledShutdown();        

	if (TargetDevice >= 0)
		close(TargetDevice);

}	// closeAll


/****************************************************************************
 * sendCommand()
 *
 * Sends the command and returns a copy of the reply using the passed
 * packetptr.  If replyptr is null, no read is performed and the command
 * is simply sent blind.
 *
 * Pass: command, packetptr
 * Returns: status = 0 success
 ***************************************************************************/
int sendCommand(int command, USB_RepPktPtr reptr)
{
	USB_CmdPacket  pkt;


	if (TargetDevice < 0)
		return (1);

	pkt.cmd = command;

	if (write(TargetDevice, (BYTE *)&pkt, REPORT_LEN) != REPORT_LEN)				// write to target
	{
		printf("error writing packet\n");
		return (1);
	}

	if (reptr != NULL)
	{
		if (read(TargetDevice, (BYTE *)reptr, REPORT_LEN) != REPORT_LEN)					// then read reply
		{
			printf("error reading packet\n");
			return (1);
		}
	}


	return (0);

}	// sendCommand


/******************************************************************************
* getUpdate()
*
* Get a regular update of switches, encoder, etc and display values.
*
* Pass: nothing
* Returns: nothing
*****************************************************************************/
void getUpdate(void)
{
        int res;
        int16_t lctr;
        int16_t cctr;
        char szTemp[64];
        USB_ReportPkt replypkt;

        res = sendCommand(KPOD_USB_CMD_UPDATE, &replypkt);
        if (res != 0)
        {
                printf("usb command failure");
                return;
        }
        else
        {
                if (replypkt.cmd == KPOD_USB_CMD_UPDATE)                        // only display data on valid data
                {
//                        printf("enc:%d  button:0x%02X  rocker:0x%02X\n", replypkt.ticks, (replypkt.button & 0x1F), (replypkt.button & 0xE0) >> 5);
                        if (replypkt.button & 0xE0)
                        {
                        // Get Inductor ticks when rocker switch is on either outside position
                        lctr=lctr+replypkt.ticks;
                        printf ("L:%d\n",lctr);
                        sprintf (szTemp,"L:%d",lctr);
                        oledWriteString(0,0,szTemp,1);
                                if (replypkt.ticks < 0)
                                    {
                                    digitalWrite (1, 0) ;         // direction - Clockwise
                                    digitalWrite (4, 1) ;         // step - in increments set by the drivers dip switches
                                    delayMicroseconds (10) ;
                                    digitalWrite (4, 0) ;
                                    delayMicroseconds (10) ;
                                    }
                                if (replypkt.ticks > 0)          // direction - counter Clockwise
                                    {
                                    digitalWrite (1, 1) ;
                                    digitalWrite (4, 1) ;
                                    delayMicroseconds (10) ;
                                    digitalWrite (4, 0) ;
                                    delayMicroseconds (10) ;
                                    } 

                        }
                        else
                        {
                        // Get capacitor ticks when rocker switch is set to center position
                        cctr=cctr+replypkt.ticks;
                        printf ("C:%d\n",cctr);
                        sprintf (szTemp,"C:%d",cctr);
                        oledWriteString(0,0,szTemp,1);
                                if (replypkt.ticks < 0) 
                                    {
                                    digitalWrite (22, 0) ;         // direction - Clockwise
                                    digitalWrite (23, 1) ;         // step - in increments set by the drivers dip switches
                                    delayMicroseconds (10) ;
                                    digitalWrite (23, 0) ;
                                    delayMicroseconds (10) ;
                                    }
                                if (replypkt.ticks > 0)          // direction - counter Clockwise
                                    {
                                    digitalWrite (22, 1) ;
                                    digitalWrite (23, 1) ;
                                    delayMicroseconds (10) ;
                                    digitalWrite (23, 0) ;
                                    delayMicroseconds (10) ;
                                    } 


                        }
                }
        }



}       // showInfo

/******************************************************************************
 * main()
 *
 * Main entry
 *
 *
 *
 *****************************************************************************/
int main(int argc, char *argv[])
{
	USB_ReportPkt replypkt;
        int x;


        // initialize WiringPi library
	x = wiringPiSetup ();
	if (x == -1)
	{
		printf("Error on wiringPiSetup.  Program quitting.\n");
		return 0;
	}
	// set the first three GPIO pins to output and set to 0
                pinMode(1, OUTPUT);
                digitalWrite(1, 0);
                pinMode(4, OUTPUT);
                digitalWrite(4, 0);
                pinMode(22, OUTPUT);
                digitalWrite(22, 0);
                pinMode(23, OUTPUT);
                digitalWrite(23, 0);


	if (findHidRawDevice())
	{
		printf("could not find KPOD\n");
		closeAll();
		return (1);
	}
        // Set up OLED 
        int y;
        y=oledInit(0x3c);
        {
        oledFill(0); // Fill with black
        oledWriteString(0,0,"KY6R LAB",1);
        usleep(1000);
        oledFill(0); // Fill with black
        }

	
	sendCommand(KPOD_USB_CMD_ID, &replypkt);
	printf("status reply: %s\n\r",(char *)&replypkt);

        while (1==1) {
            getUpdate();
            } 

	printf("done\n\r");
	closeAll();
	return (0);

}	// main











// ----------------------------------------------------------------------------------------------------------------





/************************************************ EOF URAT.C ****************************************************/




