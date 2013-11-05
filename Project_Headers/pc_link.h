//----------------------------------------------------------------------------
//
//                  Robokid
//
//----------------------------------------------------------------------------
// pc_link.h    header file for pc_link.c
// =========
//
//
// Author                Date          Comment
//----------------------------------------------------------------------------
// Jim Herd           04/11/2013    Initial edits
//----------------------------------------------------------------------------

#include "global.h"

#ifndef PC_LINK_H_
#define PC_LINK_H_

uint8_t  check_for_computer_link(void);
void     run_PC_link(void);
uint8_t  check_line(char line[]);
uint8_t  process_PC_cmd(char line[]);
uint8_t  get_ubasicp_program(void);

#endif /* PC_LINK_H_ */
