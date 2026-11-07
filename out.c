/* Згенеровано транслятором calc. Збирання: gcc out.c -o out -lm */
#include <stdio.h>
#include <math.h>

int main(void)
{
    printf("%g\n", (1.0 + (2.0 * 3.0)));
    printf("%g\n", ((1.0 + 2.0) * 3.0));
    printf("%g\n", pow(2.0, pow(3.0, 2.0)));
    printf("%g\n", (-pow(2.0, 2.0)));
    printf("%g\n", ((10.0 - 4.0) - 3.0));
    double x = (3.0 + 4.0);
    printf("x = %g\n", x);
    double y = ((x * 2.0) - 1.0);
    printf("y = %g\n", y);
    double area = (3.1415899999999999 * pow(y, 2.0));
    printf("area = %g\n", area);
    x = (x + 1.0);
    printf("x = %g\n", x);
    printf("%g\n", fmod(x, 3.0));
    return 0;
}
