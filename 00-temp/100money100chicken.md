百钱百鸡问题：

公鸡🐓：5文一只，*x*只

母鸡：3文一只，*y*只

小鸡：1文三只，*z*只

可列如下方程组[^1]。：
$$
\begin{cases}
x + y + z = 100 & (1)\\
5x + 3y + z/3 = 100 & (2)\\
x, y, z \geq 0 & (3)
\end{cases}
$$



## 1.使用python枚举法：

```python
def solve_100_chickens_100_money():
    solutions = []
    for cock in range(0, 101):
        for hen in range(0, 101 - cock):
            chick = 100 - cock - hen
            if chick % 3 == 0 and 5 * cock + 3 * hen + chick / 3 == 100:
                solutions.append((cock, hen, chick))
    return solutions

solutions = solve_100_chickens_100_money()

if solutions:
    print("解为：")
    for solution in solutions:
        print(f"公鸡：{solution[0]}，母鸡：{solution[1]}，小鸡：{solution[2]}")
else:
    print("方程无解")

```

> 解为：
> 公鸡：0，母鸡：25，小鸡：75
> 公鸡：4，母鸡：18，小鸡：78
> 公鸡：8，母鸡：11，小鸡：81
> 公鸡：12，母鸡：4，小鸡：84

## 2.使用数值分析

令公式`(1)`x5 - 公式`(2)`并化简得到公式4，令(1)`x3 - 公式`(2)`并化简得到公式5
得:
$$
\begin{cases}
y=200-\frac{7}{3}x &(4) \\
x=\frac{4}{3}z-100 &(5) \\
\end{cases}
$$

另外加上：
$$
\begin{cases}
0 \leq 200-\frac{7}{3}x \leq100 &(4) \\
0 \leq \frac{4}{3}z-100 \leq100 &(5) \\
z=3t, t∈\mathbb{N}
\end{cases}
$$


求解此不等式组：
$$
\begin{cases}
\frac{300}{7} \leq z \leq \frac{600}{7} \\
75 \leq z \leq 150 \\
z=3t, t∈\mathbb{N}
\end{cases}
$$



*z* 的值为75，78，81，84，然后代入公式(4)(5)中，可得：

$$
\begin{bmatrix}
x \\
y \\
z
\end{bmatrix}
=
\begin{pmatrix}
0 & 4 & 8 & 12 	\\
25 & 18 & 11 & 4 \\
75 & 78 & 81 & 84
\end{pmatrix}
$$



[^1]: \tag{3} 可以用来给单个公式加标号，& (1) 可以给多个公式分别加标号


