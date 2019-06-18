#include <stdio.h>
#include <time.h>

// Comment DEBUG line to focus on time
// Uncomment DEBUG line to print arrays (not time-focused)
//#define DEBUG 1

int n, array[2000000], c, d, t;

void insertion_sort() {
    for (c = 1; c <= n - 1; c++) {
        d = c;

        while (d > 0 && array[d - 1] > array[d]) {
            t = array[d];
            array[d] = array[d - 1];
            array[d - 1] = t;

            d--;
        }
    }
}

int main() {

    printf("Enter size of array (max is 2 million)\n");
    scanf("%d", & n);

    // Creates worst-case array
    for (c = 0; c < n; c++) {
        array[c] = n - c;
    }

    // Prints unsorted array
    #ifdef DEBUG
    printf("[ ");
    for (c = 0; c <= n - 1; c++)
        printf("%d ", array[c]);
    printf("]");
    #endif

    // Ticks the clock
    clock_t start, end;
    double cpu_time_used;
    start = clock();

    insertion_sort();

    // Tocks the clock
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("\nTime that took to sort was %.2f seconds\n", cpu_time_used);

    // Prints sorted array
    #ifdef DEBUG
    printf("[ ");
    for (c = 0; c <= n - 1; c++)
        printf("%d ", array[c]);
    printf("]");
    #endif

    return 0;
}

