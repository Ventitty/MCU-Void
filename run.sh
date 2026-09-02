#!/usr/bin/env bash
set -e

CHIP="esp32s3"
CC="arch/xtensa/xtensa-esp-elf/bin/xtensa-${CHIP}-elf-gcc"
QEMU="arch/xtensa/xtensa-esp-elf/bin/qemu-system-xtensa"
INCLUDES="-I."

SRC_DIR="kernel"
OUT_DIR="build"
ARCH_DIR="arch/xtensa"

LINKER_SCRIPT="arch/xtensa/boot/linker.ld"
KERNEL_ELF="${OUT_DIR}/kernel.elf"

echo "[1/3] Nettoyage et préparation du dossier de build"
mkdir -p ${OUT_DIR}
rm -f ${KERNEL_ELF}

echo "[2/3] Compilation du kernel"

${CC} -nostdlib -T ${LINKER_SCRIPT} -g -fno-tree-loop-distribute-patterns -mtext-section-literals -mlongcalls -fno-builtin -fno-stack-protector -fno-pic -fno-pie -Wall -Wextra -Werror -std=c99 -pedantic -ffreestanding \
    -mabi=call0 \
    ${INCLUDES} \
    -o ${KERNEL_ELF} \
    ${ARCH_DIR}/boot/boot.S ${SRC_DIR}/kernel.c ${SRC_DIR}/utils.c ${SRC_DIR}/memory_manager/mmu.c ${SRC_DIR}/scheduler/scheduler.c ${ARCH_DIR}/interrupts/interrupts.c ${ARCH_DIR}/interrupts/vector.S

echo "Succès : ${KERNEL_ELF} généré."

echo "[3/3] Démarrage de QEMU"

${QEMU} \
    -machine ${CHIP} \
    -display none \
    -serial mon:stdio \
    -kernel ${KERNEL_ELF} \
    -d int,guest_errors -D /tmp/int-trace.log
