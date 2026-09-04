#include "arch/xtensa/drivers/cores/smp.h"

/*
 * kernel/smp/smp.c -- toutes les adresses ci-dessous viennent
 * directement du code source esp-idf (voir smp.h). Références exactes :
 *   - RTC_CNTL_*    : components/soc/esp32s3/register/soc/rtc_cntl_reg.h
 *   - SYSTEM_*      : components/soc/esp32s3/register/soc/system_reg.h
 *   - INTERRUPT_CORE0/1_CPU_INTR_FROM_CPU_*_MAP_REG :
 *       components/soc/esp32s3/register/soc/interrupt_core{0,1}_reg.h
 *   - Séquence de démarrage : components/hal/esp32s3/include/hal/cpu_utility_ll.h
 *   - Séquence IPI : components/hal/esp32s3/include/hal/crosscore_int_ll.h
 */

#define RTC_CNTL_OPTIONS0_REG          (*(volatile uint32_t *) 0x60008000u)
#define RTC_CNTL_SW_STALL_APPCPU_C0_M  0x3u

#define RTC_CNTL_SW_CPU_STALL_REG      (*(volatile uint32_t *) 0x600080BCu)
#define RTC_CNTL_SW_STALL_APPCPU_C1_M  (0x3Fu << 20)

#define SYSTEM_CORE_1_CONTROL_0_REG      (*(volatile uint32_t *) 0x600C0000u)
#define SYSTEM_CONTROL_CORE_1_RUNSTALL   (1u << 0)
#define SYSTEM_CONTROL_CORE_1_CLKGATE_EN (1u << 1)
#define SYSTEM_CONTROL_CORE_1_RESETING   (1u << 2)

#define SYSTEM_CPU_INTR_FROM_CPU_0_REG (*(volatile uint32_t *) 0x600C0030u)
#define SYSTEM_CPU_INTR_FROM_CPU_1_REG (*(volatile uint32_t *) 0x600C0034u)
#define SYSTEM_CPU_INTR_FROM_CPU_0     (1u << 0)
#define SYSTEM_CPU_INTR_FROM_CPU_1     (1u << 0)

#define INTERRUPT_CORE0_CPU_INTR_FROM_CPU_1_MAP_REG (*(volatile uint32_t *) 0x600C2140u)
#define INTERRUPT_CORE1_CPU_INTR_FROM_CPU_0_MAP_REG (*(volatile uint32_t *) 0x600C293Cu)

/* Ligne d'interruption CPU (0-31) choisie pour l'IPI, sur CHAQUE cœur.
 * La ligne 6 est déjà prise par le timer du scheduler (voir
 * interrupts.c) -- on prend la 7. Contrairement au timer (câblé en
 * interne au cœur, sans passer par la matrice), CPU_INTR_FROM_CPU_x DOIT
 * être explicitement routé vers cette ligne via le registre MAP
 * correspondant, sinon l'interruption n'arrive jamais. */
#define IPI_CPU_LINE 7

extern void ets_set_appcpu_boot_addr(uint32_t start); /* fournie par la ROM */
extern void _core1_entry(void); /* arch/xtensa/boot/core1_entry.S */

static void (*core1_user_entry)(void) = NULL;
static ipi_handler_t ipi_handlers[2];

int smp_get_core_id(void) {
    uint32_t prid;
    __asm__ volatile ("rsr.prid %0" : "=r"(prid));
    return (prid != 0) ? 1 : 0;
}

