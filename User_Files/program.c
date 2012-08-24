//----------------------------------------------------------------------------
//                  Robokid
//----------------------------------------------------------------------------
// program.c : code to run programming modes
// =========
//
// Description
//
// Author                Date          Comment
//----------------------------------------------------------------------------
// Jim Herd          14/02/09      
//----------------------------------------------------------------------------

#include "global.h"

//----------------------------------------------------------------------------
// run_program_mode : run one of a set of programming activities
// ================
//
// Notes
//
//      Active switches are 
//          switch A = go/stop button
//          switch B = -----------
//          switch C = exit to main slection level
//          switch D = step through selections
//
uint8_t run_program_mode(void) {

uint8_t		activity;

    P_MODE_LEDS;
    activity = FIRST_PROGRAM_MODE;
    show_dual_chars('P', ('0'+ activity), (A_TO_FLASH | 10));
//
// main loop
//       
    FOREVER {
//
//  check and act on the switches
//
        if (switch_D == PRESSED) {
            WAIT_SWITCH_RELEASED(switch_D);
            activity++;
            SOUND_NEXT_SELECTION;
            if (activity > LAST_PROGRAM_MODE) {
                activity = FIRST_PROGRAM_MODE;
            }
            show_dual_chars('P', ('0'+ activity), (A_TO_FLASH | 10));
        }
        if (switch_C == PRESSED) {            //  back to main
            SOUND_EXIT_SELECTION; 
            WAIT_SWITCH_RELEASED(switch_C);
            return 0;
        }        
        if (switch_A == PRESSED) {              
            set_LED(LED_A, FLASH_ON); 
            set_LED(LED_D, FLASH_ON);
            WAIT_SWITCH_RELEASED(switch_A); 
        } else {
            continue;
        }
//
// run one of the programming activities
// 
        show_dual_chars('P', ('0'+ activity), 0);
        push_LED_display();       
        switch (activity) {
                case PROGRAM_MODE_0 :        // get and run sequence based on distances
                    run_program_mode_0();
                    break;
                case PROGRAM_MODE_1 :        // get and run sequence based black/white strip (motorised)
                    run_program_mode_1();
                    break;                    
                case PROGRAM_MODE_2 :        // get and run sequence based on time
                    run_program_mode_2();
                    break;
                case PROGRAM_MODE_3 :        // get and run sequence based black/white strip
                    run_program_mode_3();
                    break; 
                case PROGRAM_MODE_4 :        // get and run basic programs from the serial port
                    run_program_mode_4();
                    break;       
                default :
                    break;
        }
        show_dual_chars('P', ('0'+ activity), (A_TO_FLASH | 10));
        pop_LED_display();       
    } 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// run_program_mode_0 : input and run distance sequences
// ==================
//
// Description
//      Mode to allow a user to input a series of distance/turn commands
//		through the 4 buttons and subsequently cause them to be executed.
//
// Notes
//      Active switches are 
//          switch A = select current PLAY/COLLECT/SAVE/RECALL/DUMP command
//          switch B = --------
//          switch C = exit
//          switch D = Step through PLAY/COLLECT/SAVE/RECALL/DUMP command list
//
//      Active pots
//			Not active at the top level

uint8_t run_program_mode_0(void) {

mode_state_t  state;
sequence_mode_t   seq_mode;
uint8_t    i;

    state = MODE_INIT;
    seq_mode = FIRST_SEQUENCE_MODE;
    gLeft_Speed = SLOW_SPEED;
    gRight_Speed = gLeft_Speed - pwm_differential + DIFFERENTIAL_NULL;
    show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
    set_LED(LED_A, FLASH_ON);
    set_LED(LED_B, FLASH_ON);
    clr_LED(LED_C);
    set_LED(LED_D, FLASH_ON);
//
// initialise program sequence area
//
    for (i=0 ; i < RAM_SEQUENCE_SIZE ; i++) {
        shared.RAM_sequence.uint16[i] = 0xFFFF;
    }
//
// main loop
//       
    FOREVER {
// 
// No POT input at this time
//
        if (switch_B == PRESSED) {
            SOUND_READ_POTS;
            WAIT_SWITCH_RELEASED(switch_B);
        }
//
//  check for exit
//
        if (switch_C == PRESSED) {            //  exit mode
            vehicle_stop();
            SOUND_EXIT_SELECTION;
            WAIT_SWITCH_RELEASED(switch_C);
            return 0;
        }              
//
//  run simple state machine to define operating modes
//  There are two states : MODE_INIT and MODE_RUNNING
//
        if (state == MODE_INIT) {
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                SOUND_NEXT_SELECTION;
                seq_mode++;
                if (seq_mode > LAST_SEQUENCE_MODE) {
                    seq_mode = FIRST_SEQUENCE_MODE;
                }
                show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
            }         
            if (switch_A == PRESSED) {            //  go to RUN state              
                state = MODE_RUNNING;
                set_LED(LED_A, FLASH_ON); 
                set_LED(LED_D, FLASH_ON);
                WAIT_SWITCH_RELEASED(switch_A); 
            } else {
                continue;
            }
        }
        if (state == MODE_RUNNING) {
            if (switch_A == PRESSED) {            // exit   
                vehicle_stop();
                clr_LED(LED_A);
                clr_LED(LED_D);
                state = MODE_INIT; 
                WAIT_SWITCH_RELEASED(switch_A);                               
                return 0;
            }
        } 
//
// run one of PLAY/COLLECT/SAVE/RECALL/DUMP commands
// 
        show_dual_chars('S', ('0'+ seq_mode), 0);
        push_LED_display();       
        switch (seq_mode) {
            case PLAY :
                run_sequence(shared.RAM_sequence.uint16);
                break;            
            case COLLECT :
                input_distance_sequence();
                break;
            case SAVE :
                save_sequence(0);
                break;
            case RECALL :
                load_sequence(0);
                break;
            case DUMP :
                dump_sequence();
                break;                                                                  
            default :
                break;
        }
        state = MODE_INIT;
        seq_mode = FIRST_SEQUENCE_MODE;
        show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
        pop_LED_display();
    }                         // end of FOREVER loop
}

