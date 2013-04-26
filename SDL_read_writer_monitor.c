
#include "tea_local.h"
#include "tea_datatype.h"
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"

/*
 * readers/writers monitor.
 *
 * this implementation is biased towards readers
 *
 */

typedef struct {
    int readersWaiting;
    int writersWaiting;
    int readers;
    int writers;
    void* cond_OkToRead;
    void* cond_OkToWrite;
    void* mutex;
} SDL_ReadWriterMon_t;

void* SDL_ReadWriterMon_init()
{
    SDL_ReadWriterMon_t* mon;

    mon = calloc(1,sizeof(SDL_ReadWriterMon_t));
    mon->mutex = SDL_CreateMutex();
    mon->cond_OkToRead = SDL_CreateCond();
    mon->cond_OkToWrite = SDL_CreateCond();
    mon->readers = mon->writers = 0;
    mon->readersWaiting = mon->writersWaiting = 0;
    return mon;
}

#if 0
/**
 * reset this */
void SDL_ReadWriterMon_reset(void* monitor)
{
    SDL_ReadWriterMon_t* mon = monitor;

}
#endif

void SDL_ReadWriterMon_start_read(void* monitor)
{
    SDL_ReadWriterMon_t* mon;

    mon = monitor;

    if (-1 == SDL_mutexP(mon->mutex))
        assert(FALSE);

    mon->readersWaiting++;

    /* writers are writing and there are writers waiting to write */
    while (0 != mon->writers || mon->writersWaiting != 0)
        SDL_CondWait(mon->cond_OkToRead, mon->mutex);

    mon->readers++;
    mon->readersWaiting--;
    assert(mon->readersWaiting >= 0);
    SDL_CondSignal(mon->cond_OkToRead);

    if (-1 == SDL_mutexV(mon->mutex))
        assert(FALSE);
}

void SDL_ReadWriterMon_end_read(void* monitor)
{
    SDL_ReadWriterMon_t* mon;

    mon = monitor;

    if (-1 == SDL_mutexP(mon->mutex))
        assert(FALSE);

    mon->readers--;
    if (0 == mon->readers)
        SDL_CondSignal(mon->cond_OkToWrite);

    if (-1 == SDL_mutexV(mon->mutex))
        assert(FALSE);
}

void SDL_ReadWriterMon_start_write(void* monitor)
{
    SDL_ReadWriterMon_t* mon;

    mon = monitor;

    if (-1 == SDL_mutexP(mon->mutex))
        assert(FALSE);

    mon->writersWaiting++;
    while (0 != mon->writers || mon->readers != 0)
        SDL_CondWait(mon->cond_OkToWrite, mon->mutex);

    mon->writers++;
    mon->writersWaiting--;
    assert(mon->writersWaiting >= 0);

    if (-1 == SDL_mutexV(mon->mutex))
        assert(FALSE);
}

void SDL_ReadWriterMon_end_write(void* monitor)
{
    SDL_ReadWriterMon_t* mon;
    
    mon = monitor;

    if (-1 == SDL_mutexP(mon->mutex))
        assert(FALSE);

    mon->writers--;
    if (mon->readersWaiting == 0)
        SDL_CondSignal(mon->cond_OkToWrite);
    else
        SDL_CondSignal(mon->cond_OkToRead);

    if (-1 == SDL_mutexV(mon->mutex))
        assert(FALSE);
}

