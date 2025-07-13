#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// See: https://stackoverflow.com/questions/25190073/difference-between-condition-variable-and-condition-predicate-in-java-synchroniz
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond; // condition variable:
                         // is a construct provided by an OS or threading system that provides
                         // `wait` and `notify`
    int turn; // predicate:
              // a thread waits on a condition variable until the predicate is true
              // condition var must used with a predicate
              // 0 → tick may run, 1 → tock may run
} Primitive;

void* tick_thread(void* arg) {
    Primitive* pr = (Primitive*)arg;
    for (int i = 0; i < 10; i++) { // 打印10次0
        pthread_mutex_lock(&pr->mutex);
        while(pr->turn != 0) pthread_cond_wait(&pr->cond, &pr->mutex);
        printf("0 "); // Tick

        // tail process
        pr->turn = 1;
        pthread_mutex_unlock(&pr->mutex);
        pthread_cond_signal(&pr->cond); // 唤醒打印1的线程
    }
    return NULL;
}

void* tock_thread(void* arg) {
    Primitive* pr = (Primitive*)arg;
    for (int i = 0; i < 10; i++) { // 打印10次1
        pthread_mutex_lock(&pr->mutex);
        while(pr->turn != 1) pthread_cond_wait(&pr->cond, &pr->mutex);
        printf("1 "); // Tock

        // tail process
        pr->turn = 0;
        pthread_mutex_unlock(&pr->mutex);
        pthread_cond_signal(&pr->cond); // 唤醒打印0的线程
    }
    return NULL;
}

int implement_1() {
    Primitive pr;
    pthread_mutex_init(&pr.mutex, NULL);
    pthread_cond_init(&pr.cond, NULL);
    pr.turn = 0;

    setvbuf(stdout, NULL, _IONBF, 0); // avoid stdio buffer

    pthread_t t0, t1;
    pthread_create(&t0, NULL, tick_thread, &pr);
    pthread_create(&t1, NULL, tock_thread, &pr);
    // pthread_cond_signal(&pr.cond); // initial signal to trigger
    //                                     // Why Error: pthread signal do not have pending
    //                                     //            if we send signal but no comsue
    //                                     //            signal will lost


    pthread_join(t0, NULL);
    pthread_join(t1, NULL);

    pthread_mutex_destroy(&pr.mutex);
    pthread_cond_destroy(&pr.cond);

    printf("\n");
    return 0;
}

int main(int argc, char *argv[]) {
    implement_1();

    return 0;
}
