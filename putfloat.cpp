/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/10/13 15:48:02 by dnakano           #+#    #+#             */
/*   Updated: 2020/12/19 23:40:55 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
//#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <float.h>
#include <stdint.h>
#include<Windows.h>

#define FLT_FRACSIZE 1100	// 10進数での小数部の最大桁数149 + 計算用余裕1
#define FLT_INTSIZE 400		// 10進数での最大桁数 39

struct          s_ifloat
{
    int8_t     sign;       // 符号1bitを収納
    int16_t     exp;        // 指数部8bitを収納
    uint64_t    frac;       // 仮数部23bitを収納
    int8_t      intpart[FLT_INTSIZE];   // 10進数での整数部を保存
    int8_t      fracpart[FLT_FRACSIZE]; // 10進数での小数部を保存
};

void            array_add(int8_t *a, int8_t *b, int size)
{
    int		i;

    i = size - 1;
    while (i >= 0)
    {
        a[i] += b[i];
        if (a[i] >= 10 && i != 0)
        {
            a[i] -= 10;
            a[i - 1] += 1;
        }
        i--;
    }
}

void            array_divbytwo(int8_t *n, int size)
{
    int		i;

    i = 0;
    while (i < size - 1)
    {
        n[i + 1] += (n[i] % 2) * 10;
        n[i] /= 2;
        i++;
    }
}

void            array_double(int8_t *n, int size)
{
    int		i;

    i = size - 1;
    while(i >= 0)
    {
        n[i] *= 2;
        if (i < size - 1 && n[i + 1] >= 10)
        {
            n[i] += 1;
            n[i + 1] -= 10;
        }
        i--;
    }
}

struct s_ifloat	store_ifloat(double num)
{
    struct s_ifloat	ifloat;
    uint64_t        mem;

    memcpy(&mem, &num, sizeof(uint64_t));
    ifloat.sign = mem >> 63;
    ifloat.exp = mem >> 52;
    ifloat.frac = mem << 12;
    return (ifloat);
}

void			print_ifloat(struct s_ifloat ifloat)
{
    int     i;
    char    c;
    bool    f = TRUE;
    char fff[FLT_FRACSIZE + 1];

    if (ifloat.exp == 2048 && ifloat.frac != 0)  // nanの場合
    {
        fprintf(stdout, "nan");
        return ;
    }
    if (ifloat.sign == 1)                       // 符号ビットが1の場合'-'を出力
        fprintf(stdout, "-");
    if (ifloat.exp == 2048)                      // infの場合
    {
        fprintf(stderr, "inf");
        return ;
    }
    for (i = 0; i < FLT_INTSIZE; i++)           // 整数部の出力
    {
        if (ifloat.intpart[i] == 0 && f) continue;
        f = FALSE;
        c = ifloat.intpart[i] + '0';
        fprintf(stdout, "%c", c);
    }
    if(f==TRUE) fprintf(stdout, "0");
    fprintf(stdout, ".");
    f = TRUE;

    ZeroMemory(fff, sizeof(fff));

    for (i = FLT_FRACSIZE - 1; i >= 0; i--)      // 小数部の出力
    {
        if (ifloat.fracpart[i] == 0 && f) continue;
        f = FALSE;
        fff[i] = ifloat.fracpart[i] + '0';
    }
    if (f == TRUE) fff[0] = '0';
    fprintf(stdout, fff);
}

void			convert_fracpart(struct s_ifloat *ifloat)
{
    int16_t      i;                  // カウンタ
    uint64_t    fracpart_bin;       // fracの整数部分を格納
    int8_t      n[FLT_FRACSIZE];    // 2^(i-1)を格納

    ZeroMemory(ifloat->fracpart, sizeof(ifloat->fracpart));
    if (ifloat->exp >= 52 + 1023 || (ifloat->exp == 0 && ifloat->frac == 0)) // 小数部0の場合
        return ;
    else if (ifloat->exp >= 1023)    // 一部のみ小数部の場合
        fracpart_bin = ifloat->frac << (ifloat->exp - 1023);
    else if (ifloat->exp == 0)      // 非正規数（ケチ表現分bitなし）
        fracpart_bin = ifloat->frac;
    else                            // 全て小数部かつ正規数（ケチ表現分bitを入れる）
        fracpart_bin = ifloat->frac >> 1ull | (1ull << 63ull);
    ZeroMemory(n, sizeof(n));
    n[0] = 5;                       // 2^(-1)(=0.5)からスタート
    for (i = 0; i < (1022 - ifloat->exp); i++)
    {
        array_divbytwo(n, FLT_FRACSIZE);    // あらかじめ指数に合わせて割っておく
    }
    for (i = 0; i < 53; i++)
    {
        if (fracpart_bin & (1ull << (63ull - i))) // bitが立っていればfracpartに足していく
            array_add(ifloat->fracpart, n, FLT_FRACSIZE);
        array_divbytwo(n, FLT_FRACSIZE);
    }
}

void            convert_intpart(struct s_ifloat *ifloat)
{
    int16_t     i;                  // カウンタ
    int         offset;             // fracの整数部分までのoffset
    uint64_t    intpart_bin;        // fracの整数部分を格納
    int8_t      n[FLT_INTSIZE];     // 2^i乗を格納

    ZeroMemory(ifloat->intpart, sizeof(ifloat->intpart));
    if (ifloat->exp < 1023 || ifloat->exp == 2048) // 整数部は0
        return ;
    else if (ifloat->exp < 1023 + 52) // ifloat->fracの一部が整数部
        offset = ifloat->exp - 1023;
    else // ifloat->exp >= 1023 + 52 => ifloat->fracの全てが整数部
        offset = 52;
    if (offset == 0)
        intpart_bin = 1ull << offset;
    else
        intpart_bin = (ifloat->frac >> (64 - offset)) | (1ull << offset);
    ZeroMemory(n, sizeof(n));
    n[FLT_INTSIZE - 1] = 1;     // n=1からスタート(最も右を1の位とする)
    for (i = 0; i < 53; i++)    // 0~24ビットを確認する
    {
        if (intpart_bin & (1ull << i))     // bitが立っていれば2^n乗を足す
            array_add(ifloat->intpart, n, FLT_INTSIZE);
        array_double(n, FLT_INTSIZE);   // nを2倍して次へ
    }
    while (i++ <= ifloat->exp - 1023)    // iがexpより小さければその分2倍していく
        array_double(ifloat->intpart, FLT_INTSIZE);
}

void            convert_ifloat(struct s_ifloat *ifloat)
{
    convert_intpart(ifloat);        // 整数部の出力
    convert_fracpart(ifloat);       // 小数部の出力
}

void            printfloat(double num)
{
    struct s_ifloat ifloat;

    ifloat = store_ifloat(num);     // 符号部、指数部、仮数部を別々の変数に取り出す
    convert_ifloat(&ifloat);        // 2進数->10進数へ変換
    print_ifloat(ifloat);           // 標準出力へ出力
}

void            printcomp(double num)
{
    char s[FLT_FRACSIZE + FLT_INTSIZE];
    char* ss;

    // printfの表示（nan, inf以外は整数部39ケタ、小数部149ケタ表示する）
    if (isnan(num) || isinf(num))
        printf("printf  : %f\n", num);
    else
    {
        ZeroMemory(s, sizeof(s));
        int i = sprintf_s(s, "printf  : %.*f", FLT_FRACSIZE - 1, num);
        ss = &s[i - 1];
        while ('0' == *ss || *ss == NULL)
        {
            if ('0' == *ss) *ss = NULL;
            --ss;
        }
        if (*ss == '.')
        {
            *(ss + 1) = '0';
        }
        printf("%s\n", s);
        //printf("printf  : %0*.*f\n", FLT_INTSIZE + FLT_FRACSIZE, FLT_FRACSIZE - 1, num);
    }
    fflush(stdout);
    // 自作printfloatの表示（nan, inf以外は整数部39ケタ、小数部149ケタ表示する）
    fprintf(stdout, "putfloat: ");
    printfloat(num);
    fprintf(stdout, "\n");
}

int             main(void)
{
    double	    num;
    uint64_t    mem;

    printcomp(8205.3);
    printcomp(42);
    printcomp(4.2);
    printcomp(0.0);
    printcomp(1.1);
    printcomp(M_PI);        // 円周率 (math.hで定義)
    printcomp(LDBL_MAX);     // 正規数の最大 = 3.40282347e+38F (float.hで定義)
    printcomp(LDBL_MIN);     // 正規数の最小= 1.17549435e-38F (float.hで定義)
    mem = 1;
    memcpy(&num, &mem, sizeof(mem));
    printcomp(num);         // 非正規数の最小 (仮数部の最下位bitのみ1)
    //printcomp(0.0/0.0);     // nan
    //printcomp(1.0/0.0);     // inf
    //printcomp(-1.0/0.0);    // -inf
    return (0);
}
