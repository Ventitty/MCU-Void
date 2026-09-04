#include "kernel/scheduler/spinlock.h"

/*
 * kernel/smp/spinlock.c
 *
 * Ce verrou protège uniquement contre la concurrence entre CŒURS. Si le
 * code qui le prend peut aussi être interrompu par une interruption SUR
 * LE MÊME cœur qui essaierait elle-même de prendre le même verrou, il y
 * a un risque d'auto-interblocage classique -- ce fichier ne s'occupe
 * pas de ça. Pour une section critique qui doit être sûre à la fois
 * entre cœurs ET vis-à-vis des interruptions locales, combine avec les
 * fonctions de désactivation d'interruptions déjà utilisées ailleurs
 * dans ce projet (voir le motif PS.INTLEVEL dans mmu.c) autour de
 * l'acquisition/libération.
 */

void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

int spinlock_try_acquire(spinlock_t *lock) {
    uint32_t result;
    uint32_t expected = 0; /* déverrouillé */
    uint32_t desired   = 1; /* verrouillé */

    __asm__ volatile (
        "wsr.scompare1 %2\n\t"
        "s32c1i %0, %1, 0\n\t"
        : "+r"(desired)
        : "r"(&lock->locked), "r"(expected)
        : "memory"
    );
    result = desired; /* s32c1i renvoie l'ancienne valeur lue dans le même registre */

    return (result == 0) ? 1 : 0;
}

void spinlock_acquire(spinlock_t *lock) {
    while (!spinlock_try_acquire(lock)) {
        /* Boucle active simple. Pas de backoff/pause -- suffisant pour
         * un premier test SMP, à améliorer si la contention devient un
         * problème réel (par exemple avec un compteur de tentatives
         * avant de céder la main via sched_yield()). */
    }
}

void spinlock_release(spinlock_t *lock) {
    __asm__ volatile ("memw" ::: "memory"); /* barrière mémoire avant de relâcher */
    lock->locked = 0;
}
