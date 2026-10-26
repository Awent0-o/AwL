#include <stdio.h>
#include <stdbool.h>
#define print(x) _Generic((x), \
    float: printf("%f", x), \
    char*: printf("%s", x), \
    char: printf("%c", x), \
    int: printf("%d", x), \
    bool: printf("%d", x), \
    default: printf("undefine type") \
)


#define AWL_ARRAY_RETURN_SIZE 256
static int _awl_ret_buf[AWL_ARRAY_RETURN_SIZE];

int partition(int *arr, int low, int high) {
    int pivot = arr[high];
    __auto_type i = low - 1;
for(int j =  low; j < high; j++) {
if( arr[j] <= pivot ){
    i ++ ;
    int t = arr[i];
    arr[i] = arr[j];
    arr[j] = t;
    }
    }
    int t = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = t;
    return i + 1;
}

void quick_sort(int *arr, int low, int high) {
if( low < high ){
    int pi = partition(arr, low, high);
    quick_sort(arr, low, pi - 1);
    quick_sort(arr, pi + 1, high);
    }
}

void printArray(int *arr, int size) {
for(int i =  0; i < size; i++) {
    printf("%d\n", arr[i]);
    }
}

int main() {
int arr[] = {1, 10, 2, 3, 9, 4, 7, 5, 8, 6};
    int s = 0;
s = sizeof(arr)/sizeof(arr[0]);
    printf("not sorted\n");
    printArray(arr, s);
    quick_sort(arr, 0, s - 1);
    printf("sorted\n");
    printArray(arr, s);
return 0;
}
