#include "kernel/scheduler/spinlock.h"
#include "arch/xtensa/drivers/cores/smp.h"

static inline void barrier(void) {
    __asm__ volatile ("memw" ::: "memory");
}

void spinlock_init(spinlock_t *lock) {
    lock->interested[0] = 0;
    lock->interested[1] = 0;
    lock->turn = 0;
    barrier();
}

void spinlock_acquire(spinlock_t *lock) {
    int me    = smp_get_core_id();
    int other = 1 - me;

    lock->interested[me] = 1;
    barrier();
    lock->turn = (uint32_t) other;
    barrier();


    while (lock->interested[other] && lock->turn == (uint32_t) other) {

    }
    barrier();
}

void spinlock_release(spinlock_t *lock) {
    int me = smp_get_core_id();
    barrier();
    lock->interested[me] = 0;
}

uint32_t spinlock_acquire_irqsave(spinlock_t *lock) {
    uint32_t old_ps;
    __asm__ volatile ("rsr.ps %0" : "=r"(old_ps));
    __asm__ volatile ("wsr.ps %0; rsync" :: "r"((old_ps & ~0xFu) | 0xFu));

    spinlock_acquire(lock);
    return old_ps;
}

void spinlock_release_irqrestore(spinlock_t *lock, uint32_t saved_ps) {
    spinlock_release(lock);
    __asm__ volatile ("wsr.ps %0; rsync" :: "r"(saved_ps));
}
