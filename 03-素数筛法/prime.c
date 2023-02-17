#include<stdio.h>
#include<stdbool.h>
#include<math.h>
#include<stdlib.h>
#define N 10000

bool IsPrime[N+1];
int main()
{
 int range = N;
 int i,j,k = 0;
 for (i = 0; i <= range; ++i)
 	IsPrime[i] = true;
 IsPrime[0] = IsPrime[1] = false;
 for (i = 2; i <= range; ++i) {
	if (IsPrime[i]) {
            for (j = 2 * i; j <= range; j += i)//从此数的2倍起，及后续倍数都为合数
                IsPrime[j]=false;
	} 
 }

 for(i=2; i<N; i++)
 {
    if(IsPrime[i]) {
	k++;
    	printf("%d ",i);
    }
 }
 printf("range=%d, count=%d, rate=%f%%\n",range,k,(float)k/(float)range*100);
 return 0;
}
