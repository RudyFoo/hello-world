#include <stdio.h>  
#include <stdlib.h>  
#include <string.h> 
typedef struct
{
    int id;
    double score;
} cls_score;
static void sort_cls_score(cls_score* array, int left, int right)
{
    int i = left;
    int j = right;
    cls_score key;

    if (left >= right)
        return;

    memmove(&key, &array[left], sizeof(cls_score));
    while (left < right)
    {
        while (left < right && key.score >= array[right].score)
        {
            --right;
        }
        memmove(&array[left], &array[right], sizeof(cls_score));
        while (left < right && key.score <= array[left].score)
        {
            ++left;
        }
        memmove(&array[right], &array[left], sizeof(cls_score));
    }

    memmove(&array[left], &key, sizeof(cls_score));

    sort_cls_score(array, i, left - 1);
    sort_cls_score(array, left + 1, j);
}

void print_topk(double* data, int total_num, int topk)
{
    cls_score* cls_scores = ( cls_score* )malloc(total_num * sizeof(cls_score));
    for (int i = 0; i < total_num; i++)
    {
        cls_scores[i].id = i;
        cls_scores[i].score = data[i];
	printf("cls_scores[%d].score=%f\n",i,data[i]);
    }

    sort_cls_score(cls_scores, 0, total_num - 1);
    char strline[128] = "";
    for (int i = 0; i < topk; i++)
    {
        fprintf(stderr, "%f, %d\n", cls_scores[i].score, cls_scores[i].id);
    }
    free(cls_scores);
}

int main(int argc,char* argv[])
{
	char line_str[64] = "";
	int index = 0;
	char* filename=argv[1];
	FILE * stream = fopen(filename,"r");
	if(stream == NULL)
	{
		printf("open file failed\n");
		return 0;
	}
	while(NULL != fgets(line_str, 64, stream)){
		index++;
    }
	int bufsize = index;   
    double* buff = (double*)malloc(bufsize*sizeof(double));
	
	rewind(stream);   // 回到文件最开头
	index = 0;
    // read lines  
    while(NULL != fgets(line_str, 64, stream)){
        //printf("Read line with len: %d\n", strlen(line_str));  
        //printf("line=%d,double=%f,string=%s", index,strtod(line_str,NULL),line_str);
		buff[index++] = strtod(line_str,NULL);//
    }  
	print_topk(buff,  index, 5);
    // free buff
    free(buff);
	fclose(stream);
}
