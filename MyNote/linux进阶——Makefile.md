# linux进阶——Makefile

[TOC]



## 1.概念

Makefle就定义了一套规则，决定了哪些文件要先编译，哪些文件后编译，哪些文件要重新编译。

整个工程通常只要一个make命令就可以完成编译、链接，甚至更复杂的功能。可以说，任何一个Linux源程序都带有一个Makefile文件。

## 2.优点

1）管理代码的编译，决定该编译什么文件，编译顺序，以及是否需要重新编译；

2）节省编译时间。如果文件有更改，只需重新编译此文件即可，无需重新编译整个工程；

3）一劳永逸。Makefile通常只需编写一次，后期就不用过多更改。

## 3.命名规则

一般来说将Makefile命名为Makefile或makefile都可以，但很多源文件的名字是小写的，所以更多程序员采用的是Makefile的名字，因为这样可以将Makefile居前显示。

如果将Makefile命为其它名字，比如Makefile_demo，也是允许的，但使用的时候应该采用以下方式：

```makefile
make -f Makefile_demo
```

## 4.基本规则

Makefile的基本规则为：

目标：依赖

(tab)规则

目标 --> 需要生成的目标文件

依赖 --> 生成该目标所需的一些文件

规则 --> 由依赖文件生成目标文件的手段

tab --> 每条规则必须以tab开头，使用空格不行

例如我们经常写的gcc test.c -o test，使用Makefile可以写成

```makefile
test: test.c
    gcc test.c -o test
```


其中，第一行中的test就是要生成的目标，test.c就是依赖，第二行就是由test.c生成test的规则。

Makefile中有时会有多个目标，但Makefile会将第一个目标定为终极目标。

## 5.工作原理

目标的生成：

a. 检查规则中的依赖文件是否存在；

b. 若依赖文件不存在，则寻找是否有规则用来生成该依赖文件。

