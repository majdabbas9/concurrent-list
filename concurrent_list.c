#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"
#include <math.h>
#define _GNU_SOURCE
typedef enum {
    false, true
} bool;
struct node {
    int value;
    node* next;
    pthread_mutex_t lock;
};
struct list {
    node* head;
    pthread_mutex_t listLock; 
    int max;//max num in the list
};

void print_node(node* node)
{
    if (node)
    {
        printf("%d ", node->value);
    }
}

list* create_list()
{
    list* List = (list*)malloc(sizeof(list));
    List->head = NULL;
    List->max = 0;
    pthread_mutex_init(&(List->listLock), NULL);
    return List;
}

void delete_list(list* list)
{
    if (!list) {//if no list, we don't need to do anyting
        return;
    }
    //getting the list head
    pthread_mutex_lock(&(list->listLock)); // we lock the list lock to prevent others from updating the list head while we getting it
    node* nodeTmp = list->head;
    pthread_mutex_unlock(&(list->listLock));
    if (!nodeTmp) {//if there is no head, just delete the list
        pthread_mutex_destroy(&(list->listLock));
        free(list);
        return;
    }
    
    pthread_mutex_lock(&(nodeTmp->lock));
    node* Nodenext = nodeTmp->next;
    while (nodeTmp->next) {
        pthread_mutex_lock(&(nodeTmp->next->lock));
        Nodenext = nodeTmp->next;
        pthread_mutex_unlock(&(nodeTmp->lock));

        //deleteing the node
        pthread_mutex_destroy(&(nodeTmp->lock));
        free(nodeTmp);
        //end of deleteing the node

        nodeTmp = Nodenext;
    }
    pthread_mutex_unlock(&(nodeTmp->lock));

    //deleteing the node
    pthread_mutex_destroy(&(nodeTmp->lock));//delete the last node on the list
    free(nodeTmp);
    //end of deleteing the node

    // deleteing the list
    pthread_mutex_destroy(&(list->listLock));//destroy the list lock and free it
    free(list);
    //end of deleteing the list
}
void insert_value(list* list, int value)
{

    if (!list)return;//if no list, we don't need to do anyting

    pthread_mutex_lock(&(list->listLock)); // we lock the list lock to prevent others from updating the list head while we getting it
    node* nodeTmp = list->head;//getting the list head
    pthread_mutex_unlock(&(list->listLock));

    /*node creating*/
    node* newNode = (node*)(malloc(sizeof(node)));
    newNode->value = value;
    newNode->next = NULL;
    pthread_mutex_init(&(newNode->lock), NULL);
    /*end of node cearting*/

    pthread_mutex_lock(&(list->listLock));
    if (!nodeTmp) { /* the list is empty we add the node to the head */
        list->head = newNode;
        list->max = value;
        pthread_mutex_unlock(&(list->listLock));
        return;
    }
    pthread_mutex_unlock(&(list->listLock));

    pthread_mutex_lock(&(nodeTmp->lock));
    if (nodeTmp->value > value) // if the node  value is the samllest number in the list hence it should be the head
    {
        newNode->next = nodeTmp;
        list->head = newNode;
        pthread_mutex_unlock(&(nodeTmp->lock));
        return;
    }
    pthread_mutex_unlock(&(nodeTmp->lock));

    pthread_mutex_lock(&(list->head->lock));
    if (value > list->max) // if the node  value is the largest number in the list hence it should be the last node 
    {
        while (nodeTmp->next)
        {
            pthread_mutex_unlock(&(nodeTmp->lock));
            nodeTmp = nodeTmp->next;
            pthread_mutex_lock(&(nodeTmp->lock));
        }
        nodeTmp->next = newNode;
        list->max = value;
        pthread_mutex_unlock(&(nodeTmp->lock));
        return;
    }
    pthread_mutex_unlock(&(list->head->lock));

    node* prev = nodeTmp; // gettint the current node no need for locking becasue there is no contex switch 
    pthread_mutex_lock(&(nodeTmp->lock));
    nodeTmp = nodeTmp->next;
    pthread_mutex_unlock(&(prev->lock));

    pthread_mutex_lock(&(nodeTmp->lock));
    while (nodeTmp->next && nodeTmp->value <= value) {
        prev = nodeTmp;
        nodeTmp = nodeTmp->next;
        pthread_mutex_lock(&(nodeTmp->lock));
        pthread_mutex_unlock(&(prev->lock));
    }
    /* place it in between prev and nodeTmp */
    prev->next = newNode;
    newNode->next = nodeTmp;
    /* successfully added new value, unlock nodeTmp */
    pthread_mutex_unlock(&(nodeTmp->lock));
    return;
}