//----------------------------------------------------------------------------
// input_distance_sequence : input a distance based sequence from switches
// =======================
//
// Description
//      User can load a sequence of commands into the command buffer.  Commands
//      are input from the four push switches.  There are four possible commands
//
//      F (Forward), b (backward), L (spin Left), r (spin right)
//
//      The F and b commands have a single parameter that specifies the distance
//      to be moved in units of 1cm. Range is 0 to 99cm.
//      The L and r commands have a single parameter that is either 0, 45 or 90 
//      degrees.
//
// Notes
//
//      Active switches on starting 
//          switch A = accept command and goto parameter setting
//          switch B = --
//          switch C = exit
//          switch D = step through F/b/L/r commands
//
//      Active switches on parameter setting for F and b commands
//          switch A = accept command with current distance parameter
//          switch A = press for 3 seconds to abort this command setting
//          switch B = inc distance parameter by 10 cm        (F and b commands)
//          switch C = inc distance parameter by 1 cm         (F and b commands)
//          switch D = decrement distance parameter by 1 cm   (F and b commands)
//
//      Active switches on parameter setting for L and r commands
//          switch A = accept command with current turn angle parameter
//          switch A = press for 3 seconds to abort this command setting
//          switch B = inc turn parameter by 45 degrees       (L and r commands)
//          switch C = inc turn parameter by 45 degrees       (L and r commands)
//          switch D = decrement turn parameter by 45 degrees (L and r commands)
//
//      Active pots
//          None
//

uint8_t input_distance_sequence(void) 
{
uint8_t   activity, distance, seq_ptr;
uint16_t  ticks;
jmp_buf   env;
//
// initialise sequence array pointer
//
    seq_ptr = 0;
//        
// show "forward command"
//
    show_dual_chars('F', (' '), (A_TO_FLASH | B_TO_FLASH | 10));
    activity = FIRST_CMD;
    push_LED_display();
    set_LED(LED_A, FLASH_ON);
    clr_LED(LED_B);
    set_LED(LED_C, FLASH_ON);
    set_LED(LED_D, FLASH_ON);
  
    FOREVER {
        setjmp(env);            // jump point for abort current command input
//
//  check and act on the switches
//
        if (switch_D == PRESSED) {
            WAIT_SWITCH_RELEASED(switch_D);
            activity++;
            SOUND_NEXT_SELECTION;
            if (activity > LAST_CMD) {
                activity = FIRST_CMD;
            }
            switch (activity) {
                case CMD_FORWARD    : show_dual_chars('F', (' '), (B_TO_FLASH | 10)); break;
                case CMD_BACKWARD   : show_dual_chars('b', (' '), (B_TO_FLASH | 10)); break;
                case CMD_SPIN_LEFT  : show_dual_chars('L', (' '), (B_TO_FLASH | 10)); break;
                case CMD_SPIN_RIGHT : show_dual_chars('r', (' '), (B_TO_FLASH | 10)); break;
                default : break;
            }
        }
        
        if (switch_C == PRESSED) {            //  Load termination command
            store_instruction(shared.RAM_sequence.uint16, seq_ptr, EXECUTE, NO_MOD, STOP); seq_ptr++;
            store_instruction(shared.RAM_sequence.uint16, seq_ptr, EXIT,    NO_MOD, NO_DATA);  seq_ptr++;
            WAIT_SWITCH_RELEASED(switch_C);
            pop_LED_display();
            return 0;
        }
                
        if (switch_A == PRESSED) {         // accept command and move on to read distance parameter     
            push_LED_display();
            set_LED(LED_A, FLASH_ON);
            set_LED(LED_B, FLASH_ON);
            set_LED(LED_C, FLASH_ON);
            set_LED(LED_D, FLASH_ON);
            WAIT_SWITCH_RELEASED(switch_A); 
        } else {
            continue;
        }
//
// now find distance/angle parameter.  Set initial guess for distance/angle
// 
        distance = 0;
        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));     
//
// 
//        
        FOREVER {
//
// switch C increments the time parameter to a max value of 99
// switch C increments the angle parameter to a max value of 350 degrees
//        
            if (switch_C == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_C);
                switch (activity) {
                    case CMD_FORWARD    : 
                    case CMD_BACKWARD   : 
                        if (distance < 99) { distance++; }
                        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));
                        break;
                    case CMD_SPIN_LEFT  :                  
                    case CMD_SPIN_RIGHT : 
                        if (distance < 90) { distance += 45; }
                        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));
                        break;                                            
                    default : break;
                }
            }
//
// switch D decrements the time parameter to a min value of 0
//                                   
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                switch (activity) {
                    case CMD_FORWARD    : 
                    case CMD_BACKWARD   : 
                        if (distance >= 1) { distance--; }
                        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));
                        break;
                    case CMD_SPIN_LEFT  :                    
                    case CMD_SPIN_RIGHT : 
                        if (distance >= 45) { distance -= 45;  }
                        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));
                        break;                    
                    default : 
                        break;
                } 
            }      
//
// switch B to increment by 10cm (distance) or 45 degrees (angle)
//
            if (switch_B == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_B);
                switch (activity) {
                    case CMD_FORWARD    :
                    case CMD_BACKWARD   :                      
                        if (distance < 90) { distance += 10; }
                        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));
                        break;
                    case CMD_SPIN_LEFT  :                 
                    case CMD_SPIN_RIGHT :  
                        if (distance < 90) { distance += 45; }
                        display_number(distance, (A_TO_FLASH | B_TO_FLASH | 10));
                        break;                    
                    default : 
                        break;
                } 
            }
