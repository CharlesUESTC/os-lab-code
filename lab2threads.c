#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#define FALSE 0
#define TRUE  1

int P;	//producer
int C;	//consumer
int N;
int B;
int* msgq_pointer;			//point to current sending position
int* msgq_end;				//point	to end of msgq
int* msg_receive_pointer;	//point to current receiving position
int received_msg_num;		//number of received messages
pthread_mutex_t msg_mutex;	//lock
sem_t msg_count;
pthread_t *thread_cancel;

short ParseArgs(int, char **);
void* SendMessages(void*);
void* ReceiveMessages(void*);

int main(int argc, char **argv)
{
	if(ParseArgs(argc, argv)==FALSE)
  	{ 	
    	printf("Invalid arguement\n");
		exit(1);
 	}
	
	//initialize message buffer
	int * msgq = (int*) malloc(B * sizeof(int));
	if (msgq==NULL) 
	{
		exit (1);
	}
	memset(msgq, -1, B-1);
	
	pthread_mutex_init(&msg_mutex, NULL);	//initialize lock
	sem_init(&msg_count, 0, 0);				//initialize sem
	
	//initialize pointers
	msgq_pointer = msgq;					
	msg_receive_pointer = msgq;
	msgq_end = &msgq[B-1];	
	received_msg_num = 0;
	printf("msg begins from %p to %p\n",msgq_pointer,msgq_end);

	pthread_t send_thread[P];
	int send_arg[P];
	int send_counter;
	for(send_counter=0;send_counter<P;++send_counter)
	{
		//create thread referring to pthread_create sample
		//printf("WTF with P value %d\n", P);
		send_arg[send_counter] = send_counter;
		pthread_create(&(send_thread[send_counter]), NULL, &SendMessages, (void*)(&send_arg[send_counter]));
	}
	
	pthread_t receive_thread[C];
	//points to array of receive_thread, it is used later to cancel
	//other receive threads once one thread receives the last msg
	thread_cancel =  receive_thread;
	int receive_arg[C];		 
	int receive_counter;
	for(receive_counter=0;receive_counter<C;++receive_counter)
	{	
		receive_arg[receive_counter] = receive_counter;
		pthread_create(&(receive_thread[receive_counter]), NULL, &ReceiveMessages, &receive_arg[receive_counter]);
	}
	
	//wait for producer to finish
	for(send_counter=0;send_counter<P;++send_counter)
	{
		pthread_join(send_thread[send_counter],NULL);
	}

	//wait for receiver to finish
	for(receive_counter=0;receive_counter<C;++receive_counter)
	{	
		pthread_join(receive_thread[receive_counter],NULL);	//one receiver finishes
	}
	
}

//Parse input arguements
short ParseArgs(int argc, char **argv)
{
	if(argc!=5)
	{
		printf("This program takes %d arguements\n",argc);
    	return FALSE;
	}

	P = atoi(argv[1]);
  	printf("P (number of producers) is %d\n",P);

  	C = atoi(argv[2]);
  	printf("C (number of consumers) is %d\n",C);
	
  	N = atoi(argv[3]);
  	printf("N (number of messages) is %d\n",N);

  	B = atoi(argv[4]);
  	printf("B (capacity) is %d\n",B);

  	if(N>B)
  	{
    	return TRUE;
  	}
  	else
  	{
    	return FALSE;
  	}
}

void* SendMessages(void* arg)
{
	printf("Send Thread %d\n",*(int*)arg);

	int int_msg = *(int*)arg;
	while(1)
	{	
		sleep(1);
		pthread_mutex_lock(&msg_mutex);
		if(*msgq_pointer == -1)
		{			
			*msgq_pointer = int_msg;
			printf("send msg is %d\n",*msgq_pointer);
			//printf("msgqpointer %p\n", msgq_pointer);
			msgq_pointer++;
			if(msgq_pointer>msgq_end)
			{
				msgq_pointer -= B;
			}
			
			int_msg += P;
			sem_post(&msg_count);
			
			if(int_msg >= N)
			{
				pthread_mutex_unlock (&msg_mutex);
				break;	
			}		
		}
		pthread_mutex_unlock (&msg_mutex);	
	}
	return;
}

void* ReceiveMessages(void* arg)
{	
	int msg;
	int receiveID = 0;
	int this_receiveID = *(int*)arg;
	while(1)
	{
		sleep(1);
		//printf("waiting for this motherfucker\n");
		sem_wait(&msg_count);
		pthread_mutex_lock(&msg_mutex);
		if(*msg_receive_pointer!=-1)
		{
			msg = *msg_receive_pointer;
			printf("Received msg: %d\n",msg);
			//printf("Address of received: %p\n",msg_receive_pointer);
			*msg_receive_pointer = -1;

			++msg_receive_pointer;
			if(msg_receive_pointer > msgq_end)
			{
				msg_receive_pointer -= B;
			}

			++received_msg_num;
			if(received_msg_num == N)
			{
				for(receiveID=0;receiveID < C;++receiveID)
				{
					if(receiveID != this_receiveID)
					{
						pthread_cancel(*(thread_cancel+receiveID));
						printf("Canceling receiver %d\n",receiveID);
					}
				}
				pthread_mutex_unlock (&msg_mutex);
				break;
			}
		}
		pthread_mutex_unlock (&msg_mutex);	
	}
	return;
}


