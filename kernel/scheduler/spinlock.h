#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "kernel/types.h"

/*
 * kernel/scheduler/spinlock.h
 *
 * Exclusion mutuelle SANS instruction atomique : l'algorithme de
 * Peterson, qui n'utilise que des lectures/écritures ordinaires.
 *
 * POURQUOI PAS S32C1I : la version précédente utilisait S32C1I
 * (compare-and-swap de l'ISA Xtensa). Testé sur la machine esp32s3 de
 * QEMU : l'instruction ne revient JAMAIS (le tout premier
 * spinlock_try_acquire() bloque, avant même que le timer soit armé,
 * donc sans aucune concurrence en jeu). Elle n'est pas exploitable
 * ici, quelle qu'en soit la raison exacte (option absente de la config
 * du cœur émulé, ou émulation incomplète).
 *
 * LIMITE STRUCTURELLE : Peterson ne marche que pour DEUX participants.
 * Ce verrou est donc valable tant que le système a exactement 2 cœurs
 * (ce qui est le cas de l'ESP32-S3). Il ne se généralise pas à 3+.
 *
 * REPOSE SUR : smp_get_core_id() renvoyant bien 0 et 1 (via PRID).
 * C'est la même hypothèse non vérifiée que dans smp.c -- si elle est
 * fausse, les deux cœurs se croiront le même et le verrou ne protégera
 * rien. À valider en affichant smp_get_core_id() depuis chaque cœur.
 */

typedef struct {
    volatile uint32_t interested[2];
    volatile uint32_t turn;
} spinlock_t;

#define SPINLOCK_INIT { {0, 0}, 0 }

void spinlock_init(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

/* Variantes sûres vis-à-vis des interruptions LOCALES, en plus de
 * l'exclusion entre cœurs. À utiliser dès qu'une section critique peut
 * être interrompue par un handler qui prend le même verrou -- c'est
 * précisément le cas de uart_print(), appelée à la fois depuis les
 * tâches ET depuis c_interrupt_handler(). Sans ça : auto-interblocage
 * garanti dès que le timer du scheduler tourne. */
uint32_t spinlock_acquire_irqsave(spinlock_t *lock);
void     spinlock_release_irqrestore(spinlock_t *lock, uint32_t saved_ps);

#endif /* SPINLOCK_H */
