#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>

#define OVECCOUNT 30 // 匹配缓存的大小

unsigned int hex2int(char *phex)
{
    int leng = strlen(phex);
    // printf("leng=%d\n", leng);
    int len = 0;
    unsigned ret = 0;
    // printf("hex:a==0x%s\r\n",phex);
    do{
        if(phex[len] >= '0' && phex[len] <= '9')
            ret += (phex[len] - '0') ;
        else if(phex[len] >= 'a' && phex[len] <= 'f')
            ret += (phex[len] - 'a' + 10) ;
        else if(phex[len] >= 'A' && phex[len] <= 'F')
            ret += (phex[len] - 'A' + 10) ;
        else{
            printf("num is need in [0-9a-f]\n");
            return -1;
        }
        if (++len < leng)
        {
            ret *= 16;
        }else{
            break;
        }
    }while(1);
    // printf("int:a==%d\r\n",ret);
    return (ret);
}

/*
 查找文件中正则表达式模式为"(?<=(|, )[0-9a-f]{8}" 的hex数据 并替换它为float的示例代码
 将一个文件中的所有hex数据(如40000000) 转为float数据(即2)
 使用pcre库，因为只有它支持(?<=pattern) 非捕获模式的匹配，编译时需要加选项 -lpcre
*/
int main(int argc, char **argv) {
    FILE *fp_in, *fp_out;
    char *pattern = "(?<=\\(|, )[0-9a-f]{8}"; // 正则表达式模式
    char *replace = ""; // 替换为float
    char *err;
    int err_offset;
    int rc, offset, i;
    int ovector[OVECCOUNT];
    pcre *re;
    const char *error;
    int error_offset;
    int line_num = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    // 打开输入文件
    fp_in = fopen(argv[1], "r");
    if (fp_in == NULL) {
        fprintf(stderr, "Error opening input file.\n");
        return 1;
    }

    // 打开输出文件
    fp_out = fopen(argv[2], "w");
    if (fp_out == NULL) {
        fprintf(stderr, "Error opening output file.\n");
        return 1;
    }

    // 编译正则表达式模式
    re = pcre_compile(pattern, 0, &error, &error_offset, NULL);
    if (re == NULL) {
        fprintf(stderr, "Error compiling pattern: %s\n", error);
        return 1;
    }

    // 逐行读取输入文件
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fp_in) != -1) {
        line_num++;
        offset = 0;
        while (1) {
            // 在每行中搜索匹配项
            rc = pcre_exec(re, NULL, line, strlen(line), offset, 0, ovector, OVECCOUNT);
            if (rc <= 0) break;

            // 输出匹配项之前的内容
            fwrite(line + offset, 1, ovector[0] - offset, fp_out);

            // 将匹配项替换为float
            for (i = ovector[0]; i < ovector[1]; i += 8) {
                char hex_str[9];
                strncpy(hex_str, line + i, 8);
                hex_str[8] = '\0';
                // hex_str为匹配到的内容
                int f32_str = hex2int(hex_str);
                float value = *(float *)&f32_str;
                fprintf(fp_out, "%g", value);
            }

            offset = ovector[1];
        }

        // 输出匹配项之后的内容
        fwrite(line + offset, 1, strlen(line) - offset, fp_out);
    }

    // 释放资源
    free(line);
    pcre_free(re);
    fclose(fp_in);
    fclose(fp_out);

    return 0;
}
