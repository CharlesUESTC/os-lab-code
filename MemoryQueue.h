#ifndef __MEMORYQUEUE_H_
#define __MEMORYQUEUE_H_

#define O_RDONLY 	(1<<0) 		//Open the queue to receive messages only
#define O_WRONLY 	(1<<1)		//Open the queue to send messages only
#define O_RDWR 		(1<<2)		//Open the queue to both send and receive messages
#define O_NONBLOCK	(1<<3)		//Open the queue in nonblocking mode
#define O_CREAT		(1<<4)		//Create the message queue if it does not exist
#define O_EXCL		(1<<5)		//If O_CREAT was specified in oflag, and a queue with the given name already exists, then fail with the error EEXIST


/************
open errors:
*************/
//The queue exists, but the caller does not have permission to open it in the specified mode.
//name contained more than one slash.
#define EACCES 			-1

//Both O_CREAT and O_EXCL were specified in oflag, but a queue with this name already exists.
#define EEXIST 			-2

//O_CREAT was specified in oflag, and attr was not NULL, but attr->mq_maxmsg or attr->mq_msqsize was invalid. 
//Both of these fields must be greater than zero. In a process that is unprivileged (does not have the CAP_SYS_RESOURCE capability), 
//attr->mq_maxmsg must be less than or equal to the msg_max limit, and attr->mq_msgsize must be less than or equal to the msgsize_max limit. 
//In addition, even in a privileged process, attr->mq_maxmsg cannot exceed the HARD_MAX limit. (See mq_overview(7) for details of these limits.)
#define EINVAL 			-3

//The process already has the maximum number of files and message queues open.
#define EMFILE 			-4

//name was too long.
#define ENAMETOOLONG 	-5

//The system limit on the total number of open files and message queues has been reached.
#define ENFILE			-6

//The O_CREAT flag was not specified in oflag, and no queue with this name exists.
//name was just "/" followed by no other characters.
#define ENOENT			-7

//Insufficient memory.
#define ENOMEM			-8

//Insufficient space for the creation of a new message queue.
#define ENOSPC			-9

/************
close errors:
*************/

#define EBADF			-10

/************
send/recieve errors:
*************/

#define EAGAIN			-11

#define EINTR			-12

#define EMSGSIZE		-13

#define ETIMEDOUT		-14


//////////////////////////
#define MAX_STR_LENGTH 32
#define MAX_MESSAGE_SIZE 32

typedef int mqd_t;

struct message_t_s
{
	char* data;
	size_t size;
	unsigned priority;
	struct message_t_s* next_message;
};

typedef struct message_t_s message_t;

//msg queue in linked list
struct queue_data_t_s
{	
	char name [MAX_STR_LENGTH];
	int queue_id;
	unsigned size;
	unsigned flag;
	message_t* head;
	struct queue_data_t_s* next_queue;
	OS_SEM messages_ready;
};

typedef struct queue_data_t_s queue_data_t;



mqd_t mq_open(const char *name, unsigned flags);

mqd_t mq_close(mqd_t queue_descriptor);

mqd_t mq_send(mqd_t queue_descriptor, const char *message_ptr, size_t message_len, unsigned message_priority);

mqd_t mq_receive(mqd_t queue_descriptor, char *message_ptr, size_t message_len, unsigned* message_priority);

queue_data_t * find_queue(mqd_t queue_id);


#endif
