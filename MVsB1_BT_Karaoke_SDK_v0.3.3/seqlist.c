#include "seqlist.h"

void SeqListInit(SeqList *list)
{
	list->capacity = SEQLIST_CAPACITY;
	list->size = 0;
	list->base = (SeqListDataType *)calloc(list->capacity, sizeof(SeqListDataType));

	if(list->base == NULL)
	{
		printf("SeqList Init Error!\n");
	}
	else
	{
		printf("SeqList Init OK!\n");
	}
}

void SeqListShow(SeqList *list)
{
	uint8_t i = 0;

	for(; i<list->size; i++)
	{
		printf("%d、", list->base[i]);
	}
	printf("\n");
}

void SeqListDestory(SeqList *list)
{
    free(list->base);
    list->base = NULL;
    list->size = list->capacity = 0;
}

void SeqListPushBack(SeqList *list, SeqListDataType x)
{
	if(list->size >= list->capacity) return;//不能在尾部插入

	list->base[list->size] = x;//插入数据
	list->size++;
}
void SeqListPopBack(SeqList *list)
{
	if(list->size == 0) return;

	list->size--;
}

void SeqListPushFront(SeqList *list, SeqListDataType x)
{
	if(list->size >= list->capacity) return;

    //挪动数据
    uint8_t end = list->size - 1;//因为数组从0开始
    while(end >= 0)//end表示数组下标，所以第0个数组元素也要挪动
    {
        list->base[end+1] = list->base[end];
        end--;
    }

    list->base[0] = x;
    list->size++;
}
void SeqListPopFront(SeqList *list)
{
    if(list->size == 0) return;
    
    uint8_t begin = 1;
    while(begin < list->size)
    {
        list->base[begin-1] = list->base[begin];
        begin++;
    }

    list->size--;
}

uint8_t SeqListFind(SeqList *list, SeqListDataType x)
{
    uint8_t i;

    for(i=0; i<list->size; i++)
    {
        if(list->base[i] == x)
        {
            return i;
        }
    }

    return -1;
}

void SeqListSort(SeqList *list)//从小到达排序:选择排序法
{
	int i = 0;
	int j = 0;
	int k = 0;

	for(i=0; i<SEQLIST_CAPACITY-1; i++)
	{
		for(j=i+1; j<SEQLIST_CAPACITY; j++)
		{
			if(list->base[i] > list->base[j])
			{
				k = list->base[i];
				list->base[i] = list->base[j];
				list->base[j] = k;
			}
		}
	}
}

void SeqListInsert(SeqList *list, uint8_t pos, SeqListDataType x)
{
    if(list->size >= list->capacity)  return;//已满

    uint8_t end = list->size - 1;

    while(end >= pos)
    {
        list->base[end + 1] = list->base[end];
        end--;
    }

    list->base[pos] = x;
    list->size++;;
}

void SeqListErase(SeqList *list, uint8_t pos)
{
    if(pos>=list->size) return;

    uint8_t begin = pos + 1;

    while(begin < list->size)
    {
        list->base[begin-1] = list->base[begin];
        begin++;
    }

    list->size--;
}

void SeqListPopMax(SeqList *list)
{
    if(list->size <= 0) return;

    uint8_t i;
    SeqListDataType max      = 0; //存放最大值
    uint8_t       max_index = 0; //存放最大值下标

    for(i=0; i<list->size; i++)
    {
        if(list->base[i] > max)
        {
            max       = list->base[i];
            max_index = i;
        }
    }

    //已经确定好最大值下标
    SeqListErase(list, max_index);
}

void SeqListPopMin(SeqList *list)
{
    if(list->size <= 0) return;

    uint8_t i;
    SeqListDataType min      = 0xFFFF; //存放最小值
    uint8_t       min_index = 0; //存放最小值下标

    for(i=0; i<list->size; i++)
    {
        if(list->base[i] < min)
        {
            min       = list->base[i];
            min_index = i;
        }
    }

    //已经确定好最小值下标
    SeqListErase(list, min_index);
}

