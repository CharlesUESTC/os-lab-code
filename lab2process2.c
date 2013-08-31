#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "lab2my_ipc.h"

int P;	//producer
int C;	//consumer
int N;
int B;
int msgqid;  					//message queue id

short ParseArgs(int, char **);
int SendMessage(int);
int ReceiveMessage(void);
int SignalExit(void);
int CreateQueue(void);
int DestroyQueues(void);

int main (int argc, char **argv)
{
	struct timeval timeslice;
	gettimeofday(&timeslice, NULL);
	double start=timeslice.tv_sec+(timeslice.tv_usec/1000000.0);  

  	if(ParseArgs(argc, argv)==FALSE)
  	{ 	
    	printf("Invalid arguement\n");
		exit(1);
 	}

	CreateQueue();   
	
	//end of init
	gettimeofday(&timeslice, NULL);
	double end_of_init=timeslice.tv_sec+(timeslice.tv_usec/1000000.0);  
	
	int processStatus; 
	pid_t pID = fork();

	if (pID < 0)
	{
		//failed to fork
		printf("failed to fork\n");
	}
	else if(pID > 0)
	{
		//code for parent process
		printf("fork producer is running\n");
		
		pid_t producer_pID;		//declare a positive value to handle when P=1
		int producerStatus;
		
		int producer_num;
		for(producer_num = 0; producer_num<P; ++producer_num)
		{
			producer_pID = fork();
			if(producer_pID < 0)
			{
				printf("Error encountered forking multiple producers!\n");
				exit(1);
			}
			else if(producer_pID == 0)
			{
				//child producers
				printf("Start producer %d\n",producer_num);
				SendMessage(producer_num);
				exit(0);				
			}
		}
		
		if(producer_pID > 0)
		{
			//this is handling signal after sending and receiving all msgs
			printf("Main producer signal handling\n");
			int producer_num2 = 0;
			while (producer_num2!=P) 
			{
				if(wait(&producerStatus)>=0)
				{
					++producer_num2;
				}
			}
			printf("I am exiting from producers!\n"); 
			
			printf("Signal consumers to exit\n");
			int i;
			for(i=0;i<C;i++)
			{
				SignalExit();
			}

			wait(&processStatus);			
			printf("I am exiting from consumers!\n"); 

			DestroyQueue();
			//timing
			gettimeofday(&timeslice, NULL);
			double end=timeslice.tv_sec+(timeslice.tv_usec/1000000.0);  

			printf("Init Time: %lf seconds\n",end_of_init-start);
			printf("Time: %lf seconds\n",end-end_of_init);

			exit(EXIT_SUCCESS);
		}

	}
	else if(pID == 0)
	{	
		//code for child process
		printf("fork consumer is running\n");
		pid_t consumer_pID;
		int consumerStatus;
		int consumer_num;
		for(consumer_num =0; consumer_num<C;++consumer_num)
		{
			consumer_pID = fork();
			if(consumer_pID<0)
			{
				printf("Error encountered! forking multiple consumers\n");
				exit(1);
			}
			else if(consumer_pID == 0)
			{
				printf("Making consumer %d\n",consumer_num);
				ReceiveMessage();
				exit(EXIT_SUCCESS);	
			}
		}
		if (consumer_pID>0)
		{
			int consumer_num2 = 0;
			while (consumer_num2!=C) 
			{
				if(wait(&consumerStatus)>=0)
				{
					++consumer_num2;
				}
			}
			printf("Parent consumer is gonna exit\n");
			exit(EXIT_SUCCESS);	
		}
	}
	
	//code execute for parent and child
  	return 0;
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

int SendMessage(int producer_num)
{
    char str_msg[10];        
	int int_msg = producer_num;
    message_t message;
	struct msqid_ds buffer;	//message queue

	while(1)
	{	
		//access message queue information
		sleep(1);
		if (msgctl(msgqid, IPC_STAT, &buffer) == -1)
		{
		perror("msgctl IPC_STAT failed:");
        	exit(EXIT_FAILURE);
		}
		//printf("Number of messages in the queue: %lu\n", buffer.msg_qnum);
		
		//check if number of messages in the queue is reached B
		if(buffer.msg_qnum<(unsigned long)B)
		{
			sprintf(str_msg,"%d", int_msg);
			message.m_type = T_TEXT; 
			message.m_sender = getpid(); 
			strcpy(message.m_data, str_msg);		//copy message from str_msg

			if (msgsnd(msgqid, (void *) &message, 10, 0 ) == -1)
		    {
		        perror("msgsnd failed:");
		        exit(EXIT_FAILURE);
		    }
			printf("Process %d: The number is %s\n", producer_num, message.m_data);

			int_msg += P;	//increment my message value
			if(int_msg>N)
			{
				break;
			}
		}
    }

	return 0;
}

int ReceiveMessage(void)
{
    message_t message;
    long int msg_type = T_TEXT;

    while(1)
    {
		sleep(1);
        if (msgrcv(msgqid, (void *) &message, 10, msg_type, 0 ) == -1)
        {
            perror("msgrcv failed:");
            exit(EXIT_FAILURE);
        }
		
        printf("Received number: %s\n", message.m_data);
		if(strcmp(message.m_data,"exit")==0)
		{
			break;
		}
    }
	printf("Receiver exits\n");
	return 0;
}

int SignalExit(void)
{
	message_t message;
	message.m_type = T_TEXT; 
	message.m_sender = getpid(); 
	strcpy(message.m_data, "exit");		//exit signal

	if (msgsnd(msgqid, (void *) &message, 10, 0 ) == -1)
	{
		perror("msgsnd failed:");
		exit(EXIT_FAILURE);
	}
	printf("Exit signal has been sent to consumer\n");
	return 0;
}

int CreateQueue(void)
{
	key_t key = ftok (".", FTOK_ID);	//generating a key for message queue
	
    if (key == -1) 
    {
        perror("ftok failed:");
        exit(EXIT_FAILURE);     // EXIT_FAILURE is defined in stdlib.h
    }
	
    // create a message queue with permission of --rw-rw-rw- .
    // If the queue already exists and it was not created using IPC_PRIVATE,
    // then msgget just returns the exisiting message queue id
    msgqid = msgget(key, 0666 | IPC_CREAT);
	
    if (msgqid == -1 ) 
	{
        perror("msgget failed:");
        exit(EXIT_FAILURE);
    }
	
    printf("msgqid=%d\n", msgqid);	//Message queue ID
	return 0;
}

int DestroyQueue(void)
{
	if (msgctl(msgqid, IPC_RMID, 0) == -1)
    {
        perror("msgctl IPC_RMID failed:");
        exit(EXIT_FAILURE);
    }
	
	return 0;
}

