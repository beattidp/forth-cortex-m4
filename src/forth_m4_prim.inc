/* forth_m4_prim.inc -- Core Primitives for ARM Cortex-M4
 * Ported from CAML382.s (CamelForth 32-bit by Douglas Beattie Jr. / Brad Rodriguez)
 */

/* Forth Cold Startup Routine (Called from C main) */
    .global forth_cold
    .thumb_func
forth_cold:
    /* 1. Copy RAM Trampolines (40 bytes) to SRAM 0x20001000 */
    mov32 r0, ram_trampolines_src
    mov32 r1, 0x20001000
    movs r2, #10
1:  ldr r3, [r0], #4
    str r3, [r1], #4
    subs r2, r2, #1
    bne 1b

    /* 2. Initialize User Area at 0x20001040 from UINIT table */
    mov32 up, 0x20001040
    mov32 r0, uinit_data
    mov r1, up
    movs r2, #16
2:  ldr r3, [r0], #4
    str r3, [r1], #4
    subs r2, r2, #1
    bne 2b

    /* 3. Initialize Parameter and Return Stacks */
    mov32 psp, 0x20001800     /* Parameter stack grows down */
    mov32 rsp, 0x20002000     /* Return stack grows down */
    movs tos, #0

    /* 4. Flush instruction/data barriers */
    dsb
    isb

    /* 5. Start high-level COLD execution */
    mov32 f_ip, cold_boot_thread
    next

    .balign 4
cold_boot_thread:
    .long COLD_XT

/* DEFINING WORD RUNTIMES =========================================== */

    .global docolon
    .thumb_func
