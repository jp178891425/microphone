#ifndef _SEQLIST_H_
#define _SEQLIST_H_

#include "type.h"
#include <stdlib.h>
#include <stdio.h>

#define SEQLIST_CAPACITY 	9
#define MIN_IGNORE_NUM		3//去掉几个最小值
#define MAX_IGNORE_NUM		3//去掉几个最大值

typedef uint16_t SeqListDataType;
typedef struct
{
	SeqListDataType *base;	//指向首地址的指针
	uint8_t capacity;		//数组的容量
	uint8_t size;			//当前元素数量 
}SeqList;

void SeqListInit(SeqList *list);
void SeqListDestory(SeqList *list);
void SeqListShow(SeqList *list);

void SeqListPushBack(SeqList *list, SeqListDataType x);
void SeqListPopBack(SeqList *list);
void SeqListPushFront(SeqList *list, SeqListDataType x);
void SeqListPopFront(SeqList *list);

void SeqListInsert(SeqList *list, uint8_t pos, SeqListDataType x);
void SeqListErase(SeqList *list, uint8_t pos);

void SeqListSort(SeqList *list);

void SeqListPopMax(SeqList *list);
void SeqListPopMin(SeqList *list);

#endif