void smp_start_core1(void (*entry)(void)) {
    core1_user_entry = entry;

    /* Routage matriciel côté cœur 0 : sans cette ligne, une IPI envoyée
     * PAR le cœur 1 (via SYSTEM_CPU_INTR_FROM_CPU_1_REG) n'est routée
     * vers AUCUNE ligne d'interruption locale du cœur 0 -- elle reste
     * physiquement "en attente" au niveau du périphérique sans jamais
     * atteindre le CPU. Sans ce correctif, seul core1 pouvait recevoir
     * des IPI (configuré dans core1_main()) ; le sens cœur1 -> cœur0
     * était muet. */
    INTERRUPT_CORE0_CPU_INTR_FROM_CPU_1_MAP_REG = IPI_CPU_LINE;

    /* 1. Lever le "stall" logiciel RTC_CNTL -- mécanisme DISTINCT du
     * RUNSTALL de SYSTEM_CORE_1_CONTROL_0_REG plus bas ; les deux
     * doivent être levés indépendamment. */
    RTC_CNTL_OPTIONS0_REG &= ~RTC_CNTL_SW_STALL_APPCPU_C0_M;
    RTC_CNTL_SW_CPU_STALL_REG &= ~RTC_CNTL_SW_STALL_APPCPU_C1_M;

    /* 2. Activer l'horloge et lever le reset du cœur 1 (ne rien faire
     * si déjà actif -- évite un reset accidentel qui effacerait des
     * points d'arrêt si un débogueur JTAG a déjà réveillé le cœur). */
    if (!(SYSTEM_CORE_1_CONTROL_0_REG & SYSTEM_CONTROL_CORE_1_CLKGATE_EN)) {
        SYSTEM_CORE_1_CONTROL_0_REG |= SYSTEM_CONTROL_CORE_1_CLKGATE_EN;
        SYSTEM_CORE_1_CONTROL_0_REG &= ~SYSTEM_CONTROL_CORE_1_RUNSTALL;
        SYSTEM_CORE_1_CONTROL_0_REG |= SYSTEM_CONTROL_CORE_1_RESETING;
        SYSTEM_CORE_1_CONTROL_0_REG &= ~SYSTEM_CONTROL_CORE_1_RESETING;
    }

    /* 3. Adresse de démarrage : toujours notre propre point d'entrée
     * bas niveau, qui pose sa propre pile + VECBASE avant d'appeler
     * core1_main() (plus bas), qui appelle enfin `entry`. */
    ets_set_appcpu_boot_addr((uint32_t) _core1_entry);
}

/* Appelée depuis _core1_entry.S une fois pile + VECBASE prêts sur le
 * cœur 1. Ne revient jamais. */
void core1_main(void) {
    INTERRUPT_CORE1_CPU_INTR_FROM_CPU_0_MAP_REG = IPI_CPU_LINE;

    /* Même convention que _ResetVector côté cœur 0 : EXCM=0, INTLEVEL=0,
     * UM=0. */
    uint32_t ps = 0;
    __asm__ volatile ("wsr.ps %0; rsync" :: "r"(ps));

    if (core1_user_entry != NULL) {
        core1_user_entry();
    }

    for (;;) {
        __asm__ volatile ("waiti 0");
    }
}

void smp_send_ipi(int core_id) {
    if (core_id == smp_get_core_id()) {
        return; /* s'auto-interrompre n'a pas de sens ici */
    }

    if (core_id == 0) {
        SYSTEM_CPU_INTR_FROM_CPU_0_REG = SYSTEM_CPU_INTR_FROM_CPU_0;
    } else {
        SYSTEM_CPU_INTR_FROM_CPU_1_REG = SYSTEM_CPU_INTR_FROM_CPU_1;
    }
}

static void smp_ipi_ack(int from_core) {
    /* On acquitte le registre QUE L'AUTRE cœur a écrit pour NOUS
     * interrompre -- si from_core==1, c'est le cœur 1 qui a écrit
     * SYSTEM_CPU_INTR_FROM_CPU_1_REG pour nous atteindre. */
    if (from_core == 0) {
        SYSTEM_CPU_INTR_FROM_CPU_0_REG = 0;
    } else {
        SYSTEM_CPU_INTR_FROM_CPU_1_REG = 0;
    }
}

void smp_ipi_register_handler(int from_core, ipi_handler_t handler) {
    if (from_core == 0 || from_core == 1) {
        ipi_handlers[from_core] = handler;
    }
}

/* Appelée depuis c_interrupt_handler() (interrupts.c) quand le bit
 * IPI_CPU_LINE est présent dans le registre INTERRUPT local. Dans un
 * système à 2 cœurs, ce cœur ne peut recevoir une IPI QUE de l'autre
 * cœur -- pas besoin de lire un registre pour savoir "de qui" au-delà
 * de smp_get_core_id(). */
void smp_ipi_dispatch_local(void) {
    int my_core = smp_get_core_id();
    int from_core = (my_core == 0) ? 1 : 0;

    smp_ipi_ack(from_core);

    if (ipi_handlers[from_core] != NULL) {
        ipi_handlers[from_core]();
    }
}
