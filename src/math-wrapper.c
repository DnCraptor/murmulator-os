#include "math-wrapper.h"

uint32_t __u32u32u32_div(uint32_t x, uint32_t y) {
    return x / y;
}

uint32_t __u32u32u32_rem(uint32_t x, uint32_t y) {
    return x % y;
}

float __fff_div(float x, float y) {
    return x / y;
}
float __fff_mul(float x, float y) {
    return x * y;
}
float __ffu32_mul(float x, uint32_t y) {
    return x * y;
}
float __ffu32_div(float x, uint32_t y) {
    return x / y;
}

double __ddd_div(double x, double y) {
    return x / y;
}
double __ddd_mul(double x, double y) {
    return x * y;
}
double __ddu32_mul(double x, uint32_t y) {
    return x * y;
}
double __ddu32_div(double x, uint32_t y) {
    return x / y;
}
double __ddf_mul(double x, float y) {
    return x * y;
}
