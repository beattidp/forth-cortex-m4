/* forth_m4_d.inc -- CPU and Model Dependencies for ARM Cortex-M4
 * Ported from CAML382d.S (CamelForth 32-bit by Douglas Beattie Jr. / Brad Rodriguez)
 *
 * Direct-Threaded Forth model for ARM Cortex-M4 (ARMv7E-M / Thumb-2)
 *   cell size is    32 bits (4 bytes)
 *   char size is     8 bits (1 byte)
 *   address unit is  8 bits (1 byte)
 */

/* ALIGNMENT AND PORTABILITY OPERATORS =============================== */

/* ALIGN ( -- ) Align HERE to cell boundary (4 bytes) */
    head ALIGN_WORD, "ALIGN", docode
        ldr r2, [up, #16]               /* r2 = DP (HERE) */
        adds r2, r2, #3
        bics r2, r2, #3                 /* r2 = (HERE + 3) & ~3 */
        str r2, [up, #16]
        next

/* ALIGNED ( addr -- a-addr ) Align given addr to 4 bytes */
    head ALIGNED_WORD, "ALIGNED", docode
        adds tos, tos, #3
        bics tos, tos, #3
        next

/* CELL ( -- n ) Size of one cell (4 bytes) */
    head CELL_WORD, "CELL", docon
        .long 4

/* CELL+ ( a-addr1 -- a-addr2 ) Add cell size (4 bytes) */
    head CELLPLUS, "CELL+", docode
        adds tos, tos, #4
        next

/* CELLS ( n1 -- n2 ) Cells to address units (* 4) */
    head CELLS_WORD, "CELLS", docode
        lsls tos, tos, #2
        next

/* CHAR+ ( c-addr1 -- c-addr2 ) Add char size (1 byte) */
    head CHARPLUS, "CHAR+", docode
        adds tos, tos, #1
        next

/* CHARS ( n1 -- n2 ) Chars to address units (* 1) */
    head CHARS_WORD, "CHARS", docode
        next

/* >BODY ( xt -- a-addr ) Address of parameter field
 * In DTC on Thumb-2:
 * XT is (CFA | 1) with Thumb mode bit set.
 * CFA is 4 bytes (BL docolon).
 * PFA is at CFA + 4.
 * Therefore: PFA = (XT & ~1) + 4 = XT + 3.
 */
    head TOBODY, ">BODY", docode
        adds tos, tos, #3
        bics tos, tos, #1
        next

/* COMPILE, ( xt -- ) Append execution token to dictionary at HERE */
    head COMMAXT, "COMPILE,", docolon
        .long COMMA_WORD, EXIT_XT

/* !CF ( target_adrs cfa -- ) Set code action of a word in RAM
 * For RAM words:
 * Writes a 32-bit Thumb BL instruction pointing to the appropriate RAM trampoline.
 * Trampolines at 0x20001000:
 *   0x20001000 : ram_docolon
 *   0x20001008 : ram_docon
 *   0x20001010 : ram_dovar
 *   0x20001018 : ram_douser
 *   0x20001020 : ram_dodoes
 */
    head STORECF, "!CF", docode
        ldr t1, [psp], #4      /* t1 = target action */
        bic tos, tos, #1       /* ensure cfa is halfword aligned */

        /* Determine which RAM trampoline to target based on action */
        mov32 t2, docolon
        cmp t1, t2
        bne 11f
        mov32 r2, 0x20001000   /* ram_docolon */
        b 10f

11:     mov32 t2, docon
        cmp t1, t2
        bne 12f
        mov32 r2, 0x20001008   /* ram_docon */
        b 10f

12:     mov32 t2, dovar
        cmp t1, t2
        bne 13f
        mov32 r2, 0x20001010   /* ram_dovar */
        b 10f

13:     mov32 t2, docreate
        cmp t1, t2
        bne 14f
        mov32 r2, 0x20001010   /* ram_dovar */
        b 10f

14:     mov32 t2, douser
        cmp t1, t2
        bne 15f
        mov32 r2, 0x20001018   /* ram_douser */
        b 10f

15:     mov32 t2, dodoes
        cmp t1, t2
        bne 16f
        mov32 r2, 0x20001020   /* ram_dodoes */
        b 10f

16:     mov r2, t1             /* direct target */

10:
        /* Encode 32-bit Thumb BL instruction from 'tos' (cfa) to 'r2' (target)
         * Offset = target - (PC + 4) = r2 - (tos + 4)
         */
        adds r3, tos, #4
        subs r3, r2, r3        /* r3 = offset in bytes */
        asrs r3, r3, #1        /* r3 = halfword offset (24-bit signed) */

        /* Thumb-2 BL encoding:
         * hw1 = 0xF000 | ((offset >> 11) & 0x07FF)
         * hw2 = 0xD000 | (1 << 13) | (1 << 11) | (offset & 0x07FF)
         */
        ubfx r8, r3, #11, #11  /* extract 11 bits starting at bit 11 */
        orr.w r8, r8, #0xF000  /* hw1 */

        ubfx r9, r3, #0, #11   /* extract 11 bits starting at bit 0 */
        orr.w r9, r9, #0xD000
        orr.w r9, r9, #(1 << 13) /* J1 = 1 */
        orr.w r9, r9, #(1 << 11) /* J2 = 1 */ /* hw2 */

        /* Store two halfwords (little endian: hw1 at cfa, hw2 at cfa+2) */
        strh r8, [tos]
        strh r9, [tos, #2]

        /* Pipeline / cache sync */
        dsb
        isb

        ldr tos, [psp], #4
        next

/* ,CF ( adrs -- ) Append a code field at HERE */
    head COMMACF, ",CF", docolon
        .long HERE_WORD, STORECF, LIT_XT, 4, ALLOT_WORD, EXIT_XT

/* !COLON ( -- ) Change code field to docolon */
    head STORCOLON, "!COLON", docolon
        .long LIT_XT, -4, ALLOT_WORD
        .long LIT_XT, docolon, COMMACF, EXIT_XT

/* ,EXIT ( -- ) Append high-level EXIT action */
    head CEXIT, ",EXIT", docolon
        .long LIT_XT, EXIT_XT, COMMAXT, EXIT_XT

/* CONTROL STRUCTURES =============================================== */

/* ,BRANCH ( xt -- ) Append a branch instruction */
    head COMMABRANCH, ",BRANCH", docode
        ldr t2, [up, #16]               /* HERE */
        str tos, [t2], #4
        str t2, [up, #16]
        ldr tos, [psp], #4
        next

/* ,DEST ( dest -- ) Append a branch destination address */
    head COMMADEST, ",DEST", docode
        ldr t2, [up, #16]               /* HERE */
        str tos, [t2], #4
        str t2, [up, #16]
        ldr tos, [psp], #4
        next

/* !DEST ( dest adrs -- ) Change a branch destination */
    head STOREDEST, "!DEST", docode
        ldr t1, [psp], #4               /* dest */
        str t1, [tos]                   /* store at adrs */
        ldr tos, [psp], #4
        next

/* FLUSH-CACHE ( -- ) Flush ARM Cortex-M instruction pipeline */
    head FLUSH_CACHE, "FLUSH-CACHE", docode
        dsb
        isb
        next
