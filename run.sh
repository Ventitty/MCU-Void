#!/usr/bin/env bash
set -e

CHIP="esp32s3"
CC="xtensa-esp-elf/bin/xtensa-${CHIP}-elf-gcc"
QEMU="qemu/bin/qemu-system-xtensa"
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

${CC} -nostdlib -T ${LINKER_SCRIPT} -g -O2 -mtext-section-literals \
    ${INCLUDES} \
    -o ${KERNEL_ELF} \
    ${ARCH_DIR}/boot/boot.S ${SRC_DIR}/kernel.c ${SRC_DIR}/memory_manager/mmu.c

echo "Succès : ${KERNEL_ELF} généré."

echo "[3/3] Démarrage de QEMU"

${QEMU} \
    -machine ${CHIP} \
    -nographic \
    -kernel ${KERNEL_ELF} \
    -bios none \
