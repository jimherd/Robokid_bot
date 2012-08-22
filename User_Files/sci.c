//----------------------------------------------------------------------------
//                  Robokid
//----------------------------------------------------------------------------
//  init.c : initialise the chip subsystems
//  ======
//
// Common initialization of the write once registers
//
// Author                Date          Comment
//----------------------------------------------------------------------------
// Jim Herd            8/08/2008    
//----------------------------------------------------------------------------
 
#include "global.h"

//***********************************************************************
//** Function:      sci_tx_byte
//** Description:   SCI Tx function
//** Parameters:    None
//** Returns:       None
//*********************************************************************** 
void sci_tx_byte(char s_char){
char dummy;

    SCI1C2_TE = 1;          	// enable Tx
    dummy = SCI1S1;          	// 1st half of TDRE clear procedure
    SCI1D = s_char;             // 2nd half of TDRE clear procedure
    while(!SCI1S1_TDRE)
        ;			            // Wait until send buffer empty
    while(!SCI1S1_TC)
        ;       	            // Wait until byte sent
    SCI1C2_TE = 0;
}

//***********************************************************************
//** Function:      sci_rx_byte
//** Description:   SCI Rx function
//** Parameters:    None
//** Returns:       None
//***********************************************************************
char sci_rx_byte()
{
char rec_char;

    SCI1C2_RE = 1;           	// enable Rx
    while(!SCI1S1_RDRF)         
        ;                       // wait for character
    rec_char = SCI1D;        	// get received character
    SCI1C2_RE = 0;              // disable Rx
    return rec_char;			 			
}

//***********************************************************************
//** Function:      send_msg
//** Description:   SCI String Sender
//** Parameters:    None
//** Returns:       None
//*********************************************************************** 
void send_msg(char *msg){
byte n=0;	   							// String pointer
  
    do{
        sci_tx_byte(msg[n]);
  	    n++;
    }while(msg[n]);
}

//***********************************************************************
//** Function:      HexToAsc
//** Description:   Converts Byte to ASCII string
//** Parameters:    None
//** Returns:       None
//*********************************************************************** 
void * HexToAsc(char byte, char number_str[]){
char temp;

    temp = byte & 0xF0;
    temp = temp >> 4;
    if (temp <= 9) {
        number_str[0]=(temp + '0');
    } else {
        number_str[0]=(temp + 'A' - 10); 
    }
    temp = byte & 0x0F;
    if (temp <= 9) {
        number_str[1]=(temp + '0');
    } else {
        number_str[1]=(temp + 'A' - 10);
    }
    number_str[2]=0x00;
    return number_str;      
}

void * HexToBin(char byte, char number_str[]){
  char n;
  for (n=0;n<8;n++){
    if (byte & 0b10000000) number_str[n]='1';
    else number_str[n]='0';
    byte = byte <<1;
  }
  number_str[n]=0x00;
  return number_str;
}

//***********************************************************************
//** Function:      bcd
//** Description:   Bin2Dec conversion function
//** Parameters:    None
//** Returns:       None
//***********************************************************************
void * bcd(char byte, char number_str[])
{
	number_str[0] = ((byte - (byte % 100)) / 100) & 0xFF;
	byte -= number_str[0]*100;

	number_str[1] = ((byte - (byte % 10)) / 10) & 0xFF;
	byte -= number_str[1]*10;
	
	number_str[2] = byte;

	number_str[0] += '0';
	number_str[1] += '0';
	number_str[2] += '0';
	number_str[3] = 0x00;
	return number_str;
}

//----------------------------------------------------------------------------
// read_line : read an ASCII string from the serial input via the USB channel
// =========
//
// Description
//		Read an ASCII string via the USB virtual COM port.  String will be terminated
//		with a newline character and be less that MAX_LINE_SIZE characters in length.
//		Error check to detect lines that would overflow the buffer.
//
// Notes
//
uint8_t read_line(char string[])
{
	uint8_t   i;
	
	for(i=0 ; i < TEMP_STRING_SIZE ; i++){
		if ((string[i] = sci_rx_byte()) == '\n') {
			string[i+1] = '\0';
			return i;
		}
	}
//
// To here if line is too long. Insert NULL at end of buffer and return maximum size
// 
	string[TEMP_STRING_SIZE-1] = '\0';
	return TEMP_STRING_SIZE;
}


    