//
// switch A to accept an load command or press for > 3 seconds to abort
//
            if (switch_A == PRESSED) {            
                CLR_TIMER16;                                 // clear 16-bit 8mS tick counter  
                WAIT_SWITCH_RELEASED(switch_A);              // wait until button returns to quiescent state 
                GET_TIMER16(ticks);                          // read 16-bit 8mS tick counter
                if (ticks > (3 * TICKS_IN_ONE_SECOND)) {     // button press time > 3 seconds
                    pop_LED_display();
                    longjmp(env,1);
                }            
                switch (activity) {
                    case CMD_FORWARD    :
                        display_number(distance, 0);
                        distance =  (distance * WHEEL_CONSTANT) / 100;    // convert to wheel counts
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_FORWARD); seq_ptr++;    
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, LEFT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_FORWARD); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, RIGHT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;                          
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_16, IMMEDIATE, distance); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, DISTANCE); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, EXECUTE, NO_MOD, MOVE_DISTANCE); seq_ptr++; 
                        break;
                    case CMD_BACKWARD   :
                        display_number(distance, 0);  
                        distance =  (distance * WHEEL_CONSTANT) / 100;    // convert to wheel counts
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_BACKWARD); seq_ptr++;    
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, LEFT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_BACKWARD); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, RIGHT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;                     
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_16, IMMEDIATE, distance); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, DISTANCE); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, EXECUTE, NO_MOD, MOVE_DISTANCE); seq_ptr++; 
                        break;
                    case CMD_SPIN_LEFT  :
                        display_number(distance, 0);
                        if (distance == 0) { distance = 0;}
                        if (distance == 45) { distance = 8;}
                        if (distance == 90) { distance = 16;}       // convert to wheel counts
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_BACKWARD); seq_ptr++;    
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, LEFT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_FORWARD); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, RIGHT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;                     
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_16, IMMEDIATE, distance); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, DISTANCE); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, EXECUTE, NO_MOD, MOVE_DISTANCE); seq_ptr++; 
                        break;
                    case CMD_SPIN_RIGHT :
                        display_number(distance, 0);
                        if (distance == 0) { distance = 0;}
                        if (distance == 45) { distance = 8;}
                        if (distance == 90) { distance = 16;}      // convert to wheel counts 
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_FORWARD); seq_ptr++;    
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, LEFT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, WHEEL_SENSOR_CALIBRATE_SPEED); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, MOTOR_BACKWARD); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_L8, IMMEDIATE, RIGHT_MOTOR); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, SPEED); seq_ptr++;                     
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, PUSH_16, IMMEDIATE, distance); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, SET_PARAMETER, NO_MOD, DISTANCE); seq_ptr++;
                        store_instruction(shared.RAM_sequence.uint16, seq_ptr, EXECUTE, NO_MOD, MOVE_DISTANCE); seq_ptr++; 
                        break;
                    default : 
                        break;
                }
                show_dual_chars('F', (' '), (A_TO_FLASH | B_TO_FLASH | 10));
                activity = FIRST_CMD;
                pop_LED_display();
                break;
            }    
        }       // end of inner FOREVER loop
    }       // end of outer FOREVER loop
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// run_program_mode_2 : input and run timed sequences
// ==================
//
// Description
//      ????????
//
// Notes
//
//      Active switches are 
//          switch A = go/stop button
//          switch B = no effect
//          switch C = exit
//          switch D = start/stop scanning operation
//
//      Active pots
//

uint8_t run_program_mode_2(void) {

mode_state_t  state;
sequence_mode_t   seq_mode;

    state = MODE_INIT;
    seq_mode = FIRST_SEQUENCE_MODE;
    gLeft_Speed = SLOW_SPEED;
    gRight_Speed = gLeft_Speed - pwm_differential + DIFFERENTIAL_NULL;
    show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
    set_LED(LED_A, FLASH_ON);
    set_LED(LED_B, FLASH_ON);
    clr_LED(LED_A);
    set_LED(LED_D, FLASH_ON);
//
// main loop
//       
    FOREVER {
// 
// No POT input at this time
//
        if (switch_B == PRESSED) {
            SOUND_READ_POTS;
            WAIT_SWITCH_RELEASED(switch_B);
        }
//
//  check for exit
//
        if (switch_C == PRESSED) {            //  exit mode
            vehicle_stop();
            SOUND_EXIT_SELECTION;
            WAIT_SWITCH_RELEASED(switch_C);
            return 0;
        }              
//
//  run simple state machine to define operating modes
//  There are two states : MODE_INIT and MODE_RUNNING
//
        if (state == MODE_INIT) {
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                SOUND_NEXT_SELECTION;
                seq_mode++;
                if (seq_mode > LAST_SEQUENCE_MODE) {
                    seq_mode = FIRST_SEQUENCE_MODE;
                }
                show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
            }         
            if (switch_A == PRESSED) {            //  go to RUN state              
                state = MODE_RUNNING;
                set_LED(LED_A, FLASH_ON); 
                set_LED(LED_D, FLASH_ON);
                WAIT_SWITCH_RELEASED(switch_A); 
            } else {
                continue;
            }
        }
        if (state == MODE_RUNNING) {
            if (switch_A == PRESSED) {            // exit   
                vehicle_stop();
                clr_LED(LED_A);
                clr_LED(LED_D);
                state = MODE_INIT; 
                WAIT_SWITCH_RELEASED(switch_A);                               
                return 0;
            }
        } 
//
// run an experiment
// 
        show_dual_chars('S', ('0'+ seq_mode), 0);
        push_LED_display();       
        switch (seq_mode) {
            case PLAY :
                run_sequence(shared.RAM_sequence.uint16);
                break;            
            case COLLECT :
                input_timed_sequence();
                break;
            case SAVE :
                save_sequence(0);
                break;
            case RECALL :
                load_sequence(0);
                break;
            case DUMP :
                dump_sequence();
                break;                                                                  
            default :
                break;
        }
        state = MODE_INIT;
        seq_mode = FIRST_SEQUENCE_MODE;
        show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
        pop_LED_display();
    }                         // end of FOREVER loop
}

//----------------------------------------------------------------------------
// input_timed_sequence : input a timed sequence from switches
// ====================
//
// Description
//      User can load a sequence of commands into the command buffer.  Commands
//      are input from the fours keys.  Distances are measure in units of 1cm and
//      angles are measured 
//
// Notes
//
//      Active switches are 
//          switch A = go/stop button
//          switch B = no effect
//          switch C = exit
//          switch D = start/stop scanning operation
//
//      Active pots
//          None
//

