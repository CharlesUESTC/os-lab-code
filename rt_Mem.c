/*----------------------------------------------------------------------------
 *      Name:    RT_MEM.C
 *      Purpose: Interface functions for dynamic memory block management system
 *      NOTES:   Template file for ECE254 Lab3 Assignment
 *----------------------------------------------------------------------------*/

/** 
  * @author: PUT YOUR GROUP ID and NAME HERE
  */

/* The following are some header files you may need,
   feel free to add more or remove some 
   */

   
#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include <stdio.h>
#include <stdlib.h>
#include "Serial.h"
#include "LED.h"
//include "RTL.h"
   
   
#include "rt_TypeDef_mod.h"
#include "RTX_Config.h"
#include "rt_System.h"
#include "rt_MemBox.h"
#include "rt_HAL_CM.h"
#include "rt_Task.h"

#include "rt_Mem.h"
#include "rt_Semaphore.h"
#include "rt_Time.h"
#include "rt_List.h"



extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;  // symbol defined in the scatter file

#define MEM_END 0x10008000

#define MAX_MEM_POOL_SIZE 14
#define BLOCK_SIZE 10
#define MAX_TREE_HEIGHT (MAX_MEM_POOL_SIZE - BLOCK_SIZE + 1)

unsigned MEM_POOL_SIZE;
unsigned TREE_HEIGHT;

int isInit = 0;
char * memPool;

signed char memTree[1 << MAX_TREE_HEIGHT];
OS_TID blocked_proc[16];
int numBlocked = 0;

#define TREE_NO_NODE -1
#define TREE_FREE 0
#define TREE_IN_USE 1
#define TREE_CHILD_IN_USE 2

#define ROOT_NODE 1
/* Definition of Semaphore type */
typedef U32 OS_SEM[2];


/*--------------------------- rt_mem_init -------------------------------------*/
/** 
 * @brief: initialize the free memory region for dynamic memory allocation
 *         For example buddy system
 * @return: 0 on success and -1 otherwise
 * NOTE: You are allowed to change the signature of this function.
 *       You may also need to extern this function 
 */

int rt_mem_init (void) 
{
	int i;
	memPool = (char *) &Image$$RW_IRAM1$$ZI$$Limit;
	MEM_POOL_SIZE = prevPowerOfTwo(MEM_END - ((unsigned)memPool));
	TREE_HEIGHT  = (MEM_POOL_SIZE - BLOCK_SIZE + 1);
	
	for(i = 0; i < (1 << TREE_HEIGHT); ++i)
	{
		memTree[i] = TREE_NO_NODE;
	}
	memTree[ROOT_NODE] = TREE_FREE;
	isInit = 1;
	return (0);
}

/*--------------------------- rt_mem_alloc ----------------------------------*/

void * rt_mem_alloc (int size_bytes, unsigned char flag) 
{
	unsigned size;
	int treeIndex, tempIndex;
	if(!isInit)
	{
		rt_mem_init();
	}

	size = nextPowerOfTwo(size_bytes);
	treeIndex = check_for_free_node(ROOT_NODE, size);
	if(treeIndex == -1)
	{
		#if DEBUG
		printf("   FAILED TO FIND MEM\n");
		printf("   Return: 0x%X\n", NULL);
		#endif
		if(flag == MEM_WAIT)
		{
			#if DEBUG
			printf("   EXECUTING rt_block\n");
			#endif
			
			blocked_proc[numBlocked] = rt_tsk_self();
			numBlocked++;
			rt_block(0xFFFF,INACTIVE);
		}
		return NULL;
	}
	memTree[treeIndex] = TREE_IN_USE;
	tempIndex = getParent(treeIndex);
	while(tempIndex >= ROOT_NODE && memTree[tempIndex] == TREE_FREE)
	{
		memTree[tempIndex] = TREE_CHILD_IN_USE;
		tempIndex = getParent(tempIndex);
	}
	#if DEBUG
	printf("   MemPool: 0x%X\n", memPool);
	printf("   PoolSize: 0x%X\n", MEM_POOL_SIZE);
	printf("   Index: 0x%X\n", treeIndex);
	printf("   Return: 0x%X\n", ((treeIndex * (1 << (MEM_POOL_SIZE - prevPowerOfTwo(treeIndex))))  -  (1 << MEM_POOL_SIZE)  ) + memPool);
	printTree();
	#endif
	return ((treeIndex * (1 << (MEM_POOL_SIZE - prevPowerOfTwo(treeIndex))))  -  (1 << MEM_POOL_SIZE)  ) + memPool;
}

