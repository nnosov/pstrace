#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>
#include <sys/time.h>

class SC_Mutex {
public:
    SC_Mutex() {
        pthread_mutex_init(&m, NULL);
    }
    SC_Mutex(SC_Mutex const&) {
        pthread_mutex_init(&m, NULL);
    }
    SC_Mutex& operator=(SC_Mutex const& rhs) {
        if (&rhs != this) {
            pthread_mutex_init(&m, NULL);
        }
        return *this;
    }
    ~SC_Mutex() {
        pthread_mutex_destroy(&m);
    }

    void Lock() {
        pthread_mutex_lock(&m);
    }

    void Unlock() {
        pthread_mutex_unlock(&m);
    }

    bool Trylock() {
        return (pthread_mutex_trylock(&m) == 0);
    }
private:
    pthread_mutex_t m;
};

#endif /* MUTEX_H_ */
