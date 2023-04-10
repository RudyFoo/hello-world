#include <math.h>
#include <stdio.h>

#include <iostream>
#include <queue>
#define N_max 1000
#define Hout_max 1000
#define Wout_max 1000
#define Ci 3
#define WIN_S 5
#define Stride_X 1
#define BPE 2

using namespace std;
int ceil_my(int nom, int denom) {
  int tmp = (nom + denom - 1) / denom;
  return tmp * denom;
}

int ceil_int64(int nom, int denom) {
  int tmp = (nom + denom - 1) / denom;
  return tmp * denom;
}

int div_ceil(int nom, int denom) {
  int tmp = (nom + denom - 1) / denom;
  return tmp;
}

int get_min(int dat0, int dat1) {
  int min = dat0 < dat1 ? dat0 : dat1;
  return min;
}

int get_max(int dat0, int dat1) {
  int max = dat0 > dat1 ? dat0 : dat1;
  return max;
}

int squence(int N, int Hout, int Wout) {
  printf("N=%d, Hout=%d, Wout=%d\n", N, Hout, Wout);
  std::queue<int> ld_iv_qu;
  std::queue<int> smr_iv_qu;
  int WinC = div_ceil((Wout - 1) * Ci * Stride_X + S * Ci, 64 / BPE);

  for (int idx = 0; idx < N * Hout * Wout; idx++) smr_iv_qu.push(idx % 32);
  for (int idx = 0; idx < N * Hout * WinC; idx++) ld_iv_qu.push(idx % 32);
  int iv_ptr = 0;
  for(int loop1 = 0; loop1 < N * Hout * Wout; loop1++)
  {
    ptr
    printf("load iv%d, svmm {iv%d,iv%d} smr\n ", ld_iv_qu.front(), smr_iv_qu.front());
  }
}
int main() {
  Ci* Stride < 64 / BPE ? printf("\n") : printf("Ci*Stride too large\n");

  for (int n = 1; n <= N_max; n++)
    for (int h = 1; h < Hout_max; h++)
      for (int w = 1; w < Wout_max; w++) {
        if (n * h * w <= 1000) squence(n, h, w);
      }
  return 0;
}