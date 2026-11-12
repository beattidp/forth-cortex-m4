#ifndef __FORTH_CONFIG_H
#define __FORTH_CONFIG_H

#include <stdint.h>

/* Memory Map Configuration for ARM Cortex-M4 CamelForth DTC (128 KB SRAM)
 *
 * SRAM1 Layout (128 KB):
 *  0x20000000 - 0x20000FFF : C data, BSS, HAL drivers (4 KB)
 *  0x20001000 - 0x2000103F : RAM Trampoline Table (40 bytes)
 *  0x20001040 - 0x200010FF : User Area (192 bytes, UP points to 0x20001040)
 *  0x20001100 - 0x200011FF : Terminal Input Buffer (TIB, 256 bytes)
 *  0x20001200 - 0x200015FF : Numeric PAD & HOLD Buffer (1 KB)
 *  0x20001600 - 0x200017FF : Leave Stack (LP, 512 bytes)
 *  0x20001800              : Parameter Stack (PSP top at 0x20001800)
 *  0x20002000              : Return Stack (RSP top at 0x20002000)
 *  0x20002000 - 0x2001DFFF : RAM Dictionary Space (~112 KB free)
 *  0x2001E000 - 0x2001FFFF : C Main Stack (MSP, 8 KB)
 */

#define FORTH_TRAMPOLINE_BASE   0x20001000U
#define FORTH_USER_BASE         0x20001040U
#define FORTH_TIB_BASE          0x20001100U
#define FORTH_PAD_BASE          0x20001200U
#define FORTH_LP_BASE           0x20001600U
#define FORTH_PSP_TOP           0x20001800U
#define FORTH_RSP_TOP           0x20002000U
#define FORTH_RAM_DICT_BASE     0x20002000U
#define FORTH_RAM_DICT_END      0x2001DFFCU

#define FORTH_TIB_SIZE          256U
#define FORTH_PAD_SIZE          256U
#define FORTH_CELL_SIZE         4U

#endif /* __FORTH_CONFIG_H */
