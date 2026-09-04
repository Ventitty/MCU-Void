#ifndef SMP_H
#define SMP_H

#include "kernel/types.h"

typedef void (*ipi_handler_t)(void);

void smp_start_core1(void (*entry)(void));
int smp_get_core_id(void);
void smp_send_ipi(int core_id);
void smp_ipi_register_handler(int from_core, ipi_handler_t handler);
void smp_ipi_dispatch_local(void);

#endif /* SMP_H */
