#include<stdio.h>
#include<time.h>
void sort2(int *p,int size )
{//升序
	if(size<=0)
		return;
	int *start=p;
	int	*end=(p+size-1);
	int key=*start;
	while(start<end)
	{
		while(start < end && key < *end)
			end--;
		*start = *end;
		while(start < end && key > *start)
			start++;
		*end = *start;
	}//结束标志是start==end;
	*start=key;
	//for(int i=0;i<size;i++)
	//	printf("%d ",p[i]);
	//printf("\n");
	sort2(p,start-p);
	sort2(start+1,size-1-(start-p));
}
//我自己的版本
void sort(int *p,int size )
{//升序
	if(size<=0)
		return;
	int *start=p;
	int	*end=(p+size-1);
	int num=*start;
	while(start<end)
	{
		if(*start>*end)
		{
			int temp=*start;
			*start=*end;
			*end=temp;
		}
		if(*start!=num)
			start++;
		else
			end--;
	}//结束标志是start==end;
	//for(int i=0;i<size;i++)
	//	printf("%d ",p[i]);
	//printf("\n");
	sort(p,start-p);
	sort(start+1,size-1-(start-p));
}
#define SIZE_NUM 10000
int main()
{
	srand(time(0));
	int size=SIZE_NUM;
	int num1[SIZE_NUM]={};
	int num2[SIZE_NUM]={};
	for(int i=0;i<size;i++)
	{
		num1[i]=rand();
		num2[i]=num1[i];
	}
	clock_t start1=clock();
	sort(num1,size);
	double time1=(clock()-start1)/(double)(1.0);
	
	clock_t start2=clock();
	sort2(num2,size);
	double time2=(clock()-start2)/(double)(1.0);
	
	printf("time1=%lf\ntime2=%lf\n",time1,time2);
}

int main2()
{
	int size=SIZE_NUM;
	int num[SIZE_NUM]={};
	int num2[SIZE_NUM]={};
	int i=0;
	
	clock_t start=clock();
	for(i=0;i<size;i++)
	{
		num[i]=size-i;
		//num2[i]=size;
	}
	for(int i=0;i<1;i++)
		sort(num,size);
	double time=(clock()-start)/(double)(1.0);
	//printf("sort=%lf\n",(clock()-start)/(CLOCKS_PER_SEC));
	
	
	clock_t start1=clock();
	for(i=0;i<size;i++)
	{
		//num[i]=size-i;
		num2[i]=size-i;
	}
	for(int i=0;i<1;i++)
		sort2(num2,size);
	double time2=(clock()-start1)/(CLOCKS_PER_SEC);

	
	for(i=0;i<size;i++)
		printf("%d ",num[i]);
	printf("\n");
	
	for(i=0;i<size;i++)
		printf("%d ",num2[i]);
	printf("\n");
	printf("sort1=%lf\nsort2=%lf\n",time,time2);
	return 0;
}