![img](https://img-blog.csdnimg.cn/20210513195446959.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3d3d19kb25n,size_16,color_FFFFFF,t_70)

比如上图中，生成calculator的规则是`gcc main.o add.o sub.o mul.o div.o -o`，Makefile会先检查`main.o`, `add.o`, `sub.o`, `mul.o`, `div.o`是否存在，如果不存在，就会再寻找是否有规则可以生成该依赖文件。

比如缺少了main.o这个依赖，Makefile就会在下面寻找是否有规则生成main.o。当它发现gcc main.c -o main.o这条规则可以生成main.o时，它就利用此规则生成main.o，然后再生成终极目标calculator。

整个过程是向下寻找依赖，再向上执行命令，生成终极目标。

目标的更新：

a. 检查目标的所有依赖，任何一个依赖有更新时，就重新生成目标；

b. 目标文件比依赖文件时间晚，则需要更新。

![img](https://img-blog.csdnimg.cn/20210513195556332.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3d3d19kb25n,size_16,color_FFFFFF,t_70)

比如，修改了main.c，则main.o目标会被重新编译，当main.o更新时，终极目标calculator也会被重新编译。其它文件的更新也是类推。

## 6.命令执行

make:

使用此命令即可按预定的规则生成目标文件。

如果Makefile文件的名字不为Makefile或makefile，则应加上-f选项，比如：

```makefile
make -f Makefile_demo
```

`make clean`:

清除编译过程中产生的中间文件（.o文件）及最终目标文件。

如果当前目录下存在名为clean的文件，则该命令不执行。

-->解决办法：伪目标声明：

```makefile
.PHONY:clean
```



特殊符号：

`-` : 表示此命令即使执行出错，也依然继续执行后续命令。如：

```makefile
-rm a.o build/
```

`@`：表示该命令只执行，不回显。一般规则执行时会在终端打印出正在执行的规则，而加上此符号后将只执行命令，不回显执行的规则。如：

```makefile
@echo $(SOURCE)
```


## 7.普通变量

变量定义及赋值：

变量直接采用赋值的方法即可完成定义，如：

```makefile
INCLUDE = ./include/
```


变量取值：

用括号括起来再加个美元符，如：

```makefile
FOO = $(OBJ)
```

系统自带变量：

通常都是大写，比如`CC`，`PWD`，`CFLAG`，等等。

有些有默认值，有些没有。比如常见的几个：

`CPPFLAGS` : 预处理器需要的选项 如：-I
`CFLAGS`：编译的时候使用的参数 –Wall –g -c
`LDFLAGS` ：链接库使用的选项 –L -l
变量的默认值可以修改，比如CC默认值是cc，但可以修改为gcc：CC=gcc



### **设置软件版本号**

```c
//文件名 main.c
#include<stdio.h>
int main()
{
  printf("version=%s,%d\n",GCUNAME, NUM);
}
```

 

```makefile
#文件名Makfile
CFLAGS += -DGCUNAME=\"$(ver1)\" -DNUM=$(num1)
a.out:main.c
	gcc main.c $(CFLAGS) -o a.out
.PHONY:clean
clean:
	rm -rf *.o a.out
	rm -rf $(TARGET)
```

执行：

```bash
make ver1="gcu200" num1=123 && ./a.out 
```

显示：

> gcc main.c -DGCUNAME=\"gcu200\" -o a.out
>
> version=gcu200,123



## 8.自动变量

### 常用自动变量：

Makefile提供了很多自动变量，但常用的为以下三个。这些自动变量只能在规则中的命令中使用，其它地方使用都不行。

`$@` --> 规则中的目标

`$<` --> 规则中的第一个依赖条件

`$^` --> 规则中的所有依赖条件

例如：

```makefile
app: main.c func1.c fun2.c
    gcc $^ - o $@
```


其中：`$^`表示`main.c func1.c fun2.c`，`$<`表示`main.c`，`$@`表示`app`。

### 模式规则：

模式规则是在目标及依赖条件中使用%来匹配对应的文件，比如在目录下有main.c, func1.c, func2.c三个文件，对这三个文件的编译可以由一条规则完成：

```makefile
%.o:%.c
    $(CC) –c  $< -o $@
```


这条模式规则表示：

    main.o由main.c生成，
    func1.o由func1.c生成，
    func2.o由func2.c生成

这就是模式规则的作用，可以一次匹配目录下的所有文件。

## 9.函数

makefile也为我们提供了大量的函数，同样经常使用到的函数为以下两个。需要注意的是，makefile中所有的函数必须都有返回值。在以下的例子中，假如目录下有main.c，func1.c，func2.c三个文件。

`wildcard`:

用于查找指定目录下指定类型的文件，跟的参数就是目录+文件类型，比如：

```makefile
src = $（wildcard ./src/*.c)
```


这句话表示：找到./src 目录下所有后缀为.c的文件，并赋给变量src。

命令执行完成后，src的值为：main.c func1.c fun2.c。

`patsubst`:

匹配替换，例如以下例子，用于从src目录中找到所有.c 结尾的文件，并将其替换为.o文件，并赋值给obj。

```makefile
obj = $(patsubst %.c ,%.o ,$(src))
```


把src变量中所有后缀为.c的文件替换成.o。

命令执行完成后，obj的值为main.o func1.o func2.o

特别地，如果要把所有.o文件放在obj目录下，可用以下方法：

```makefile
ob = $(patsubst ./src/%.c, ./obj/%.o, $(src))
```


原文链接：https://blog.csdn.net/www_dong/article/details/116763349

## 10.实践篇

### 1.源代码

```c
//main.c  
int main()  
{  
    printf("hello world\n");  
    fun1();  
    fun2();  
}  
//fun1.c  
void fun1()  
{  
    printf("this is fun1\n");  
}  
//fun2.c  
void fun2()  
{  
    printf("this is fun2\n");  
}  
```

### 2.第一版Makefile

对于我们的示例代码，不通过Makefile编译其实也很简单：

```makefile
gcc main.c fun1.c fun2.c -o app
```


我们知道，Makefile其实就是按规则一条条的执行。所以，我们完全可以把上面那条命令写成Makefile的一个规则。我们的目标是app，按此写法依赖是main.c fun1.c fun2.c，则最终的Makefile如下：

```makefile
app: main.c fun1.c fun2.c  
    gcc main.c fun1.c fun2.c -o app  
```

但这个版本的Makefile有两个很重要的不足：

对于简单代码还好，而对于大型项目，具有成千上万代码来说，仅用一行规则是完全不够的，即使够的话也需要写很长的一条规则；

任何文件只要稍微做了修改就需要整个项目完整的重要编译。

基于此，我们在第一版的基础上优化出第二版。

### 3.第二版Makefile

在第二版Makefile中，为了避免改动任何代码就需要重新编译整个项目的问题，我们将主规则的各个依赖替换成各自的中间文件，即main.c --> main.o，fun1.c --> fun1.o，fun2.c --> fun2.o，再对每个中间文件的生成各自写条规则比如对于main.o，规则为：

```makefile
main.o: main.c  
    gcc -c main.c -o main.o 
```


这样做的好处是，当有一个文件发生改动时，只需重新编译此文件即可，而无需重新编译整个项目。完整Makefile如下：

```makefile
app: main.o fun1.o fun2.o  
    gcc main.o fun1.o fun2.o -o app  

main.o: main.c  
    gcc -c main.c -o main.o  

fun1.o: fun1.c  
    gcc -c fun1.c -o fun1.o  

fun2.o: fun2.c  
    gcc -c fun2.c -o fun2.o  
```

第二版Makefile同样具有一些缺陷：

里面存在一些重复的内容，可以考虑用变量代替；

后面三条规则非常类似，可以考虑用一条模式规则代替。

基于此，我们在第二版的基础上优化出第三版。

### 4.第三版Makefile

在第三版Makefile中，我们使用变量及模式规则使Makefile更加简洁。使用的三个变量如下：

```makefile
obj = main.o fun1.o fun2.o  
target = app  
CC = gcc  
```

使用的模式规则为：

```makefile
%.o: %.c  
        $(CC) -c $< -o $@  
```

这条模式规则表示：所有的.o文件都由对应的.c文件生成。在规则里，我们又看到了两个自动变量：`$<`和`$@`。其实自动变量有很多，常用的有三个：

```makefile
    $<：第一个依赖文件；

    $@：目标；

    $^：所有不重复的依赖文件，以空格分开
```

```makefile
obj = main.o fun1.o fun2.o  
target = app  
CC = gcc  

$(target): $(obj)  
    $(CC) $(obj) -o $(target)  

%.o: %.c  
    $(CC) -c $< -o $@ 
```

第三版Makefile依然存在一些缺陷：

obj对应的文件需要一个个输入，工作量大；

文件数目比较少时还好，文件数目一旦很多的话，obj将很长；

而且每增加/删除一个文件，都需要修改Makefile。

基于此，我们在第二版的基础上优化出第四版。

### 5.第四版Makefile

在第四版Makefile中，我们隆重推出了两个函数：`wildcard`和`patsubst`。

`wildcard`：

扩展通配符，搜索指定文件。在此我们使用`src = $(wildcard ./*.c)`，代表在当前目录下搜索所有的.c文件，并赋值给src。函数执行结束后，`src`的值为：`main.c fun1.c fun2.c`。

`patsubst`：

替换通配符，按指定规则做替换。在此我们使用`obj = $(patsubst %.c, %.o`, `$(src))`，代表将src里的每个文件都由.c替换成.o。函数执行结束后，obj的值为main.o fun1.o fun2.o，其实跟第三版Makefile的obj值一模一样，只不过在这里它更智能一些，也更灵活。

除了使用`patsubst`函数外，我们也可以使用模式规则达到同样的效果，比如：`obj = $(src:%.c=%.o)`，也是代表将src里的每个文件都由.c替换成.o。

几乎每个Makefile里都会有一个伪目标clean，这样我们通过执行make clean命令就是将中间文件如.o文件及目标文件全部删除，留下干净的空间。一般是如下写法：

```makefile
PHONY: clean  
clean:  
        rm -rf $(obj) $(target)  
```

`.PHONY`代表声明`clean`是一个伪目标，这样每次执行`make clean`时，下面的规则都会被执行。

```makefile
src = $(wildcard ./*.c)  
obj = $(patsubst %.c, %.o, $(src))  
#obj = $(src:%.c=%.o)  
target = app  
CC = gcc  

$(target): $(obj)  
    $(CC) $(obj) -o $(target)  

%.o: %.c  
    $(CC) -c $< -o $@  

.PHONY: clean  
clean:  
    rm -rf $(obj) $(target) 
```


原文链接：https://blog.csdn.net/www_dong/article/details/116765394
