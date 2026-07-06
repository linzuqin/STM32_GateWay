#ifndef _MSH_H_
#define _MSH_H_

#define MSH_CMD_MAX_LEN    32
#define MSH_PROFILE_MAX_LEN    32
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
    char cmd[MSH_CMD_MAX_LEN];
    char desc[MSH_PROFILE_MAX_LEN];
    void (*callback)(int argc, char *argv);
}msh_cmd_table_t;
extern msh_cmd_table_t msh_cmd_table[];

void msh_process(void);
void msh_rx_data(uint8_t *data , uint16_t size);
void msh_init(void);



#endif
