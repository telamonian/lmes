#include <immintrin.h>
#include <stdio.h>

int main()
{
    float a = 16.0f;
    float b = 9.0f;
    float c = 21.0f;

    __m256 AVXa = _mm256_set1_ps(a);
    __m256 AVXb = _mm256_set1_ps(b);
    __m256 AVXc = _mm256_set1_ps(c);
    __m256 AVXv = _mm256_fmadd_ps(AVXa, AVXb, AVXc);

    float temp[8] __attribute((aligned(32)));
    _mm256_store_ps(&temp[0], AVXv);
    printf("tempavx is %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
    temp[0], temp[1], temp[2], temp[3],
            temp[4], temp[5], temp[6], temp[7]);

    return 0;
}