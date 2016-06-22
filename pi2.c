#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <pthread.h>

#define DISP_VAR(var, format) \
    do { \
        fprintf(stderr, "%s:%d " #var " = " #format "\n", \
                __FUNCTION__, __LINE__, (var)); \
    } while (0) 
#define DISP_ARRAY(var, format, length) \
    do { \
        fprintf(stderr, "%s:%d " #var " = {", __FUNCTION__, __LINE__); \
        for (size_t i=0; i<(size_t)length; ++i) { \
            fprintf(stderr, #format ", ", (var)[i]); \
        } \
        fprintf(stderr, "}\n"); \
    } while (0)
#define DISP_MEMORY_AS_CHARACHER(var, length) \
    do { \
        fprintf(stderr, "%s:%d " #var " = \"", __FUNCTION__, __LINE__); \
        for (size_t i=0; i<length; ++i) \
            buffer[i] == '\0' ? \
                fprintf(stderr, "\\0") : \
                fputc(buffer[i], stderr); \
        fprintf(stderr, "\"\n"); \
    } while (0)

typedef struct {
    long long start_at;
    long long step;
    long long max_i;
    long double * result_ptr;
} argument;

void * worker(void * sb) {
    argument * p=(argument *)sb;
    *p->result_ptr = 0.0;

    DISP_VAR(p->start_at, %lld);
    DISP_VAR(p->step, %lld);
    DISP_VAR(p->max_i, %lld);
    
    for (long long i=p->start_at; i<p->max_i; i+=p->step) {
        long double delta=(long double)1/(i*2+1);
        *p->result_ptr = i & 1 ? 
            *p->result_ptr - delta : 
            *p->result_ptr + delta;
    }
    fprintf(stderr, "End of Thread %lld\n", p->start_at);
    return NULL;
}

int main(int argc, char ** argv) {
    assert(argc == 2 && 
            "You must specify the thread number via parameter. "
            "(e.g. for 4 threads, enter $ pi2 4 )");

    int thread_number=atoi(argv[1]);
    assert(thread_number > 0 && "Invalid thread number.");

    pthread_t threads[thread_number];
    argument args[thread_number];
    long double results[thread_number];

    for (size_t i=0; i<(size_t)thread_number; ++i) {
        args[i].start_at = i;
        args[i].step = thread_number;
        args[i].max_i = 1000000000;
        args[i].result_ptr = results + i;
        pthread_create(threads+i, NULL, worker, (void *)(args + i));
    }

    for (size_t i=0; i<(size_t)thread_number; ++i) {
        pthread_join(threads[i], NULL);
    }

    long double result=0.0;
    for (size_t i=0; i<(size_t)thread_number; ++i) {
        result += results[i];
    }

    printf("%.20Lf\n", 4*result);

    return 0;
}