int check_for_free_node (unsigned node, int size) 
{
	int temp_node;
	if(size == getSize(node) || isBottom(node))
	{
		switch (memTree[node])
		{
			case TREE_FREE:
				return node;
			case TREE_IN_USE:
			case TREE_CHILD_IN_USE:
				return -1;
			default:
				//ERROR!!!! shouldnt happen
				break;
		}
	}
	else
	{
		switch (memTree[node])
		{
			case TREE_FREE:
				memTree[getLeftChild(node)] = TREE_FREE;
				memTree[getRightChild(node)] = TREE_FREE;
				return(check_for_free_node(getLeftChild(node), size));
			case TREE_IN_USE:
				return -1;
			case TREE_CHILD_IN_USE:
				temp_node = check_for_free_node(getLeftChild(node), size);
				if(temp_node != -1){
					return temp_node;
				}
				return check_for_free_node(getRightChild(node), size);
				
			default:
				//ERROR!!!! shouldnt happen
				break;
		}
	
	}
	return -2;
}


/*--------------------------- rt_mem_free -----------------------------------*/

/**
 * @brief: free memory pointed by ptr
 * @return: 0 on success and -1 otherwise.
 */
int rt_mem_free(void * ptr) 
{
	int i;
	unsigned treeIndex;
	P_TCB temp_task;

	
	if(!isInit)
	{
		return -1;
	}
	if(((unsigned)ptr) < ((unsigned)memPool))
	{
		return -1;
	}
	
	treeIndex = (((unsigned)ptr) - ((unsigned)memPool)) >> BLOCK_SIZE;
	treeIndex += (1 << (TREE_HEIGHT -1));
	#if DEBUG
	printf("   treeIndex %u\n", treeIndex);
	#endif
	while(memTree[treeIndex] == TREE_NO_NODE)
	{
		treeIndex = getParent(treeIndex);
	}
	#if DEBUG
	printf("   new treeIndex %u\n", treeIndex);
	#endif
	if(memTree[treeIndex] == TREE_FREE || memTree[treeIndex] == TREE_CHILD_IN_USE)
	{
		return -1;
	}
	
	memTree[treeIndex] = TREE_FREE;
	treeIndex = getParent(treeIndex);
	
	
	while(memTree[getLeftChild(treeIndex)] == TREE_FREE && memTree[getRightChild(treeIndex)] == TREE_FREE && treeIndex >= ROOT_NODE)
	{
		
		memTree[getLeftChild(treeIndex)] = TREE_NO_NODE;
		memTree[getRightChild(treeIndex)] = TREE_NO_NODE;
		memTree[treeIndex] = TREE_FREE;
		treeIndex = getParent(treeIndex);
	}
	#if DEBUG
	printTree();
	#endif
	#if BLOCKING
	for(i = 0; i < numBlocked; ++i)
	{
		temp_task = os_active_TCB[blocked_proc[i] - 1];
		temp_task->state = READY;
		rt_put_prio (&os_rdy, temp_task);
	}
	numBlocked = 0;
	#endif
	return (0);
}

unsigned getParent(unsigned node)
{
	return node >> 1;
}

unsigned getLeftChild(unsigned node)
{
	if((node << 1) < (1 << TREE_HEIGHT))
	{ 
		return (node << 1);
	}
	return 0;
}

unsigned getRightChild(unsigned node)
{
	if((node << 1) + 1 < (1 << TREE_HEIGHT))
	{ 
		return (node << 1) + 1;
	}
	return 0;
}

unsigned getSize(unsigned node)
{
	return MEM_POOL_SIZE - prevPowerOfTwo(node);
}

unsigned isBottom(unsigned node)
{
	return (prevPowerOfTwo(node) + 1 == TREE_HEIGHT);
}

unsigned nextPowerOfTwo(unsigned num)
{
	unsigned temp = num - 1;
	int i;
	if(temp == 0)
	{
		return 0;
	}
	for(i = 0; temp != 0; ++i)
	{
		temp >>= 1;
	}
	return i;
}

unsigned prevPowerOfTwo(unsigned num)
{
	unsigned temp = num;
	int i;
	if(temp == 0)
	{
		return 0;
	}
	for(i = 0; temp != 0; ++i)
	{
		temp >>= 1;
	}
	return i - 1;
}

void printTree(void){
	int i, j;
	printf("   ");
	for(i = 1; i < 1 << TREE_HEIGHT; ++i)
	{
		for(j = 0; j < (1 << (TREE_HEIGHT - prevPowerOfTwo(i) - 1)) -1; ++j)
		{
			printf(" ");
		}
		
		switch (memTree[i])
		{
			case TREE_NO_NODE:
				printf(". ");
				break;
			case TREE_FREE:
				printf("F ");
				break;
			case TREE_IN_USE:
				printf("U ");
				break;
			case TREE_CHILD_IN_USE:
				printf("C ");
				break;
			default:
				break;
		}
		for(j = 0; j < (1 << (TREE_HEIGHT - prevPowerOfTwo(i) - 1)) -1; ++j)
		{
			printf(" ");
		}
		
		if(prevPowerOfTwo(i) != prevPowerOfTwo(i + 1))
		{
			printf("\n   ");
		}
		
	}
	printf("\n");
}
/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