uint8_t input_timed_sequence(void) {

uint8_t   activity, cmd_pt, time;
//
// show "forward command"
//
    show_dual_chars('F', (' '), (A_TO_FLASH | B_TO_FLASH | 10));
    cmd_pt = 0;
    activity = FIRST_CMD;
    push_LED_display();
    set_LED(LED_A, FLASH_ON);
    clr_LED(LED_A);
    set_LED(LED_C, FLASH_ON);
    set_LED(LED_D, FLASH_ON);
  
    FOREVER {
//
//  check and act on the switches
//
        if (switch_D == PRESSED) {
            WAIT_SWITCH_RELEASED(switch_D);
            activity++;
            SOUND_NEXT_SELECTION;
            if (activity > LAST_CMD) {
                activity = FIRST_CMD;
            }
            switch (activity) {
                case CMD_FORWARD    : show_dual_chars('F', (' '), (B_TO_FLASH | 10)); break;
                case CMD_BACKWARD   : show_dual_chars('b', (' '), (B_TO_FLASH | 10)); break;
                case CMD_SPIN_LEFT  : show_dual_chars('L', (' '), (B_TO_FLASH | 10)); break;
                case CMD_SPIN_RIGHT : show_dual_chars('r', (' '), (B_TO_FLASH | 10)); break;
                default : break;
            }
        }
        
        if (switch_C == PRESSED) {            //  Load termination command
            shared.seq.strip_data[cmd_pt][0] = CMD_STOP;
            shared.seq.strip_data[cmd_pt][1] = 0;
            WAIT_SWITCH_RELEASED(switch_C);
            pop_LED_display();
            return 0;
        }
                
        if (switch_A == PRESSED) {         // accept command and move on to read distance parameter     
            push_LED_display();
            set_LED(LED_A, FLASH_ON);
            set_LED(LED_B, FLASH_ON);
            set_LED(LED_C, FLASH_ON);
            set_LED(LED_D, FLASH_ON);
            WAIT_SWITCH_RELEASED(switch_A); 
        } else {
            continue;
        }
//
// Here if command has been selected : load command into buffer
//
        shared.seq.strip_data[cmd_pt][0] = activity;
//
// now find time period.  Set initial guess for time
// 
        switch (activity) {
            case CMD_FORWARD    : time =  1; break;
            case CMD_BACKWARD   : time =  1; break;
            case CMD_SPIN_LEFT  : time =  5; break;
            case CMD_SPIN_RIGHT : time =  5; break;
            default : break;
        }
        display_number(time, 0);     
        
        FOREVER {
//
// switch C increments the time parameter to a max value of 99
//        
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                if (time != 0) {
                    time--;
                    SOUND_NEXT_SELECTION;
                }
                display_number(time, 0); 
            }
//
// switch D decrements the time parameter to a min value of 0
//                                   
            if (switch_C == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_C);
                time++;
                if (time > 99) {
                    time = 99;
                }
                display_number(time, 0); 
            }      
//
// switch B to exit 
//
            if (switch_B == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_B);
                pop_LED_display();
                return 0;
            }
//
// switch A to accept an load command
//
            if (switch_A == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_A);
                shared.seq.strip_data[cmd_pt][1] = time;
                time = 0;
                cmd_pt++;
                activity = FIRST_CMD;
                show_dual_chars('F', (' '), (A_TO_FLASH | B_TO_FLASH | 10));
                pop_LED_display();
                break;
            }    
        }
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// run_program_mode_1 : program vehicle using line following optical sensors
// ==================
//
// Description
//      User creates a black and white linear strip pattern which is read into
//      the robot by scanning with the front line optical sensors.
//
// Notes
//
//      Active switches are 
//          switch A = go/stop button
//          switch B = 
//          switch C = exit
//          switch D = start/stop scanning operation
//
//      Active pots
//

uint8_t run_program_mode_1(void) {

mode_state_t  state;
sequence_mode_t   seq_mode;

    state = MODE_INIT;
    seq_mode = FIRST_SEQUENCE_MODE;
    gLeft_Speed = SLOW_SPEED;
    gRight_Speed = SLOW_SPEED;
    show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
    set_LED(LED_A, FLASH_ON);
    set_LED(LED_B, FLASH_ON);
    clr_LED(LED_C);
    set_LED(LED_D, FLASH_ON);
//
// main loop
//       
    FOREVER {
// 
// No POT input at this time
//
        if (switch_B == PRESSED) {
            WAIT_SWITCH_RELEASED(switch_B);        
        }
//
//  check for exit
//
        if (switch_C == PRESSED) {            //  exit mode
            vehicle_stop();
            SOUND_EXIT_SELECTION;
            WAIT_SWITCH_RELEASED(switch_C);
            return 0;
        }              
//
//  run simple state machine to define operating modes
//  There are two states : MODE_INIT and MODE_RUNNING
//
        if (state == MODE_INIT) {
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                SOUND_NEXT_SELECTION;
                seq_mode++;
                if (seq_mode > LAST_SEQUENCE_MODE) {
                    seq_mode = FIRST_SEQUENCE_MODE;
                }
                show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
            }         
            if (switch_A == PRESSED) {            //  go to RUN state              
                state = MODE_RUNNING;
                set_LED(LED_A, FLASH_ON); 
                set_LED(LED_D, FLASH_ON);
                WAIT_SWITCH_RELEASED(switch_A); 
            } else {
                continue;
            }
        }
        if (state == MODE_RUNNING) {
            if (switch_A == PRESSED) {            // exit   
                vehicle_stop();
                clr_LED(LED_A);
                clr_LED(LED_D);
                state = MODE_INIT; 
                WAIT_SWITCH_RELEASED(switch_A);                              
                return 0;
            }
        } 
//
// run an experiment
// 
        show_dual_chars('S', ('0'+ seq_mode), 0);
        push_LED_display();       
        switch (seq_mode) {
            case PLAY :
                play_sequence();
                break;            
            case COLLECT :
                scan_strips_drive();
                break;
            case SAVE :
                break;
            case RECALL :
                break;
            case DUMP :
                dump_strips();
                break;                                                                  
            default :
                break;
        }
        state = MODE_INIT;
        seq_mode = FIRST_SEQUENCE_MODE;
        show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
        pop_LED_display();
    }                         // end of FOREVER loop
}