docolon:
    str f_ip, [rsp, #-4]!
    bic f_ip, lr, #1
    ldr w, [f_ip], #4
    bx  w

    .global docon
    .thumb_func
docon:
    str tos, [psp, #-4]!
    bic lr, lr, #1
    ldr tos, [lr]
    ldr w, [f_ip], #4
    bx  w

    .global docreate
    .thumb_func
docreate:
    .global dovar
    .thumb_func
dovar:
    str tos, [psp, #-4]!
    bic tos, lr, #1
    ldr w, [f_ip], #4
    bx  w

    .global douser
    .thumb_func
douser:
    str tos, [psp, #-4]!
    bic lr, lr, #1
    ldr t1, [lr]
    add tos, up, t1
    ldr w, [f_ip], #4
    bx  w

    .global dodoes
    .thumb_func
dodoes:
    str f_ip, [rsp, #-4]!
    bic f_ip, lr, #1
    pop {t1}
    bic t1, t1, #1
    str tos, [psp, #-4]!
    mov tos, t1
    ldr w, [f_ip], #4
    bx  w

/* RAM Trampoline Table Source Image (Flash) */
    .balign 4
ram_trampolines_src:
    ldr.w pc, [pc]
    .long docolon
    ldr.w pc, [pc]
    .long docon
    ldr.w pc, [pc]
    .long dovar
    ldr.w pc, [pc]
    .long douser
    ldr.w pc, [pc]
    .long dodoes

/* INTERPRETER LOGIC ================================================ */

/* EXIT ( -- ) Exit a colon definition */
    head EXIT, "EXIT", docode
exit_word:
    ldr f_ip, [rsp], #4
    next

/* lit ( -- x ) Fetch inline 32-bit literal to stack */
    head LIT, "lit", docode
lit_word:
    str tos, [psp, #-4]!
    ldr tos, [f_ip], #4
    next

/* EXECUTE ( i*x xt -- j*x ) Execute Forth word at xt */
    head EXECUTE, "EXECUTE", docode
execute_word:
    mov w, tos
    ldr tos, [psp], #4
    bx  w

/* STACK MANIPULATION =============================================== */

/* DUP ( x -- x x ) */
    head DUP, "DUP", docode
        str tos, [psp, #-4]!
        next

/* ?DUP ( x -- 0 | x x ) */
    head QDUP, "?DUP", docode
        cmp tos, #0
        beq 1f
        str tos, [psp, #-4]!
1:      next

/* DROP ( x -- ) */
    head DROP, "DROP", docode
        ldr tos, [psp], #4
        next

/* SWAP ( x1 x2 -- x2 x1 ) */
    head SWOP, "SWAP", docode
        ldr t1, [psp]
        str tos, [psp]
        mov tos, t1
        next

/* OVER ( x1 x2 -- x1 x2 x1 ) */
    head OVERR, "OVER", docode
        str tos, [psp, #-4]!
        ldr tos, [psp, #4]
        next

/* ROT ( x1 x2 x3 -- x2 x3 x1 ) */
    head ROT, "ROT", docode
        ldr t1, [psp]          /* x2 */
        ldr t2, [psp, #4]      /* x1 */
        str t1, [psp, #4]      /* x2 to bottom */
        str tos, [psp]         /* x3 to middle */
        mov tos, t2            /* x1 to TOS */
        next

/* 2DUP ( x1 x2 -- x1 x2 x1 x2 ) */
    head TWODUP, "2DUP", docode
        ldr t1, [psp]
        str tos, [psp, #-4]!
        str t1, [psp, #-4]!
        next

/* 2DROP ( x1 x2 -- ) */
    head TWODROP, "2DROP", docode
        ldr tos, [psp, #4]
        adds psp, psp, #8
        next

/* 2SWAP ( x1 x2 x3 x4 -- x3 x4 x1 x2 ) */
    head TWOSWAP, "2SWAP", docode
        ldr t1, [psp]          /* x3 */
        ldr t2, [psp, #4]      /* x2 */
        ldr r8, [psp, #8]      /* x1 */
        str tos, [psp, #4]
        str t1, [psp, #8]
        str r8, [psp]
        mov tos, t2
        next

/* RETURN STACK ===================================================== */

/* >R ( x -- ) ( R: -- x ) */
    head TOR, ">R", docode
        str tos, [rsp, #-4]!
        ldr tos, [psp], #4
        next

/* R> ( -- x ) ( R: x -- ) */
    head RFROM, "R>", docode
        str tos, [psp, #-4]!
        ldr tos, [rsp], #4
        next

/* R@ ( -- x ) ( R: x -- x ) */
    head RFETCH, "R@", docode
        str tos, [psp, #-4]!
        ldr tos, [rsp]
        next

/* MEMORY ACCESS ==================================================== */

/* @ ( a-addr -- x ) Fetch cell from memory */
    head FETCH, "@", docode
        ldr tos, [tos]
        next

/* ! ( x a-addr -- ) Store cell to memory */
    head STORE, "!", docode
        ldr t1, [psp], #4
        str t1, [tos]
        ldr tos, [psp], #4
        next

/* C@ ( c-addr -- char ) Fetch byte from memory */
    head CFETCH, "C@", docode
        ldrb tos, [tos]
        next

/* C! ( char c-addr -- ) Store byte to memory */
    head CSTORE, "C!", docode
        ldr t1, [psp], #4
        strb t1, [tos]
        ldr tos, [psp], #4
        next

/* +! ( n a-addr -- ) Add n to contents of a-addr */
    head PLUSSTORE, "+!", docode
        ldr t1, [psp], #4
        ldr t2, [tos]
        adds t2, t2, t1
        str t2, [tos]
        ldr tos, [psp], #4
        next

/* 2@ ( a-addr -- x1 x2 ) Fetch two cells */
    head TWOFETCH, "2@", docode
        ldr t1, [tos, #4]
        ldr t2, [tos]
        str t2, [psp, #-4]!
        mov tos, t1
        next

/* 2! ( x1 x2 a-addr -- ) Store two cells */
    head TWOSTORE, "2!", docode
        ldr t1, [psp], #4      /* x2 */
        ldr t2, [psp], #4      /* x1 */
        str t2, [tos]
        str t1, [tos, #4]
        ldr tos, [psp], #4
        next

/* ARITHMETIC & LOGICAL OPERATORS =================================== */

/* + ( n1 n2 -- n3 ) */
    head PLUS, "+", docode
        ldr t1, [psp], #4
        adds tos, t1, tos
        next

/* - ( n1 n2 -- n1-n2 ) */
    head MINUS, "-", docode
        ldr t1, [psp], #4
        subs tos, t1, tos
        next

/* 1+ ( n1 -- n2 ) */
    head ONEPLUS, "1+", docode
        adds tos, tos, #1
        next

/* 1- ( n1 -- n2 ) */
    head ONEMINUS, "1-", docode
        subs tos, tos, #1
        next

/* 2+ ( n1 -- n2 ) */
    head TWOPLUS, "2+", docode
        adds tos, tos, #2
        next

/* 2- ( n1 -- n2 ) */
    head TWOMINUS, "2-", docode
        subs tos, tos, #2
        next

/* 2* ( n1 -- n2 ) */
    head TWOSTAR, "2*", docode
        lsls tos, tos, #1
        next

/* 2/ ( n1 -- n2 ) */
    head TWOSLASH, "2/", docode
        asrs tos, tos, #1
        next

/* NEGATE ( n1 -- -n1 ) */
    head NEGATE, "NEGATE", docode
        negs tos, tos
        next

/* INVERT ( x1 -- x2 ) Bitwise NOT */
    head INVERT, "INVERT", docode
        mvns tos, tos
        next

/* AND ( x1 x2 -- x3 ) Bitwise AND */
    head ANDD, "AND", docode
        ldr t1, [psp], #4
        ands tos, tos, t1
        next

/* OR ( x1 x2 -- x3 ) Bitwise OR */
    head ORR_WORD, "OR", docode
        ldr t1, [psp], #4
        orrs tos, tos, t1
        next

/* XOR ( x1 x2 -- x3 ) Bitwise XOR */
    head XORR, "XOR", docode
        ldr t1, [psp], #4
        eors tos, tos, t1
        next

/* LSHIFT ( x1 u -- x2 ) */
    head LSHIFT, "LSHIFT", docode
        ldr t1, [psp], #4
        lsl tos, t1, tos
        next

/* RSHIFT ( x1 u -- x2 ) */
    head RSHIFT, "RSHIFT", docode
        ldr t1, [psp], #4
        lsr tos, t1, tos
        next

/* COMPARISONS ====================================================== */

/* 0= ( x -- flag ) */
    head ZEROEQUAL, "0=", docode
        cmp tos, #0
        ite eq
        moveq tos, #-1
        movne tos, #0
        next

/* 0< ( n -- flag ) */
    head ZEROLESS, "0<", docode
        asrs tos, tos, #31
        next

/* 0> ( n -- flag ) */
    head ZEROGREATER, "0>", docode
        cmp tos, #0
        ite gt
        movgt tos, #-1
        movle tos, #0
        next

/* = ( x1 x2 -- flag ) */
    head EQUAL, "=", docode
        ldr t1, [psp], #4
        cmp t1, tos
        ite eq
        moveq tos, #-1
        movne tos, #0
        next

/* < ( n1 n2 -- flag ) */
    head LESS, "<", docode
        ldr t1, [psp], #4
        cmp t1, tos
        ite lt
        movlt tos, #-1
        movge tos, #0
        next

/* > ( n1 n2 -- flag ) */
    head GREATER, ">", docode
        ldr t1, [psp], #4
        cmp t1, tos
        ite gt
        movgt tos, #-1
        movle tos, #0
        next

/* U< ( u1 u2 -- flag ) */
    head ULESS, "U<", docode
        ldr t1, [psp], #4
        cmp t1, tos
        ite lo
        movlo tos, #-1
        movhs tos, #0
        next

/* MULTIPLICATION & DIVISION ======================================== */

/* * ( n1 n2 -- n3 ) */
    head STAR, "*", docode
        ldr t1, [psp], #4
        muls tos, t1, tos
        next

/* UM* ( u1 u2 -- ud ) */
    head UMSTAR, "UM*", docode
        ldr t1, [psp]
        umull t1, tos, t1, tos
        str t1, [psp]
        next

/* M* ( n1 n2 -- d ) */
    head MSTAR, "M*", docode
        ldr t1, [psp]
        smull t1, tos, t1, tos
        str t1, [psp]
        next

/* UM/MOD ( ud u1 -- urem uquot ) Unsigned 64/32 division */
    head UMSLASHMOD, "UM/MOD", docode
        ldr r2, [psp]          /* d_hi */
        ldr r3, [psp, #4]      /* d_lo */
        movs r8, #32
1:      lsls r3, r3, #1
        adcs r2, r2, r2
        cmp r2, tos
        blo 2f
        subs r2, r2, tos
        adds r3, r3, #1
2:      subs r8, r8, #1
        bne 1b

        str r2, [psp, #4]      /* urem */
        adds psp, psp, #4
        mov tos, r3            /* uquot */
        next

/* FM/MOD ( d n1 -- rem quot ) Floored 64/32 division */
    head FMSLASHMOD, "FM/MOD", docode
        ldr r2, [psp]          /* d_hi */
        ldr r3, [psp, #4]      /* d_lo */
        mov r8, r2             /* sign of dividend */
        eor r9, r2, tos        /* sign of quotient */

        cmp tos, #0
        bpl 1f
        negs tos, tos
1:      cmp r2, #0
        bpl 2f
        negs r3, r3
        sbc r2, r2, r2
        negs r2, r2
2:      movs r12, #32
3:      lsls r3, r3, #1
        adcs r2, r2, r2
        cmp r2, tos
        blo 4f
        subs r2, r2, tos
        adds r3, r3, #1
4:      subs r12, r12, #1
        bne 3b

        cmp r8, #0
        bpl 5f
        negs r2, r2
5:      cmp r9, #0
        bpl 6f
        negs r3, r3
6:      cmp r2, #0
        beq 7f
        cmp r9, #0
        bpl 7f
        subs r3, r3, #1
        adds r2, r2, tos
7:      str r2, [psp, #4]
        adds psp, psp, #4
        mov tos, r3
        next

/* SM/REM ( d n1 -- rem quot ) Symmetric 64/32 division */
    head SMSLASHREM, "SM/REM", docode
        ldr r2, [psp]          /* d_hi */
        ldr r3, [psp, #4]      /* d_lo */
        mov r8, r2             /* dividend sign */
        eor r9, r2, tos        /* quotient sign */

        cmp tos, #0
        bpl 1f
        negs tos, tos
1:      cmp r2, #0
        bpl 2f
        negs r3, r3
        sbc r2, r2, r2
        negs r2, r2
2:      movs r12, #32
3:      lsls r3, r3, #1
        adcs r2, r2, r2
        cmp r2, tos
        blo 4f
        subs r2, r2, tos
        adds r3, r3, #1
4:      subs r12, r12, #1
        bne 3b

        cmp r8, #0
        bpl 5f
        negs r2, r2
5:      cmp r9, #0
        bpl 6f
        negs r3, r3
6:      str r2, [psp, #4]
        adds psp, psp, #4
        mov tos, r3
        next

/* CONTROL FLOW PRIMITIVES ========================================== */

/* branch ( -- ) Inline unconditional branch */
    head BRANCH, "branch", docode
branch_word:
    ldr f_ip, [f_ip]
    next

/* ?branch ( x -- ) Inline branch if zero */
    head QBRANCH, "?branch", docode
qbranch_word:
    cmp tos, #0
    ite eq
    ldreq f_ip, [f_ip]
    addne f_ip, f_ip, #4
    ldr tos, [psp], #4
    next

/* (do) ( limit init -- ) ( R: -- fudge index ) */
    head XDO, "(do)", docode
        ldr t1, [psp], #4      /* t1 = limit */
        mov32 t2, 0x80000000
        subs t2, t2, t1        /* t2 = fudge = 0x80000000 - limit */
        adds tos, tos, t2      /* tos = index = init + fudge */
        str t2, [rsp, #-4]!    /* push fudge */
        str tos, [rsp, #-4]!   /* push index */
        ldr tos, [psp], #4     /* pop new TOS */
        next

/* (loop) ( R: fudge index -- | fudge index' ) */
    head XLOOP, "(loop)", docode
        ldr t1, [rsp]          /* t1 = index */
        adds t1, t1, #1
        bvs 1f
        /* Loop continues */
        str t1, [rsp]
        ldr f_ip, [f_ip]
        next
1:      /* Loop terminates */
        adds rsp, rsp, #8
        adds f_ip, f_ip, #4
        next

/* (+loop) ( n -- ) ( R: fudge index -- | fudge index' ) */
    head XPLUSLOOP, "(+loop)", docode
        ldr t1, [rsp]          /* t1 = index */
        adds t1, t1, tos
        bvs 1f
        /* Loop continues */
        str t1, [rsp]
        ldr f_ip, [f_ip]
        ldr tos, [psp], #4
        next
1:      /* Loop terminates */
        adds rsp, rsp, #8
        adds f_ip, f_ip, #4
        ldr tos, [psp], #4
        next

/* I ( -- n ) Innermost loop index */
    head II, "I", docode
        str tos, [psp, #-4]!
        ldr t1, [rsp]
        ldr t2, [rsp, #4]
        subs tos, t1, t2
        next

/* J ( -- n ) Second loop index */
    head JJ, "J", docode
        str tos, [psp, #-4]!
        ldr t1, [rsp, #8]
        ldr t2, [rsp, #12]
        subs tos, t1, t2
        next

/* UNLOOP ( -- ) ( R: sys1 sys2 -- ) */
    head UNLOOP_WORD, "UNLOOP", docode
        adds rsp, rsp, #8
        next

/* CONSOLE I/O PRIMITIVES =========================================== */

/* EMIT ( c -- ) Output character to console */
    head EMIT, "EMIT", docode
        push {r0-r3, lr}
        mov r0, tos
        bl forth_tx_char
        pop {r0-r3, lr}
        ldr tos, [psp], #4
        next

/* KEY? ( -- flag ) Check if character ready */
    head QUERYKEY, "KEY?", docode
        str tos, [psp, #-4]!
        push {r1-r3, lr}
        bl forth_rx_ready
        pop {r1-r3, lr}
        mov tos, r0
        next

/* KEY ( -- c ) Receive character */
    head KEY, "KEY", docode
        str tos, [psp, #-4]!
        push {r1-r3, lr}
        bl forth_rx_char
        pop {r1-r3, lr}
        mov tos, r0
        next

/* UD* ( ud u -- ud' ) 64-bit unsigned multiply ud' = ud * u */
    head UDSTAR, "UD*", docode
        ldr t1, [psp], #4      /* t1 = ud_hi */
        ldr t2, [psp]          /* t2 = ud_lo */
        umull r8, r9, t2, tos  /* r8 = lo', r9 = carry */
        mul t1, t1, tos
        adds t1, t1, r9        /* t1 = hi' */
        str r8, [psp]
        mov tos, t1
        next

/* DIGIT? ( char -- n flag ) Convert ASCII char to digit in current BASE */
    head DIGITQ, "DIGIT?", docode
        subs tos, tos, #'0'
        blo 2f
        cmp tos, #9
        bls 1f
        subs tos, tos, #('A' - '0')
        blo 2f
        adds tos, tos, #10
1:      ldr t1, [up, #8]       /* BASE is at UP offset 8 (2 * 4) */
        cmp tos, t1
        bhs 2f
        str tos, [psp, #-4]!
        movs tos, #1
        rsbs tos, tos, #0      /* flag = -1 */
        next
2:      str tos, [psp, #-4]!
        movs tos, #0           /* flag = 0 */
        next

/* ?SIGN ( c-addr u -- c-addr' u' f ) Check for leading '+' or '-' sign */
    head QSIGN, "?SIGN", docode
        cmp tos, #0
        beq 2f
        ldr t1, [psp]          /* c-addr */
        ldrb r8, [t1]
        cmp r8, #'-'
        bne 1f
        /* Negative sign found */
        adds t1, t1, #1
        subs tos, tos, #1
        str t1, [psp]
        str tos, [psp, #-4]!
        movs tos, #1
        rsbs tos, tos, #0      /* f = -1 */
        next
1:      cmp r8, #'+'
        bne 2f
        /* Positive sign found */
        adds t1, t1, #1
        subs tos, tos, #1
        str t1, [psp]
2:      /* No sign found */
        str tos, [psp, #-4]!
        movs tos, #0           /* f = 0 */
        next

/* BLOCK MEMORY & STRING PRIMITIVES ================================= */

/* CMOVE ( c-addr1 c-addr2 u -- ) Forward byte copy */
    head CMOVE, "CMOVE", docode
        ldr t1, [psp], #4      /* dst */
        ldr t2, [psp], #4      /* src */
        cmp tos, #0
        ble 2f
1:      ldrb r8, [t2], #1
        strb r8, [t1], #1
        subs tos, tos, #1
        bne 1b
2:      ldr tos, [psp], #4
        next

/* CMOVE> ( c-addr1 c-addr2 u -- ) Backward byte copy */
    head CMOVEUP, "CMOVE>", docode
        ldr t1, [psp], #4      /* dst */
        ldr t2, [psp], #4      /* src */
        cmp tos, #0
        ble 2f
        add t1, t1, tos
        add t2, t2, tos
1:      ldrb r8, [t2, #-1]!
        strb r8, [t1, #-1]!
        subs tos, tos, #1
        bne 1b
2:      ldr tos, [psp], #4
        next

/* FILL ( c-addr u char -- ) Fill memory with char */
    head FILL, "FILL", docode
        ldr t1, [psp], #4      /* u */
        ldr t2, [psp], #4      /* c-addr */
        cmp t1, #0
        beq 2f
1:      strb tos, [t2], #1
        subs t1, t1, #1
        bne 1b
2:      ldr tos, [psp], #4
        next

/* SKIP ( c-addr u char -- c-addr' u' ) Skip leading matching chars */
    head SKIP, "SKIP", docode
        ldr t1, [psp]          /* u */
        ldr t2, [psp, #4]      /* c-addr */
        cmp t1, #0
        beq 2f
1:      ldrb r8, [t2]
        cmp r8, tos
        bne 2f
        adds t2, t2, #1
        subs t1, t1, #1
        bne 1b
2:      str t2, [psp, #4]      /* updated c-addr */
        mov tos, t1            /* updated u in TOS */
        adds psp, psp, #4
        next

/* SCAN ( c-addr u char -- c-addr' u' ) Scan for first matching char */
    head SCAN, "SCAN", docode
        ldr t1, [psp]          /* u */
        ldr t2, [psp, #4]      /* c-addr */
        cmp t1, #0
        beq 2f
1:      ldrb r8, [t2]
        cmp r8, tos
        beq 2f
        adds t2, t2, #1
        subs t1, t1, #1
        bne 1b
2:      str t2, [psp, #4]
        mov tos, t1
        adds psp, psp, #4
        next

/* S= ( c-addr1 c-addr2 u -- n ) String compare */
    head SEQUAL, "S=", docode
        ldr t1, [psp], #4      /* c-addr2 */
        ldr t2, [psp], #4      /* c-addr1 */
        cmp tos, #0
        beq 3f                 /* length 0 -> equal */
1:      ldrb r8, [t2], #1
        ldrb r9, [t1], #1
        cmp r8, r9
        bne 2f
        subs tos, tos, #1
        bne 1b
3:      movs tos, #0           /* equal */
        next
2:      blo 4f
        movs tos, #1           /* s1 > s2 */
        next
4:      movs tos, #-1          /* s1 < s2 */
        next

/* SYSTEM =========================================================== */

/* BYE ( -- ) Return to C runtime or loop */
    head BYE, "BYE", docode
1:      b 1b
