#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include <pthread.h>
#include <unistd.h>

pthread_mutex_t stdout_mutex=PTHREAD_MUTEX_INITIALIZER;

#define LOG(format, ...) \
    do { \
        sleep(1); \
        pthread_mutex_lock(&stdout_mutex); \
        printf("[%s:%d] " #format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
        pthread_mutex_unlock(&stdout_mutex); \
    } while (0)

const size_t buffer_size=4;
typedef struct {
    int head, tail;
    size_t size;
    char data[buffer_size];
    pthread_mutex_t lock;
    pthread_cond_t non_empty, non_full;
} buffer_t;

void buffer_init(buffer_t * buf) {
    buf->head = 0;
    buf->tail = 0;
    buf->size = 0;
    pthread_mutex_init(&buf->lock, NULL);
    pthread_cond_init(&buf->non_empty, NULL);
    pthread_cond_init(&buf->non_full, NULL);
}

#define buffer_is_empty(buf)        (0 == (buf).size)
#define buffer_is_full(buf)         (buffer_size == (buf).size)
#define buffer_push(buf, ch) ({ \
    buf.data[buf.head] = (ch); \
    buf.head = (buf.head + 1)%buffer_size; \
    buf.size += 1; })
#define buffer_pop(buf) ({ \
    char __t=buf.data[buf.tail]; \
    buf.tail = (buf.tail + 1)%buffer_size; \
    buf.size -= 1; \
    __t; })


buffer_t buffer1, buffer2;

void *producer(void *dummy) {
    (void)dummy;

    LOG("====================");

    for (char ch='a'; ch<='h'; ++ch) {
        LOG("字符 %c 已生产，准备塞入缓冲 buffer1", ch);
        pthread_mutex_lock(&buffer1.lock);
        while (true) {
            LOG("检查 buffer1 是否非满");
            if (buffer_is_full(buffer1)) {
                LOG("buffer1 满，等待变为非满");
                pthread_cond_wait(&buffer1.non_full, &buffer1.lock);
            } else {
                LOG("buffer1 非满确认☑️");
                break;
            }
        }
        
        buffer_push(buffer1, ch);
        pthread_cond_signal(&buffer1.non_empty);

        pthread_mutex_unlock(&buffer1.lock);
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
        LOG("尝试从 buffer1 中取数据");
        pthread_mutex_lock(&buffer1.lock);
        while (true) {
            LOG("检查 buffer1 是否非空");
            if (buffer_is_empty(buffer1)) {
                LOG("buffer1 空，等待变为非空");
                pthread_cond_wait(&buffer1.non_empty, &buffer1.lock);
            } else {
                LOG("buffer1 非空确认☑️");
                break;
            }
        }

        char ch=buffer_pop(buffer1);
        pthread_cond_signal(&buffer1.non_full);

        pthread_mutex_unlock(&buffer1.lock);

        LOG("字符 %c 已从缓冲 buffer1 取得", ch);

        sleep(5);
        ch = toupper(ch);

        LOG("字符 %c 已计算，准备塞入缓冲 buffer2", ch);
        pthread_mutex_lock(&buffer2.lock);
        while (true) {
            LOG("检查 buffer2 是否非满");
            if (buffer_is_full(buffer2)) {
                LOG("buffer2 满，等待变为非满");
                pthread_cond_wait(&buffer2.non_full, &buffer2.lock);
            } else {
                LOG("buffer2 非满确认☑️");
                break;
            }
        }
        
        buffer_push(buffer2, ch);
        pthread_cond_signal(&buffer2.non_empty);

        pthread_mutex_unlock(&buffer2.lock);
        LOG("字符 %c 已塞入缓冲 buffer2", ch);
    }

    return NULL;
}

void *consumer(void *dummy) {
    (void)dummy;
    LOG("====================");

    while (true) {
        LOG("尝试从 buffer2 中取数据");
        pthread_mutex_lock(&buffer2.lock);
        while (true) {
            LOG("检查 buffer2 是否非空");
            if (buffer_is_empty(buffer2)) {
                LOG("buffer2 空，等待变为非空");
                pthread_cond_wait(&buffer2.non_empty, &buffer2.lock);
            } else {
                LOG("buffer2 非空确认☑️");
                break;
            }
        }

        char ch=buffer_pop(buffer2);
        pthread_cond_signal(&buffer2.non_full);

        pthread_mutex_unlock(&buffer2.lock);

        LOG("字符 %c 已从缓冲 buffer2 取得，然后我要打印这个字符了", ch);
        pthread_mutex_lock(&stdout_mutex);
        putchar(ch);
        pthread_mutex_unlock(&stdout_mutex);
    }
}

int main() {
    
    buffer_init(&buffer1);
    buffer_init(&buffer2);

    pthread_t producer_thread;
    pthread_t calculator_thread;
    pthread_t consumer_thread;
    
    pthread_create(&producer_thread, NULL, producer, NULL); 
    sleep(20);
    pthread_create(&calculator_thread, NULL, calculator, NULL); 
    sleep(20);
    pthread_create(&consumer_thread, NULL, consumer, NULL); 

    pthread_join(calculator_thread, NULL);

    return 0;
}