//----------------------------------------------------------------------------
// scan_strips_drive : read black/white sequence from paper strips
// =================
//
// Description
//      ........
//
// Notes
//
//      Active switches are 
//          switch D = start/stop scan process
//   
uint8_t scan_strips_drive(void) {

uint8_t   line_L, line_R, current_L, current_R, time, cmd_pt, current_cmd, new_cmd, i, ad_value;
int8_t    speed_differential, speed, gLeft_Speed, gRight_Speed;
uint16_t  temp16;
uint16_t  init_time;

    clr_all_LEDs();
    set_LED(LED_D, FLASH_ON);
//
// switch D used as start command
//
    WAIT_SWITCH_PRESSED(switch_D);
    WAIT_SWITCH_RELEASED(switch_D);
    
// 
// read speed (POT1) and differential (POT2) settings
//
    ad_value = get_adc(POT_1);
    speed = (ad_value >> 2) & 0x3F;           // set speed range to 0 -> 64

    ad_value = ((get_adc(POT_2) >> 2) & 0x3F);   // ignore top two bits
    temp16 = (ad_value * 50)/63;                    // convert from 0->63 to 0->50         
    speed_differential = (temp16 - 25);             // convert to range -25% -> +25% 
    if (speed_differential > 0) {
        gLeft_Speed = speed;
        gRight_Speed = speed - speed_differential;
    } else {
        speed_differential = -speed_differential;
        gLeft_Speed = speed - speed_differential; 
        gRight_Speed = speed;
    }

    set_motor(LEFT_MOTOR, MOTOR_FORWARD, gLeft_Speed);
    set_motor(RIGHT_MOTOR, MOTOR_FORWARD, gRight_Speed);
            
    set_LED(LED_D, FLASH_OFF);
    play_tune(&snd_beeps_1);
//
// initialise time count and line sensor values
//    
    init_time = tick_count_16;               // note current time
    current_L = READ_TAPE_SENSOR(LINE_SENSOR_L); 
    current_R = READ_TAPE_SENSOR(LINE_SENSOR_R);
//
//  initialise data for first command
//
    cmd_pt = 0;
    time = 1; 
    current_cmd = ((current_L << 1) & 0x02) + current_R;  
//
// scan sensors at 0.1 sec intervals for a maximum of 20 seconds  
//  
    for (i=0 ; i < 200 ; i++) {
//
// read line sensors.   1=WHITE, 0=BLACK
//
        line_L = READ_TAPE_SENSOR(LINE_SENSOR_L);
        if (line_L == BLACK) {
            set_LED(LED_B, FLASH_OFF);
        } else {
            clr_LED(LED_B);
        }
        line_R = READ_TAPE_SENSOR(LINE_SENSOR_R); 
        if (line_R == BLACK) {
            set_LED(LED_A, FLASH_OFF);
        } else {
            clr_LED(LED_A);
        }        
        new_cmd = (line_L << 1) + line_R;
        if (new_cmd != current_cmd) {
            shared.seq.strip_data[cmd_pt][0] = current_cmd;
            shared.seq.strip_data[cmd_pt][1] = time;
            if (new_cmd == STRIP_CMD_STOP) {   // exit scan process
                vehicle_stop();
                shared.seq.strip_data[cmd_pt][0] = current_cmd;
                shared.seq.strip_data[cmd_pt][1] = time;
                cmd_pt++;                        
                shared.seq.strip_data[cmd_pt][0] = STRIP_CMD_STOP;
                shared.seq.strip_data[cmd_pt][1] = 0;
                stop_tune();
                return  cmd_pt;
            }
            time = 1;
            current_cmd = new_cmd;
            cmd_pt++;
            if (cmd_pt > MAX_STRIP_CMDS) {
                cmd_pt--;
            }
        } else {
            time++;
        }
        if (switch_D == PRESSED) {       //  exit when switch D is pressed           
            set_LED(LED_A, FLASH_OFF); 
            set_LED(LED_D, FLASH_ON);
            WAIT_SWITCH_RELEASED(switch_D);
            break;
        }
        DelayMs(100);    // sample time is 0.1 second       
    }
//
// stop and tidy end of sequence
//
    vehicle_stop();
    stop_tune();    
    shared.seq.strip_data[cmd_pt][0] = current_cmd;
    shared.seq.strip_data[cmd_pt][1] = time;
    cmd_pt++;
    if (cmd_pt > MAX_STRIP_CMDS) {
        cmd_pt--;
    }
    shared.seq.strip_data[cmd_pt][0] = STRIP_CMD_STOP;      // termination command
    shared.seq.strip_data[cmd_pt][1] = 0;

    return cmd_pt;
}

//----------------------------------------------------------------------------
// play_sequence : run a set of commands input from a strip scan
// =============
//
// Description
//      Execute the set of commands held in the strips array. Each command consist of
//      two bytes
//          command
//              byte 0 : command STRIP_SCAN_BOTH_OFF,STRIP_SCAN_LEFT_OFF,STRIP_SCAN_RIGHT_OFF,STRIP_SCAN_BOTH_ON
//              byte 1 : duration of command in units of 100mS.
//                         termination is signaled by a duration of zero
//
// Notes
//

