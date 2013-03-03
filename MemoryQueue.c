#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include <stdio.h>
#include <stdlib.h>
#include "Serial.h"
#include "RTL.h"
#include <cstring>

#include "MemoryQueue.h"


queue_data_t * first_queue;


mqd_t mq_open(const char *name, unsigned flags)
{
	int i = 0;
	//check through name of each message queue from the linked-list
	queue_data_t* current_queue = first_queue;
	queue_data_t* previous_queue = NULL;

	
	if(strlen(name) >= MAX_STR_LENGTH){
		return ENAMETOOLONG;
	}
	if(strlen(name) <= 0){
		return ENOENT;
	}

	while(current_queue != NULL)
	{
		if(strcmp(current_queue->name,name) == 0)
		{
			//the name matches
			//if user passes a flag and create and the queue already exists
			if( (flags & O_CREAT) && (flags & O_EXCL))	
			{
				return EEXIST;
			}
			else
			{
				return current_queue->queue_id;
			}
		}
		previous_queue = current_queue;
		current_queue = current_queue->next_queue;
	}
	//alloc memory for the message queue
	
	if (flags & O_CREAT)
	{
		if(flags & O_NONBLOCK){
			current_queue = (queue_data_t* ) os_mem_alloc( sizeof(queue_data_t), MEM_NOWAIT );
			if(((int)current_queue) == 0){
				return ENOMEM;
			}
		}else{
			current_queue = (queue_data_t* ) os_mem_alloc( sizeof(queue_data_t), MEM_WAIT );
		}
		
		for(i = 0; i < strlen(name); ++i){
			current_queue->name[i] = name[i];
		}
		current_queue->queue_id = (int)current_queue;
		current_queue->size = MAX_MESSAGE_SIZE;
		current_queue->flag = flags;
		current_queue->head = NULL;
		current_queue->next_queue = NULL;
		os_sem_init(current_queue->messages_ready, 0);
		if(previous_queue != NULL){
			previous_queue->next_queue = current_queue;
		}
		if(first_queue == NULL){
			first_queue = current_queue;
		}
		return current_queue->queue_id;
	}
	
	//ERROR
	return ENOENT;
}

mqd_t mq_close(mqd_t queue_descriptor)
{

	//check through name of each message queue from the linked-list

	queue_data_t* current_queue = first_queue;
	queue_data_t* previous_queue = NULL;
	message_t* current_message = NULL;	
	message_t* previous_message = NULL;	
	int ret = 0;

	while(current_queue != NULL)
	{
		if( current_queue->queue_id == queue_descriptor )
		{
			//the name matches
			if(previous_queue == NULL){
				first_queue = current_queue->next_queue;
			}else{
				previous_queue->next_queue = current_queue->next_queue;
			}
			
			current_message = current_queue->head;
			while(current_message != NULL){
				previous_message = current_message;
				current_message = current_message->next_message;
				os_mem_free(previous_message);
			}
			ret = current_queue->queue_id;
			os_mem_free(current_queue);

			return ret;
		}
		previous_queue = current_queue;
		current_queue = current_queue->next_queue;
	}
	//ERROR!
	return EBADF;
}

mqd_t mq_send(mqd_t queue_descriptor, const char *message_ptr, size_t message_len, unsigned message_priority)
{
	int i;
	message_t* send_msg;
	message_t* prev;
	message_t* next;
	char * message_str;

	queue_data_t * current_queue = find_queue(queue_descriptor);
	if(current_queue == NULL)
	{
		return EBADF;
	}
	
	if((current_queue->flag & O_RDONLY))
	{
		return EBADF;
	}
	if(current_queue->size < message_len){
		return EMSGSIZE;
	}
	
	if(current_queue->flag & O_NONBLOCK){
		message_str = os_mem_alloc( message_len * sizeof(char), MEM_NOWAIT );
		
		if(((int)message_str) == 0){
			//ERROR HERE
			return EAGAIN;
		}

		send_msg = os_mem_alloc( sizeof(message_t), MEM_NOWAIT );
		if(((int)message_str) == 0){
			//ERROR HERE
			os_mem_free(message_str);
			return EAGAIN;
		}
	

	}else{
		 message_str = os_mem_alloc( message_len * sizeof(char), MEM_WAIT );
		 send_msg = os_mem_alloc( sizeof(message_t), MEM_WAIT );
	}
	
	//writing message in allocated memory
	
	//copy the msg
	for(i =0 ; i < message_len; ++i){
		message_str[i] = message_ptr[i];
	}
	send_msg->size = message_len;
	send_msg->data = message_str;
	send_msg->priority = message_priority;
	send_msg->next_message = NULL;
	
	if( current_queue->head == NULL )
	{
		current_queue->head = send_msg;
		
	}
	else
	{	
		prev = current_queue->head;
		next = prev->next_message;
		
		if(message_priority > prev->priority)
		{
			send_msg->next_message = current_queue->head;
			current_queue->head = send_msg;
		}else{
		
			while( next!=NULL )
			{
				if(message_priority > next->priority)
				{
					send_msg->next_message = next;
					prev->next_message = send_msg;
					os_sem_send(current_queue->messages_ready);
					return current_queue->queue_id;
				}
				prev = prev->next_message;
				next = next->next_message;
			}
			
			prev->next_message = send_msg;
			os_sem_send(current_queue->messages_ready);
			return current_queue->queue_id;
		}
	}
	os_sem_send(current_queue->messages_ready);
	return current_queue->queue_id;
}

mqd_t mq_receive(mqd_t queue_descriptor, char *message_ptr, size_t message_len, unsigned* message_priority)
{
	int i =0;
	queue_data_t * current_queue = find_queue(queue_descriptor);
	message_t * current_message;
	if(current_queue == NULL)
	{
		return EBADF;
	}
	
	if((current_queue->flag & O_WRONLY))
	{
		return EBADF;
	}
	if(current_queue->size < message_len){
		return EMSGSIZE;
	}

	current_message = current_queue->head;
	if(current_message == NULL && current_queue->flag & O_NONBLOCK){
		return EAGAIN;
	}
	
	os_sem_wait(current_queue->messages_ready, 0xFFFF);
	current_message = current_queue->head;
	
	for(i = 0; i < current_message->size; ++i){
		message_ptr[i] = current_message->data[i];
	}
	(*message_priority) = current_message->priority;
	current_queue->head = current_message->next_message;
	os_mem_free(current_message);
	return current_queue->queue_id;
}

queue_data_t * find_queue(mqd_t queue_id)
{
	queue_data_t* current_queue = first_queue;
	while(current_queue!= NULL)
	{
		if(current_queue->queue_id == queue_id)
		{
			return current_queue;
		}
		current_queue =  current_queue->next_queue;
	}
	
	return NULL;
}



