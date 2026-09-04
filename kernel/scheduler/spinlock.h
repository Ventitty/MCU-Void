#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "kernel/types.h"

/*
 * kernel/smp/spinlock.h
 *
 * Basé sur S32C1I ("Store 32 Compare and Conditionally Store") --
 * l'instruction atomique compare-and-swap du jeu d'instructions Xtensa
 * de base, pas un registre spécifique à l'ESP32-S3. Confiance élevée
 * ici, contrairement au reste de ce qui touche au SMP.
 *
 * Sémantique de S32C1I : SCOMPARE1 doit être chargé AVANT avec la
 * valeur attendue. `s32c1i at, ar, imm` fait atomiquement :
 *   tmp = *(ar + imm)
 *   if (tmp == SCOMPARE1) *(ar + imm) = at
 *   at = tmp   (valeur lue AVANT l'écriture, toujours renvoyée dans at)
 * Utilisé ici pour un verrou simple : compare 0 (déverrouillé), écrit 1
 * si effectivement 0, renvoie l'ancienne valeur pour savoir si on a
 * gagné la course.
 */

typedef struct {
    volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INIT { 0 }

void spinlock_init(spinlock_t *lock);

/* Bloque (boucle) jusqu'à obtention du verrou. */
void spinlock_acquire(spinlock_t *lock);

/* Essaie une fois, sans bloquer. Renvoie 1 si le verrou a été obtenu,
 * 0 sinon. */
int spinlock_try_acquire(spinlock_t *lock);

void spinlock_release(spinlock_t *lock);

#endif /* SPINLOCK_H */