uint8_t play_sequence(void) {

uint8_t  i, duration;

    for (i=0 ; i < MAX_STRIP_CMDS ; i++) {
        duration = shared.seq.strip_data[i][1];
        switch (shared.seq.strip_data[i][0]) {
            case CMD_STOP :
            case STRIP_CMD_STOP :
                set_motor(LEFT_MOTOR, MOTOR_BRAKE, 0);
                set_motor(RIGHT_MOTOR, MOTOR_BRAKE, 0);
                if (duration == 0) {           // exit if duration is zero
                    return  0;
                }
                DelayMs(duration * 100);          
                break;
            case CMD_SPIN_LEFT :
            case STRIP_CMD_SPIN_LEFT :
                set_motor(LEFT_MOTOR, MOTOR_BACKWARD, STRIP_PLAY_SPIN_SPEED);
                set_motor(RIGHT_MOTOR, MOTOR_FORWARD, STRIP_PLAY_SPIN_SPEED);
                DelayMs(duration * 100);          
                break;
            case CMD_SPIN_RIGHT :
            case STRIP_CMD_SPIN_RIGHT :
                set_motor(LEFT_MOTOR, MOTOR_FORWARD, STRIP_PLAY_SPIN_SPEED);
                set_motor(RIGHT_MOTOR, MOTOR_BACKWARD, STRIP_PLAY_SPIN_SPEED);
                DelayMs(duration * 100);          
                break;
            case CMD_FORWARD :
            case STRIP_CMD_FORWARD :
                set_motor(LEFT_MOTOR, MOTOR_FORWARD, STRIP_PLAY_FORWARD_SPEED);
                set_motor(RIGHT_MOTOR, MOTOR_FORWARD, STRIP_PLAY_FORWARD_SPEED);
                DelayMs(duration * 100);            
                break;
            default :
                return  0;
                break;  
        }
    }
}

//----------------------------------------------------------------------------
// dump_strips : dump current array of strip commands
// ===========
//
// Description
//      Sends the current list of strip commands to the serial port in ASCII format
//
// Notes
//
uint8_t dump_strips(void) {

uint8_t  i, duration , strip_cmd;

    send_msg("Current list of strip commands\r\n");
    for (i=0 ; i < MAX_STRIP_CMDS ; i++) {
        strip_cmd = shared.seq.strip_data[i][0];
        duration = shared.seq.strip_data[i][1];
        send_msg("COMMAND = ");
        send_msg(bcd(strip_cmd, tempstring));
        send_msg(" :: DURATION = ");
        send_msg(bcd(duration, tempstring));
        send_msg("\r\n");   
    }
    return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// run_program_mode_3 : program vehicle using line following optical sensors
// ==================
//
// Description
//      User creates a black and white linear strip pattern which is read into
//      the robot by scanning with the front line optical sensors.
//
// Notes
//
//      Active switches are 
//          switch A = accept "S" mode
//          switch B = 
//          switch C = exit 
//          switch D = step to next "S" mode  (S0->S4)
//
//      Active pots
//          POT_1 : 
//          POT_2 : 
//          POT_3 : 
//

uint8_t run_program_mode_3(void) {

mode_state_t  state;
sequence_mode_t   seq_mode;


    state = MODE_INIT;
    seq_mode = FIRST_SEQUENCE_MODE;
    show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
    set_LED(LED_A, FLASH_ON);
    clr_LED(LED_B);
    set_LED(LED_C, FLASH_ON);
    set_LED(LED_D, FLASH_ON);
//
// main loop
//       
    FOREVER {
// 
// No POT input at this time
//
        if (switch_B == PRESSED) {
            SOUND_READ_POTS; 
        }
//
//  check for exit
//
        if (switch_C == PRESSED) {            //  exit mode
            vehicle_stop();
            SOUND_EXIT_SELECTION;
            WAIT_SWITCH_RELEASED(switch_C);
            return 0;
        }              
//
//  run simple state machine to define operating modes
//  There are two states : MODE_INIT and MODE_RUNNING
//
        if (state == MODE_INIT) {
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                SOUND_NEXT_SELECTION;
                seq_mode++;
                if (seq_mode > LAST_SEQUENCE_MODE) {
                    seq_mode = FIRST_SEQUENCE_MODE;
                }
                show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
            }         
            if (switch_A == PRESSED) {            //  go to RUN state              
                state = MODE_RUNNING;
                set_LED(LED_A, FLASH_ON); 
                set_LED(LED_D, FLASH_ON);
                WAIT_SWITCH_RELEASED(switch_A); 
            } else {
                continue;
            }
        }
        if (state == MODE_RUNNING) {
            if (switch_A == PRESSED) {            // exit   
                vehicle_stop();
                clr_LED(LED_A);
                clr_LED(LED_D);
                state = MODE_INIT; 
                WAIT_SWITCH_RELEASED(switch_A);                               
                return 0;
            }
        } 
//
// run an experiment
// 
        show_dual_chars('S', ('0'+ seq_mode), 0);
        push_LED_display();       
        switch (seq_mode) {
            case PLAY :
                play_sequence();
                break;            
            case COLLECT :
                scan_strips_push();
                break;
            case SAVE :
                break;
            case RECALL :
                break;
            case DUMP :
                dump_strips();
                break;                                                                  
            default :
                break;
        }
        state = MODE_INIT;
        seq_mode = FIRST_SEQUENCE_MODE;
        show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
        pop_LED_display();
    }                         // end of FOREVER loop
}



