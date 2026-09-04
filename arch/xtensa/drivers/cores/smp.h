#ifndef SMP_H
#define SMP_H

#include "kernel/types.h"

/*
 * kernel/smp/smp.h
 *
 * Réveil du cœur 1 (APP_CPU) et IPI (interruption inter-cœurs).
 * Adresses vérifiées dans les sources esp-idf -- voir le commentaire en
 * tête de smp.c pour les chemins exacts.
 *
 * *** Point non vérifié : smp_get_core_id() ***
 * Utilise le registre spécial PRID (rsr.prid), en supposant PRID==0 sur
 * le cœur 0 et PRID!=0 sur le cœur 1. C'est le mécanisme standard Xtensa
 * pour l'identification de cœur, mais je n'ai pas de confirmation que
 * cette valeur exacte (0 vs non-0) soit correcte spécifiquement pour
 * l'ESP32-S3 sous QEMU -- à vérifier empiriquement (par exemple en
 * affichant smp_get_core_id() depuis du code tournant sur chaque cœur,
 * et en confirmant que ça donne bien 0 et 1 respectivement).
 */

typedef void (*ipi_handler_t)(void);

/* Réveille le cœur 1 et le fait démarrer sur `entry`. Sans effet si le
 * cœur 1 est déjà actif (ne le reset pas s'il l'est déjà -- utile si un
 * débogueur JTAG l'a déjà réveillé). */
void smp_start_core1(void (*entry)(void));

/* 0 pour le cœur qui a démarré en premier (PRO_CPU), 1 pour l'autre
 * (APP_CPU). Voir l'avertissement ci-dessus. */
int smp_get_core_id(void);

/* Envoie une IPI à `core_id` (0 ou 1). Ne fait rien si core_id est le
 * cœur courant. */
void smp_send_ipi(int core_id);

/* Enregistre le handler appelé quand CE cœur reçoit une IPI envoyée
 * PAR `from_core`. */
void smp_ipi_register_handler(int from_core, ipi_handler_t handler);

/* Appelée uniquement depuis c_interrupt_handler() (interrupts.c) --
 * pas destinée à être appelée directement ailleurs. */
void smp_ipi_dispatch_local(void);

#endif /* SMP_H */
