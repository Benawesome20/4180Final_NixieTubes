/**********************************************************************
* 
* ECE 4180 Final Project - Nixie Tube Clock with DS3231 and Bluetooth Control
* Edited from the original demo by Benjamin Ventimiglia
*
***********************************************************************
* Copyright (C) 2015 Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
**********************************************************************/

#include "mbed.h"
#include "stdio.h"
#include "ds3231.h"

#define ESC 0x1B

#define BLUETOOTH


Ds3231 rtc(p9, p10); //rtc object
Serial bluemod(p28,p27); // Bluetooth device
BusOut Anodes(p11, p12, p13, p14);
BusOut Digit(p24, p23, p22, p21);

// RTC variables
//default, use bit masks in ds3231.h for desired operation
ds3231_cntl_stat_t rtc_control_status = {0,0}; 
ds3231_time_t rtc_time;

time_t epoch_time;
Ticker timer;
bool debug = 0;

// User input functions
void get_user_input(char* message, uint8_t min, uint8_t max, uint32_t* member);
void get_user_input(char* message, uint8_t min, uint8_t max, bool* member);
void get_bt_user_input(char* message, uint8_t min, uint8_t max, uint32_t* member);
void get_bt_user_input(char* message, uint8_t min, uint8_t max, bool* member);

// internal functions
int fix_digit(int num);
void show_time(int hour, int min);
void update_epoch() {   epoch_time = rtc.get_epoch();   }

int main(void)
{   
    rtc.set_cntl_stat_reg(rtc_control_status);
    
#ifndef BLUETOOTH
    // Ask for debug mode from user
    get_user_input("\nTurn on debug? 0 for no, 1 for yes: ", 0, 1, &debug);
    
    //Get hour from user
    get_user_input("\nPlease enter the hour (0-23): ", 0, 23, &rtc_time.hours);
                       
    //Get minutes from user
    get_user_input("\nPlease enter the minute (0-59): ", 0, 59, &rtc_time.minutes);
    
    //Get seconds from user
    get_user_input("\nPlease enter the second (0-59): ", 0, 59, &rtc_time.seconds);

    // If BLUETOOTH is defined, use bt inputs
#else
    // Ask for debug mode from user
    get_bt_user_input("\nTurn on debug? 0 for no, 1 for yes: ", 0, 1, &debug);
    
    //Get hour from user
    get_bt_user_input("\nPlease enter the hour (0-23): ", 0, 23, &rtc_time.hours);
     
    //Get minutes from user
    get_bt_user_input("\nPlease enter the minute (0-59): ", 0, 59, &rtc_time.minutes);
    
    //Get seconds from user
    get_bt_user_input("\nPlease enter the second (0-59): ", 0, 59, &rtc_time.seconds);
#endif
    
    //Set the time, uses inverted logic for return value
    if(rtc.set_time(rtc_time))
    {
        printf("\nrtc.set_time failed!!\n");
        bluemod.printf("\nrtc.set_time failed!!\n");
        exit(0);
    }
    
    // Get time from RTC every second (to reduce load on mbed)
    timer.attach(&update_epoch, 1);
    tm calendar_time;
    
    for(;;)
    {   
        // convert epoch_time into hours and minutes
        calendar_time = *localtime(&epoch_time);
        int hours = int(calendar_time.tm_hour);
        int minutes = int(calendar_time.tm_min);
        
        // if debug mode, print out full time to pc and bluetooth every second
        if (debug) {
            printf("\n%s", ctime(&epoch_time));
            bluemod.printf("\n%s", ctime(&epoch_time));
            wait(1.0);
        }
        
        // Send time to nixie tubes in 24hr format
        show_time(hours, minutes);
    }//loop 
}