//----------------------------------------------------------------------------
// scan_strips_push : read black/white sequence from paper strips
// ================
//
// Description
//      ........
//
// Notes
//
//      Active switches are 
//          switch D = start/stop scan process
//   
uint8_t scan_strips_push(void) {

uint8_t   line_L, line_R, current_L, current_R, time, cmd_pt, current_cmd, new_cmd, time_samples;

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
// initialise time count and line sensor values
//    
    current_L = READ_TAPE_SENSOR(LINE_SENSOR_L); 
    current_R = READ_TAPE_SENSOR(LINE_SENSOR_R);
//
//  initialise data for first command
//
    cmd_pt = 0;
    time = 1; 
    current_cmd = ((current_L << 1) & 0x02) + current_R;  
//
// scan sensors at 0.1 sec intervals for a maximum of 20 seconds  
//  
    for (time_samples=0 ; time_samples < 200 ; time_samples++) {
//
// read line sensors.   1=WHITE, 0=BLACK
//
        line_L = READ_TAPE_SENSOR(LINE_SENSOR_L);
        if (line_L == BLACK) {
            set_LED(LED_B, FLASH_OFF);
        } else {
            clr_LED(LED_B);
        }
        line_R = READ_TAPE_SENSOR(LINE_SENSOR_R); 
        if (line_R == BLACK) {
            set_LED(LED_A, FLASH_OFF);
        } else {
            clr_LED(LED_A);
        }        
        new_cmd = (line_L << 1) + line_R;
        if (new_cmd != current_cmd) {
        	shared.seq.strip_data[cmd_pt][0] = current_cmd;
        	shared.seq.strip_data[cmd_pt][1] = time;
            if (new_cmd == STRIP_CMD_STOP) {   // exit scan process
                cmd_pt++;                        
                shared.seq.strip_data[cmd_pt][0] = STRIP_CMD_STOP;
                shared.seq.strip_data[cmd_pt][1] = 0;
                stop_tune();
                return  cmd_pt;
            }
            time = 1;
            current_cmd = new_cmd;
            cmd_pt++;
            if (cmd_pt > MAX_STRIP_CMDS) {
                cmd_pt--;
            }
        } else {
            time++;
        }
        if (switch_D == PRESSED) {       //  exit when switch D is pressed           
            set_LED(LED_A, FLASH_OFF); 
            set_LED(LED_D, FLASH_ON);
            WAIT_SWITCH_RELEASED(switch_D);
            stop_tune();
            return  cmd_pt; 
        }
        DelayMs(100);    // sample time is 0.1 second       
    }
    shared.seq.strip_data[cmd_pt][0] = current_cmd;
    shared.seq.strip_data[cmd_pt][1] = time;
    cmd_pt++;
    if (cmd_pt > MAX_STRIP_CMDS) {
        cmd_pt--;
    }
    shared.seq.strip_data[cmd_pt][0] = STRIP_CMD_STOP;      // termination command
    shared.seq.strip_data[cmd_pt][1] = 0;
    tone_off();
    sys_error = TIME_OUT;
    return cmd_pt;
}

//----------------------------------------------------------------------------
// save_sequence : save a robot command sequence from RAM to FLASH
// =============
//
// Description
//      1. erase specified page
//      2. write byte stream
//
// Notes
//  
void save_sequence(uint8_t flash_seq_no) 
{
uint8_t  count;

    if (flash_seq_no == 0) {
        FlashErasePage((uint16_t)&FLASH_seq_0.uint8[0]);
        for (count = 0 ; count < sizeof(shared.RAM_sequence) ; count++) {
            FlashProgramByte((uint16_t)&FLASH_seq_0.uint8[count], shared.RAM_sequence.uint8[count]); 
        }
    }
}

//----------------------------------------------------------------------------
// load_sequence : copy a robot command sequence from FLASH to RAM
// =============
//
// Description
//
// Notes
//  
void load_sequence(uint8_t flash_seq_no) 
{
    if (flash_seq_no == 0) {
        memcpy(&shared.RAM_sequence.uint8[0], &FLASH_seq_0.uint8[0],  (RAM_SEQUENCE_SIZE*2)); 
    }
}

