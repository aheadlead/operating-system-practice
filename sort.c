#include <stdio.h>

#include <pthread.h>

int data[]={1, 5, 3, 7, 2, 7, 4, 1, 7, 8, 4, 3, 6};

void select_sort(int * from, int * to) {
    for (int * p=from; p<to; ++p) {
        int * smallest_ptr=p;
        for (int * q=p; q<to; ++q) {
            if (*q < *smallest_ptr)
                smallest_ptr = q;
        }
        int tmp=*p;  /* swap */
        *p = *smallest_ptr;
        *smallest_ptr = tmp;
    }
}

void * assist_worker(void * dummy) {
    (void)dummy;
    size_t data_size=sizeof(data)/sizeof(data[0]);
    select_sort(data + data_size/2, data + data_size);
    return NULL;
}

int main() {

    pthread_t assist_thread;
    pthread_create(&assist_thread, NULL, assist_worker, NULL);

    size_t data_size=sizeof(data)/sizeof(data[0]);

    select_sort(data, data + data_size/2);

    pthread_join(assist_thread, NULL);

    /* merge sort */
    int * left_hand=&data[0];
    int * right_hand=&data[data_size/2];
    while (left_hand < &data[data_size/2] && right_hand < &data[data_size]) {
        if (*left_hand < *right_hand) 
            printf("%d\n", *left_hand++);
        else
            printf("%d\n", *right_hand++);
    }
    while (left_hand == &data[data_size/2] && right_hand < &data[data_size]) {
        printf("%d\n", *right_hand++);
    }
    while (right_hand == &data[data_size] && left_hand < &data[data_size/2]) {
        printf("%d\n", *left_hand++);
    }

    return 0;
}
    