void remove_value(list* list, int value)
{
    if (!list) {//do nothing if there is no place to remove from
		return;
	}
	pthread_mutex_lock(&(list->listLock));// we lock the list lock to prevent others from updating the list head while we getting it
	if (!(list->head)) {
		//do nothing cause the list is empty, just relese the list locked lock
		pthread_mutex_unlock(&(list->listLock));
		return;
	}
		node* nodeTmp = list->head;//getting the head of the list
		node * prev=NULL;
		pthread_mutex_unlock(&(list->listLock));
		pthread_mutex_lock(&(nodeTmp->lock));//lock the list head mutex
		if(!(nodeTmp->next)) // cheaking if there is only one node in the list
        {
            if(nodeTmp->value!=value) // if the value is not in the list do nothing
            {
                pthread_mutex_unlock(&(nodeTmp->lock));//unlock the list head mutex
                return;
            }
            pthread_mutex_unlock(&(nodeTmp->lock));

            //deleteing the node
            pthread_mutex_destroy(&(nodeTmp->lock));
            free(nodeTmp);
            //end of deleteing the node

            pthread_mutex_lock(&(list->listLock));
            list->head=NULL;
            list->max=0;
            pthread_mutex_unlock(&(list->listLock));
            return;
        }
		while ((nodeTmp->next) && (nodeTmp->next->value < value)) {//this while to get the place of the value of the node we need to delete
			pthread_mutex_lock(&(nodeTmp->next->lock));//
			prev=nodeTmp;
			nodeTmp = nodeTmp->next;//
			pthread_mutex_unlock(&(prev->lock));
		}
		if ((!prev) && (nodeTmp->value == value)) {//if we need to remove the first value in the list

			node* next = nodeTmp->next;
			pthread_mutex_unlock(&(nodeTmp->lock));

            //deleteing the node
			pthread_mutex_destroy(&(nodeTmp->lock));
            free(nodeTmp);
            //end of deleteing the node

			nodeTmp = next;
			pthread_mutex_lock(&(next->lock));
			pthread_mutex_unlock(&(next->lock));
			pthread_mutex_lock(&(list->listLock)); // locking the list lock to prevent other threads from updating it or getting it while we are updating it
			list->head = nodeTmp;//updating the list head to be the new head
			pthread_mutex_unlock(&(list->listLock));
			return;
		}
		 if (!nodeTmp->next || (nodeTmp->next->value > value)) { // cheaking if the value is not in the list if so do nothing
			
			pthread_mutex_unlock(&(nodeTmp->lock));
			return;
		}
		//the node we need to remove is somewhere in the middle of the list(head->next)
			node* nodeRemove = nodeTmp->next;
			pthread_mutex_lock(&(nodeRemove->lock));
			if (nodeRemove->next) {//if nodeRemove isn't the last element in the list
				pthread_mutex_lock(&(nodeRemove->next->lock));
			}
			nodeTmp->next = nodeRemove->next;
			pthread_mutex_unlock(&(nodeTmp->lock));
			if (nodeRemove->next) {//if nodeRemove isn't the last element in the list
				pthread_mutex_unlock(&(nodeRemove->next->lock));
			}
			pthread_mutex_unlock(&(nodeRemove->lock));

            //deleteing the node
            pthread_mutex_destroy(&(nodeRemove->lock));
            free(nodeRemove);
            //end of deleteing the node
            
            return;
}

void print_list(list* list)
{

    if (!list) //do nothing if there is no place to remove from 
    {
        return;
    }

    pthread_mutex_lock(&(list->listLock)); // we lock the list lock to prevent others from updating the list head while we getting it
    node* nodeTmp = list->head;
    node * prev;
    if(!nodeTmp) // if the list is empty just print \n and do nothing
    {
        printf("\n");
        pthread_mutex_unlock(&(list->listLock));
        return;
    }
    pthread_mutex_unlock(&(list->listLock));
    pthread_mutex_lock(&(nodeTmp->lock));
    while (nodeTmp->next)
    {
        pthread_mutex_lock(&(nodeTmp->next->lock));
        print_node(nodeTmp);
        prev=nodeTmp;
        nodeTmp = nodeTmp->next;
        pthread_mutex_unlock(&(prev->lock));
    }
    print_node(nodeTmp);
    pthread_mutex_unlock(&(nodeTmp->lock));
    printf("\n");

}

void count_list(list* list, int (*predicate)(int))
{
    int count = 0;
	if (!list) {//if no list therefore there is 0 items
		printf("%d items were counted\n", count);
		return;
	}
	pthread_mutex_lock(&(list->listLock));// we lock the list lock to prevent others from updating the list head while we getting it
	node* nodeTmp = list->head;
	node * prev;
	if (!nodeTmp) {//if empty list, 0 items were counted for sure!
		printf("%d items were counted\n", count);
		pthread_mutex_unlock(&(list->listLock));
		return;
	}
	pthread_mutex_unlock(&(list->listLock));
	pthread_mutex_lock(&(nodeTmp->lock));
	while (nodeTmp->next) {
		pthread_mutex_lock(&(nodeTmp->next->lock));
		count += predicate(nodeTmp->value);
		prev=nodeTmp;
		nodeTmp = nodeTmp->next;
		pthread_mutex_unlock(&(prev->lock));
	}
	count += predicate(nodeTmp->value);
	pthread_mutex_unlock(&(nodeTmp->lock));

	printf("%d items were counted\n", count);//print number of counted items.

}



