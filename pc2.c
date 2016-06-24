#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>

/* semaphore with POSIX compatibility */
typedef struct {
    int value; 
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} sem_t;

int sem_init(sem_t * sem, int pshared, int value) {
    (void)pshared;
    sem->value = value;
    pthread_cond_init(&sem->cond, NULL);
    pthread_mutex_init(&sem->mutex, NULL);
    return 0;
}

int sem_wait(sem_t * sem) {
    pthread_mutex_lock(&sem->mutex);
    while (sem->value <= 0) {
        pthread_cond_wait(&sem->cond, &sem->mutex);
    }
    sem->value -= 1;
    pthread_mutex_unlock(&sem->mutex);
    return 0;
}

int sem_post(sem_t * sem) {
    pthread_mutex_lock(&sem->mutex);
    sem->value += 1;
    pthread_cond_signal(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);
    return 0;
}

void sem_getvalue(sem_t * sem, int * valp) {
    pthread_mutex_lock(&sem->mutex);
    *valp = sem->value;
    pthread_mutex_unlock(&sem->mutex);
}

/* end of definition of semaphore */

pthread_mutex_t stdout_mutex=PTHREAD_MUTEX_INITIALIZER;

#define LOG(format, ...) \
    do { \
        sleep(1); \
        pthread_mutex_lock(&stdout_mutex); \
        printf("[%s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
        pthread_mutex_unlock(&stdout_mutex); \
    } while (0)
#define LOG_SEM(sem) \
    do { \
        int t; \
        sem_getvalue(sem, &t); \
        LOG("信号量 "#sem" = %d", t); \
    } while (0)

#define BUFFER_SIZE 4


typedef struct {
    int head, tail;
    char data[BUFFER_SIZE];
    sem_t empty, full;
} buffer_t;

char * generate_random_semaphore_name() {
    static char name[5]="2333";
    srand(time(NULL));
    *(name + 1) = rand();
    *(name + 3) = rand();
    LOG("随机的信号量名为 %s", name);
    return name;
}

void buffer_init(buffer_t * buf) {
    buf->head = 0;
    buf->tail = 0;
    sem_init(&buf->empty, 0, BUFFER_SIZE);
    sem_init(&buf->full, 0, 0);
}

#define buffer_push(buf, ch) ({ \
    buf.data[buf.head] = (ch); \
    buf.head = (buf.head + 1)%BUFFER_SIZE; })
#define buffer_pop(buf) ({ \
    char __t=buf.data[buf.tail]; \
    buf.tail = (buf.tail + 1)%BUFFER_SIZE; \
    __t; })


buffer_t buffer1, buffer2;

void *producer(void *dummy) {
    (void)dummy;

    LOG("====================");

    for (char ch='a'; ch<='h'; ++ch) {
        LOG("字符 %c 已生产，准备塞入缓冲 buffer1", ch);

        LOG("等待 buffer1 非满");
        LOG_SEM(&buffer1.empty);
        sem_wait(&buffer1.empty);
        LOG("buffer1 非满确认☑️");

        buffer_push(buffer1, ch);
        LOG_SEM(&buffer1.full);
        sem_post(&buffer1.full);
        LOG("字符 %c 已塞入缓冲 buffer1", ch);
    }

    LOG("生产结束");
    return NULL;
}

void *calculator(void *dummy) {
    (void)dummy;
    LOG("====================");

    while (true) {
        sleep(2);

        LOG("等待 buffer1 非空");
        LOG_SEM(&buffer1.full);
        sem_wait(&buffer1.full);
        LOG("buffer1 非空确认☑️");

        char ch=buffer_pop(buffer1);
        LOG_SEM(&buffer1.empty);
        sem_post(&buffer1.empty);
        LOG("字符 %c 已从缓冲 buffer1 取得", ch);

        sleep(5);
        ch = toupper(ch);
        LOG("字符 %c 已计算，准备塞入缓冲 buffer2", ch);

        LOG("等待 buffer2 非满");
        LOG_SEM(&buffer2.empty);
        sem_wait(&buffer2.empty);
        LOG("buffer2 非满确认☑️");

        buffer_push(buffer2, ch);
        LOG_SEM(&buffer2.full);
        sem_post(&buffer2.full);
        LOG("字符 %c 已塞入缓冲 buffer2", ch);
    }

    return NULL;
}

void *consumer(void *dummy) {
    (void)dummy;
    LOG("====================");

    while (true) {
        LOG("等待 buffer2 非空");
        LOG_SEM(&buffer2.full);
        sem_wait(&buffer2.full);
        LOG("buffer2 非空确认☑️");

        char ch=buffer_pop(buffer2);
        LOG_SEM(&buffer2.empty);
        sem_post(&buffer2.empty);

        LOG("字符 %c 已从缓冲 buffer2 取得，然后我要打印这个字符了", ch);
        pthread_mutex_lock(&stdout_mutex);
        putchar(ch);
        putchar('\n');
        pthread_mutex_unlock(&stdout_mutex);
    }
}

int main() {
    
    buffer_init(&buffer1);
    buffer_init(&buffer2);

    LOG_SEM(&buffer1.empty);
    LOG_SEM(&buffer1.full);
    LOG_SEM(&buffer2.empty);
    LOG_SEM(&buffer2.full);

    pthread_t producer_thread;
    pthread_t calculator_thread;
    pthread_t consumer_thread;
    
    pthread_create(&producer_thread, NULL, producer, NULL); 
    sleep(30);
    pthread_create(&calculator_thread, NULL, calculator, NULL); 
    sleep(30);
    pthread_create(&consumer_thread, NULL, consumer, NULL); 

    pthread_join(calculator_thread, NULL);

    return 0;
}