//----------------------------------------------------------------------------
// dump_sequence : dump a printed version of a robot sequence onto the serial port
// =============
//
// Description
//
// Notes
//  
void dump_sequence(void) 
{
uint8_t    i;

    send_msg("Robot commands in RAM sequence 0\r\n");
    for (i=0 ; i < (sizeof(shared.RAM_sequence.uint16)) ; i++) {
        if (shared.RAM_sequence.uint16[i] == 0xFFFF) {   // unused entries show as all 1's
            break;
        };
        decode_command(shared.RAM_sequence.uint16[i]);
        send_msg(bcd(robot_command.op_code, tempstring)); send_msg("\t");
        send_msg(bcd(robot_command.modifier, tempstring)); send_msg("\t");
        send_msg(bcd(robot_command.data, tempstring));
        send_msg("\r\n");   
    }        
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// run_program_mode_4 : download and run ubasic+ programs
// ==================
//
// Description
//      ubasic is a simple integer only small basic interpreter written by Adam Dunkels.
//      In turn this was modified to remove line numbers and incorporate some block
//      structures - while, for, {}.
//		This has been incorpoated into the Robokid robot with some additional commands
//		that allow access to the hardware features of the robot.
//
// Notes
//
//      Active switches are 
//          switch A = accept "S" mode
//          switch B = 
//          switch C = exit 
//          switch D = step to next "S" mode  (S0->S4)
//
//      Active pots
//          POT_1 : 
//          POT_2 : 
//          POT_3 : 
//

uint8_t run_program_mode_4(void) {

mode_state_t  state;
sequence_mode_t   seq_mode;


    state = MODE_INIT;
    seq_mode = FIRST_SEQUENCE_MODE;
    show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
    set_LED(LED_A, FLASH_ON);
    clr_LED(LED_B);
    set_LED(LED_C, FLASH_ON);
    set_LED(LED_D, FLASH_ON);
//
// main loop
//       
    FOREVER {
// 
// No POT input at this time
//
        if (switch_B == PRESSED) {
            SOUND_READ_POTS; 
        }
//
//  check for exit
//
        if (switch_C == PRESSED) {            //  exit mode
            vehicle_stop();
            SOUND_EXIT_SELECTION;
            WAIT_SWITCH_RELEASED(switch_C);
            return 0;
        }              
//
//  run simple state machine to define operating modes
//  There are two states : MODE_INIT and MODE_RUNNING
//
        if (state == MODE_INIT) {
            if (switch_D == PRESSED) {
                WAIT_SWITCH_RELEASED(switch_D);
                SOUND_NEXT_SELECTION;
                seq_mode++;
                if (seq_mode > LAST_SEQUENCE_MODE) {
                    seq_mode = FIRST_SEQUENCE_MODE;
                }
                show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
            }         
            if (switch_A == PRESSED) {            //  go to RUN state              
                state = MODE_RUNNING;
                set_LED(LED_A, FLASH_ON); 
                set_LED(LED_D, FLASH_ON);
                WAIT_SWITCH_RELEASED(switch_A); 
            } else {
                continue;
            }
        }
        if (state == MODE_RUNNING) {
            if (switch_A == PRESSED) {            // exit   
                vehicle_stop();
                clr_LED(LED_A);
                clr_LED(LED_D);
                state = MODE_INIT; 
                WAIT_SWITCH_RELEASED(switch_A);                               
                return 0;
            }
        } 
//
// run one of the 5 program modes
// 
        show_dual_chars('S', ('0'+ seq_mode), 0);
        push_LED_display();       
        switch (seq_mode) {
            case PLAY :
                play_sequence();
                break;            
            case COLLECT :
                get_ubasicp_program();
                break;
            case SAVE :
                break;
            case RECALL :
                break;
            case DUMP :
                dump_strips();
                break;                                                                  
            default :
                break;
        }
        state = MODE_INIT;
        seq_mode = FIRST_SEQUENCE_MODE;
        show_dual_chars('S', ('0'+ seq_mode), (A_TO_FLASH | 10));
        pop_LED_display();
    }                         // end of FOREVER loop
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
int8_t    style;

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
    shared.ubasic_program_space[0] = '\0';
    string_ptr = 0;
    CLR_TIMER16;                        // clear 16-bit 8mS tick counter
//
//  Read lines of characters from the serial port
//
    FOREVER {
    	read_line(tempstring);
    	style = trim_line(tempstring);
    	if (tempstring[0] == '@') {			// @ = end of program transfer
    		break;
    	}
    	if (style == LINE_BLANK) {          // don't store blank lines
    		continue;
    	}
    	if (style == LINE_NO_TERM) {
    		break;
    	}
    	string_ptr = store_line(string_ptr, tempstring);
    }
	tempstring[string_ptr] = '\0';		// ensure there is a null terminator
//
// Store buffer in FLASH (dumps entire 512 bytes)
//
	save_ubasicp_program(0);
	
    return 0;
}

//----------------------------------------------------------------------------
// trim_line : remove any unnecessary characters from a ubasic+ line of code
// =========
//
// Description
//		In order to save same in the limited ubasic+ 512 character storage buffer
//		this routine strips out any redundant spaces and indicated any lines that
//		need not be stores e.g. comment lines and lines with no characters.
//
// Notes
//   
int8_t trim_line(char line[]) {
	
uint8_t		  start_ptr, end_ptr, in_ptr, out_ptr, i, status;
line_scan_t	  scan_state;
//
// 1. Initialise start and end buffer pointers
//
	start_ptr = 0;
	end_ptr = 0;
	for (i=0 ; i < TEMP_STRING_SIZE ; i++) {
		if (line[i] != '\n') {
			end_ptr++;
		} else {
			break;
		}
	}
	if (end_ptr == 0) {
		return LINE_NO_TERM;
	}
//
// 2. convert all tab characters to space characters
//
	for (i=0 ; i < TEMP_STRING_SIZE ; i++) {
		if (line[i] == '\t') {
			line[i] = ' ';
		}
	}
//
// 3. run MEALY state machine to remove additional spaces (see project notes)
//
	scan_state = L_START;
	in_ptr = 0;
	out_ptr = 0;
	FOREVER {
		if (in_ptr >= (TEMP_STRING_SIZE - 1)) {
			status = LINE_USEFUL;		// check to ensure that scan does not overrun end of buffer
			scan_state = L_EXIT;			
		}
		switch (scan_state) {
		case L_START :
			if (line[in_ptr] == ' ') {
				in_ptr++;
			} else if (line[in_ptr] == '\n') {
				line[out_ptr++] = line[in_ptr];
				status = LINE_BLANK;
				scan_state = L_EXIT;
			} else {
				scan_state = L_SCAN;
			}
			break;
		case L_SCAN:
			if (line[in_ptr] == '#') {
				line[out_ptr] = '\n';
				if (out_ptr == 0) {
					status = LINE_BLANK;
					scan_state = L_EXIT;
				} else {
					status = LINE_USEFUL;
					scan_state = L_EXIT;
				}
			} else if (line[in_ptr] == '\'') {
					line[out_ptr++] = line[in_ptr++];
					scan_state = L_COPY_STRING;
			} else if (line[in_ptr] == ' ') {
					line[out_ptr++] = line[in_ptr++];
					scan_state = L_SKIP_SPACES;
			} else if (line[in_ptr] == '\n') {
					line[out_ptr] = line[in_ptr];
					if (out_ptr == 0) {
						status = LINE_BLANK;
						scan_state = L_EXIT;
					} else {
						status = LINE_USEFUL;
						scan_state = L_EXIT;
					}
			} else {
				line[out_ptr++] = line[in_ptr++];
			}
			break;
		case L_COPY_STRING:
			if (line[in_ptr] == '\'') {
				scan_state = L_SCAN;
			}
			line[out_ptr++] = line[in_ptr++];
			break;
		case L_SKIP_SPACES:
			if (line[in_ptr] == ' ') {
				in_ptr++;
			} else {
				line[out_ptr++] = line[in_ptr++];
				scan_state = L_SCAN;
			}
			break;
		case L_EXIT:
			return status;
			break;
		default:
			break;
		}
	}
}

//----------------------------------------------------------------------------
// store_line : copy line of ubasic+ code to the program buffer
// ==========
//
// Description
//		
//
// Notes
//   
uint8_t store_line(uint8_t buffer_ptr, char string[]) {

uint8_t		ptr;
char		ch;

	ptr=0;
	do {
		ch = string[ptr++];
		string[buffer_ptr++] = ch;
	} while (ch != '\n');
	return buffer_ptr;
}

//----------------------------------------------------------------------------
// save_ubasicp_prog : save a ubasic+ program from RAM to FLASH
// =================
//
// Description
//      1. erase specified page
//      2. write byte stream
//
// Notes
//  
void save_ubasicp_program(uint8_t flash_seq_no) 
{
uint8_t  count;

    if (flash_seq_no == 0) {
        FlashErasePage((uint16_t)&FLASH_seq_0.uint8[0]);
        for (count = 0 ; count < sizeof(shared.RAM_sequence) ; count++) {
            FlashProgramByte((uint16_t)&FLASH_seq_0.uint8[count], shared.RAM_sequence.uint8[count]); 
        }
    }
}
