//----------------------------------------------------------------------------
//                  Robokid
//----------------------------------------------------------------------------
// pc_link.c : code to run interaction to a USB connected PC
// =========
//
// Description
//
// Author                Date          Comment
//----------------------------------------------------------------------------
// Jim Herd            04/11/2013     Initial edit
//----------------------------------------------------------------------------

#include "global.h"

//----------------------------------------------------------------------------
// check_for_computer_link : check for computer connected to serial port
// =======================
//
// Description
//     If there is a suitable computer attached to the serial port it will require
//     to respond to a send sync character ('S').  After a short delay a check is 
//     made for a response character ('A'). Return appropriate YES or NO.
//
uint8_t  check_for_computer_link(void)
{
uint8_t  rec_char;
	
	send_msg("%\r\n");
	DelayMs(USB_PC_SYNC_DELAY);
	rec_char = SCI1D;        	// get received character
	if (rec_char == '&') {
		send_msg("OK\r\n");       // reply handshake
		return YES;
	}
	return NO;
}

//----------------------------------------------------------------------------
// run_PC_link : run with connection to PC
// ===========
//
// Description
//		Robokid aware PC is active on serial link.
//		Initially, only for the download of ubasic+ programs.
//       
void run_PC_link(void) {
	
	FOREVER {
		get_line(cmd_string);
    	if (check_line(cmd_string) == SYS_CMD) {
    		send_msg(cmd_string);
    		process_PC_cmd(cmd_string);
    		continue;
    	}
	}
}

//----------------------------------------------------------------------------
// check_line : Is line part of a ubasic+ program or a system command
// ==========
//
// Description
//		System commands have a '%' character in the first column
//
// Notes
//   
uint8_t check_line(char line[]) 
{
	if (line[0] == SYS_CMD_CHAR) {
		return SYS_CMD;
	}
	return UBASICP_CODE;
}

//----------------------------------------------------------------------------
// process_PC_cmd : Process a command that has come from the attached PC
// ==============
//
// Description
//		A downloaded ubasic+ program can contain some system commands
//
//		%Pn    	set store 'n' as FLASH destination for next program to be downloaded.
//				Value can be in range 0 to 3.  Default is 0.
//
// Notes
//   
uint8_t process_PC_cmd(char line[]) {
	
	switch (line[1]) {
	case LOAD_PROG_STORE:
		ubasicp_program_store = line[2] - '0';
		if (ubasicp_program_store > (NOS_STORED_PROGRAMS - 1)) { // default to 0 if error
			ubasicp_program_store = 0;
		}
		get_ubasicp_program();
		break;
	case READ_BUTTONS:
		break;
	case DISPLAY_LEDS:
		break;
	case DISPLAY_TEXT:
		break;
	case EXECUTE_PROG:
		break;
	case READ_SENSOR:
		break;
	default:
		break;
	}
	return 0;
}

//----------------------------------------------------------------------------
// get_ubasicp_program : read a program from the serial port
// ===================
//
// Description
//		Download a ubasic+ program as a stream of ASCII characters. Terminated by a NULL
//		character.
//
// Notes
//
//      Active switches are 
//          switch D = start program read process
//   
uint8_t get_ubasicp_program(void) {

uint8_t   string_ptr;
int8_t    line_type;

    clr_all_LEDs();
    set_LED(LED_D, FLASH_ON);
//
// switch D used as start command
//
    WAIT_SWITCH_PRESSED(switch_D);
    WAIT_SWITCH_RELEASED(switch_D);      
    set_LED(LED_D, FLASH_OFF);
    play_tune(&snd_beeps_1);
//
// initialise ubasic+ storage area by putting null terminator in location 0
// set pointer to start of buffer and set time-out counter
// Note : 'string_ptr' will point to next free space in 512 character buffer
//    
    shared.ubasicp_program_space[0] = '\0';
    string_ptr = 0;
    CLR_TIMER16;                        // clear 16-bit 8mS tick counter
//
//  Read lines of characters from the serial port
//
    FOREVER {
    	get_line(cmd_string);
    	if (check_line(cmd_string) == SYS_CMD) {
    		send_msg(cmd_string);
    		process_PC_cmd(cmd_string);
    		continue;
    	}
    	line_type = trim_line(cmd_string);		    // remove padding
    	send_msg(cmd_string);                       // send squeezed string back to sender
    	if (cmd_string[0] == UBASIC_TERMINATOR) {	// @ = end of program transfer
    		break;
    	}
    	if (line_type == LINE_BLANK) {              // don't store blank lines
    		continue;
    	}
    	if (line_type == LINE_NO_TERM) {		    // problem
    		break;
    	}
    	string_ptr = store_line(string_ptr, cmd_string);
    }
//
// ensure that ubasicp program has a null terminator
//
    shared.ubasicp_program_space[string_ptr] = '\0';
//
// Store buffer in FLASH (dumps entire buffer)
//
	save_ubasicp_program(ubasicp_program_store);
	
    return 0;
}