/**********************************************************************
* Function: get_user_input() / get_bt_user_input()
* Parameters: message - user prompt
*             min - minimum value of input
*             max - maximum value of input
*             member - pointer to struct member              
* Returns: none
*
* Description: get time/date input from user from either 
*              serial terminal or bluetooth connection
*
**********************************************************************/
void get_user_input(char* message, uint8_t min, uint8_t max, uint32_t* member)
{
    uint32_t temp;

    do
    {
        printf("\n%s", message);
        
        //for some reason mbed doesn't like a pointer to a member in scanf
        //term.scanf("%d", member); works with gcc on RPi
        scanf("%d", &temp);
        
        *member = temp;
       
        if((*(member)< min) || (*(member) > max))
        {
            printf("\nERROR-RTI");
        }
    }
    while((*(member) < min) || (*(member) > max));
}


void get_user_input(char* message, uint8_t min, uint8_t max, bool* member)
{
    uint32_t temp;

    do
    {
        printf("\n%s", message);
        
        //for some reason mbed doesn't like a pointer to a member in scanf
        //term.scanf("%d", member); works with gcc on RPi
        scanf("%d", &temp);
        
        *member = temp;
       
        if((*(member)< min) || (*(member) > max))
        {
            printf("\nERROR-RTI");
        }
    }
    while((*(member) < min) || (*(member) > max));
}

void get_bt_user_input(char* message, uint8_t min, uint8_t max, uint32_t* member)
{
    uint32_t temp;

    do
    {
        bluemod.printf("\n%s", message);
        
        //for some reason mbed doesn't like a pointer to a member in scanf
        //term.scanf("%d", member); works with gcc on RPi
        bluemod.scanf("%d", &temp);
        
        *member = temp;
       
        if((*(member)< min) || (*(member) > max))
        {
            bluemod.printf("\nERROR-RTI");
        }
    }
    while((*(member) < min) || (*(member) > max));
}

void get_bt_user_input(char* message, uint8_t min, uint8_t max, bool* member)
{
    uint32_t temp;

    do
    {
        bluemod.printf("\n%s", message);
        
        //for some reason mbed doesn't like a pointer to a member in scanf
        //term.scanf("%d", member); works with gcc on RPi
        bluemod.scanf("%d", &temp);
        
        *member = temp;
       
        if((*(member)< min) || (*(member) > max))
        {
            bluemod.printf("\nERROR-RTI");
        }
    }
    while((*(member) < min) || (*(member) > max));
}    

/**********************************************************************
* Function: fix_digit()
* Parameters: num - number to show on nixie tube            
* Returns: value to send to multiplexer
*
* Description: the wiring for the multiplexer is a bit off, so this
*              function makes the proper adjustment to set the multiplexer
*              bus
**********************************************************************/
int fix_digit(int num)
{
    switch(num){
        case 0:
            return 1;
        case 1:
            return 0;
        case 2:
            return 9;
        case 3:
            return 8;
        case 4:
            return 7;
        case 5:
            return 6;
        case 6:
            return 5;
        case 7:
            return 4;
        case 8:
            return 3;
        case 9:
            return 2;
    }
    return num;
}

/**********************************************************************
* Function: show_time()
* Parameters: hour - hour
*             min - minute            
* Returns: none
*
* Description: Displays the hour and minutes on the nixie tubes
**********************************************************************/
void show_time(int hour, int min)
{  
    if(hour > 9)
    {
        Digit = fix_digit(1);
        hour = hour - 10;
    }
    else
    {
        Digit = fix_digit(0);
    }
   
    wait_ms(2);
    Anodes = 1;
    wait_ms(1);
    Anodes = 0;
   
    Digit = fix_digit(hour);
           
    wait_ms(2);
    Anodes = 2;
    wait_ms(1);
    Anodes = 0;
   
    Digit = fix_digit(min / 10);
           
    wait_ms(2);
    Anodes = 4;
    wait_ms(1);
    Anodes = 0;
           
    Digit = fix_digit(min % 10);
           
    wait_ms(2);
    Anodes = 8;
    wait_ms(1);
    Anodes = 0;
   
    return;
}
