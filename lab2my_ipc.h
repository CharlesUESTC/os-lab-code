/**
 * @brief: Common Header for Message Queue and Message Struct
 */
#ifndef _MY_IPC_H_
#define _MY_IPC_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define FALSE 0
#define TRUE  1
#define T_TEXT 1
#define FTOK_ID 'a'        // id used in ftok to generate a key

typedef struct message 
{
    long m_type;            // refer to sys/msg.h msgbuf struct template
    pid_t m_sender;         // sender pid
    char m_data[20];    // BUFSIZ is defined in stdio.h
} message_t; 

#endif // ! _MY_IPC_H_
