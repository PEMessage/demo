
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// adaptative_clipping_filter
void adaptative_clipping_filter(int* data, int length, int v0_init, int v1_init) {
    int* start = data;
    int* end = data + length;
    int v0 = v0_init;
    int v1 = v1_init;
    int v2;

    printf("Initial values: v0=%d, v1=%d\n", v0, v1);


    while (start < end) {
        v2 = *start;

        if (v0 < v2) {         // [ 0 -- v0 -- v2]
            if (v2 <= v1)      // [ 0 -- v0 -- v2 -- v1]
                v1 = v2;       // [ 0 -- v0 -- v2 v1' <- v1]
            else if (v1 < v0)  // [ 0 -- v1 --  v0 -- v2]
                v1 = v0;       // [ 0 -- v1 -> v1' v0 -- v2]
        }
        else {                 // [ 0 -- v2 -- v0 ]
            if (v0 <= v1)      // [ 0 -- v2 -- v0 -- v1]
                v1 = v0;       // [ 0 -- v2 -- v0 v1' <- v1]
            else if (v1 < v2)  // [ 0 -- v1 -- v2 -- v0]
                v1 = v2;       // [ 0 -- v1 -> v1' v2 -- v0]
        }

        // conclusion: v1 must in between  [v0-v2(input)]
        // [ v0 -- v1 -- v2] : v0, previous output as low-boundary
        //                   : v2, current input

        // clipping filter
        *start = v1; // use v1 as output

        // adaptative adjust
        v0 = v1; // pervious output update as low-boundary
        v1 = v2; // current input as next protential output

        start += 1;
    }
}

// Raw ADC data:
//
// P output: Potential output(when it within scope of [low-boundary, high-boundary])
// [: high-boundary
// ]: low-boundary
// +-----------------------+
// | 42(P output)   961(]) | 983  417  881  679  134  101  418  179  105 1017  210  703  962  405  118  253  201   89
// |       +---------------+
// |       |         |
// |       |         V
// |500([) |        500      961  961  881  881  679  134  134  179  179  179  210  210  703  703  405  253  253  201
// +-------+
// Macro to check if a value is within the scope [a, b] or [b, a] (order-independent)

int main() {
    // Generate some mock ADC data (random values between 0-1023)
    const int data_length = 20;
    int adc_data[data_length];

    srand(time(NULL));
    for (int i = 0; i < data_length; i++) {
        adc_data[i] = rand() % 1024;
    }

    // Initial filter configuration
    int v0_init = 512;  // Initial lower bound
    int v1_init = 512;  // Initial upper bound

    // Print raw data
    printf("Raw ADC data:\n");
    for (int i = 0; i < data_length; i++) {
        printf("%4d ", adc_data[i]);
    }
    printf("\n");

    // Apply filter
    adaptative_clipping_filter(adc_data, data_length, v0_init, v1_init);

    // Print filtered data
    printf("Filtered data:\n");
    for (int i = 0; i < data_length; i++) {
        printf("%4d ", adc_data[i]);
    }
    printf("\n");

    return 0;
}
