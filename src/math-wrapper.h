#pragma once
#include <stdint.h>

uint32_t __u32u32u32_div(uint32_t x, uint32_t y);
uint32_t __u32u32u32_rem(uint32_t x, uint32_t y);

float __fff_div(float x, float y);
float __fff_mul(float x, float y);
float __ffu32_div(float x, uint32_t y);
float __ffu32_mul(float x, uint32_t y);

double __ddd_div(double x, double y);
double __ddd_mul(double x, double y);
double __ddu32_mul(double x, uint32_t y);
double __ddu32_div(double x, uint32_t y);
double __ddf_mul(double x, float y);

double __trunc (double);
double __floor (double t);
double __pow (double, double);
double __sqrt (double);
double __sin (double);
double __cos (double);
double __tan (double);
double __atan (double);
double __log (double);
double __exp (double);

// pico-sdk\src\rp2_common\pico_float\float_aeabi.S
extern float __aeabi_fmul(float, float); //         single-precision multiplication
extern float __aeabi_i2f(int); //                   integer to float (single precision) conversion
extern float __aeabi_fadd(float, float); //         single-precision addition
extern float __aeabi_fsub(float, float);//     single-precision subtraction, x - y
extern float __aeabi_fdiv(float, float); //    single-precision division, n / d
extern int __aeabi_fcmpge(float, float); //        result (1, 0) denotes (>=, ?<) [2], use for C >=
extern int __aeabi_idivmod(int, int);
extern int __aeabi_idiv(int, int);
extern double __aeabi_f2d(float);
extern float __aeabi_d2f(double);
extern int __aeabi_f2iz(float); //                     float (single precision) to integer C-style conversion [3]
extern int __aeabi_fcmplt(float, float); //         result (1, 0) denotes (<, ?>=) [2], use for C <
extern double __aeabi_dsub(double, double); //     double-precision subtraction, x - y
extern int __aeabi_d2iz(double); //                     double (double precision) to integer C-style conversion [3]
extern int __aeabi_fcmpeq(float, float); //         result (1, 0) denotes (=, ?<>) [2], use for C == and !=
extern int __aeabi_fcmpun(float, float); //         result (1, 0) denotes (?, <=>) [2], use for C99 isunordered()
extern int __aeabi_fcmpgt(float, float); //         result (1, 0) denotes (>, ?<=) [2], use for C >
extern int __aeabi_dcmpge(double, double); //         result (1, 0) denotes (>=, ?<) [2], use for C >=
extern unsigned __aeabi_uidiv(unsigned, unsigned );
extern float __aeabi_ui2f(unsigned);
extern unsigned __aeabi_f2uiz(float); //             float (single precision) to unsigned C-style conversion [3]
extern int __aeabi_fcmple(float, float); //         result (1, 0) denotes (<=, ?>) [2], use for C <=
extern double __aeabi_dmul(double, double);
extern double __aeabi_ddiv(double, double);
extern double __aeabi_dadd(double, double);
extern double __aeabi_i2d(int);
extern int __aeabi_dcmpeq(double, double);
extern double __aeabi_ui2d(unsigned);
extern int __aeabi_dcmplt(double, double);
extern unsigned __aeabi_d2uiz(double);
extern long long __aeabi_lmul(long long, long long);
extern  int __clzsi2 (unsigned int a );