/*  Forth Kernel, 32-bit, for ARM Cortex M4
 *  ..based on CamelForth for the Z80382  ("Z382")
 *  portions (C) 2003 Douglas Beattie Jr.
 * 
 *  (C) 1994 Bradford J. Rodriguez
 *  Permission is granted to freely copy, modify,
 *  and distribute this program for personal or
 *  educational use. Commercial inquiries should
 *  be directed to the author at 115 First St.,
 *  # 105, Collingwood, Ontario L9Y 4W3, Canada
 */
 
/* forth_m4_high.S -- High-Level Forth Kernel Words for ARM Cortex-M4
 * Ported from CAML382h.S (CamelForth 32-bit by Douglas Beattie Jr. / Brad Rodriguez)
 */

/* SYSTEM VARIABLES & CONSTANTS ===================================== */

/* BL ( -- char ) */
    head BL_WORD, "BL", docon
        .long 0x20

/* TRUE ( -- flag ) */
    head TRUE_WORD, "TRUE", docon
        .long -1

/* FALSE ( -- flag ) */
    head FALSE_WORD, "FALSE", docon
        .long 0

/* TIBSIZE ( -- n ) */
    head TIBSIZE_WORD, "TIBSIZE", docon
        .long 256

/* TIB ( -- a-addr ) */
    head TIB_WORD, "TIB", docon
        .long 0x20001100

/* U0 ( -- a-addr ) User area base address */
    head U0_WORD, "U0", douser
        .long 0 * 4

/* >IN ( -- a-addr ) Offset into TIB */
    head TOIN, ">IN", douser
        .long 1 * 4

/* BASE ( -- a-addr ) Number conversion radix */
    head BASE_WORD, "BASE", douser
        .long 2 * 4

/* STATE ( -- a-addr ) Compiler state */
    head STATE_WORD, "STATE", douser
        .long 3 * 4

/* DP ( -- a-addr ) Dictionary pointer */
    head DP_WORD, "DP", douser
        .long 4 * 4

/* 'SOURCE ( -- a-addr ) Two cells: len, adrs */
    head TICKSOURCE, "'SOURCE", douser
        .long 5 * 4

/* LATEST ( -- a-addr ) Last word in dictionary */
    head LATEST_WORD, "LATEST", douser
        .long 7 * 4

/* HP ( -- a-addr ) HOLD pointer */
    head HP_WORD, "HP", douser
        .long 8 * 4

/* LP ( -- a-addr ) Leave-stack pointer */
    head LP_WORD, "LP", douser
        .long 9 * 4

/* S0 ( -- a-addr ) Parameter stack top */
    head S0_WORD, "S0", docon
        .long 0x20001800

/* PAD ( -- a-addr ) PAD buffer */
    head PAD_WORD, "PAD", docon
        .long 0x20001200

/* L0 ( -- a-addr ) Leave stack bottom */
    head L0_WORD, "L0", docon
        .long 0x20001600

/* R0 ( -- a-addr ) Return stack top */
    head R0_WORD, "R0", docon
        .long 0x20002000

/* UINIT ( -- addr ) Initial values for user area */
    head UINIT_WORD, "UINIT", docon
        .long uinit_data

/* #INIT ( -- n ) # bytes of user area init data */
    head NINIT, "#INIT", docon
        .long 16 * 4

/* ARITHMETIC OPERATORS ============================================= */

/* S>D ( n -- d ) Single to double precision */
    head STOD, "S>D", docolon
        .long DUP, ZEROLESS, EXIT

/* ?NEGATE ( n1 n2 -- n3 ) Negate n1 if n2 is negative */
    head QNEGATE, "?NEGATE", docolon
        .long ZEROLESS, QBRANCH, 1f, NEGATE
1:      .long EXIT

/* ABS ( n1 -- +n2 ) Absolute value */
    head ABS_WORD, "ABS", docolon
        .long DUP, QNEGATE, EXIT

/* DNEGATE ( d1 -- d2 ) Negate double precision */
    head DNEGATE, "DNEGATE", docolon
        .long SWOP, INVERT, SWOP, INVERT, LIT, 1, MPLUS, EXIT

/* ?DNEGATE ( d1 n -- d2 ) Negate d1 if n negative */
    head QDNEGATE, "?DNEGATE", docolon
        .long ZEROLESS, QBRANCH, 1f, DNEGATE
1:      .long EXIT

/* D+ ( d1 d2 -- d3 ) Double addition */
    head DPLUS, "D+", docode
        /* d2 = [psp], tos ; d1 = [psp,#8], [psp,#4] */
        ldr r2, [psp]          /* d2_lo */
        ldr r3, [psp, #4]      /* d1_hi */
        ldr r8, [psp, #8]      /* d1_lo */
        adds r8, r8, r2        /* lo result */
        adcs tos, r3, tos      /* hi result */
        str r8, [psp, #8]
        adds psp, psp, #8
        next

/* D- ( d1 d2 -- d3 ) Double subtraction */
    head DMINUS, "D-", docolon
        .long DNEGATE, DPLUS, EXIT

/* M+ ( d n -- d' ) Double + single */
    head MPLUS, "M+", docolon
        .long STOD, DPLUS, EXIT

/* MIN ( n1 n2 -- n3 ) */
    head MIN_WORD, "MIN", docolon
        .long TWODUP, GREATER, QBRANCH, 1f, SWOP
1:      .long DROP, EXIT

/* MAX ( n1 n2 -- n3 ) */
    head MAX_WORD, "MAX", docolon
        .long TWODUP, LESS, QBRANCH, 1f, SWOP
1:      .long DROP, EXIT

/* D< ( d1 d2 -- flag ) Double signed comparison */
    head DLESS, "D<", docode
        ldr r2, [psp]          /* d2_lo */
        ldr r3, [psp, #4]      /* d1_hi */
        ldr r8, [psp, #8]      /* d1_lo */
        cmp r3, tos            /* compare high words */
        bne 1f
        cmp r8, r2            /* compare low words unsigned */
        blo 2f
        b 3f
1:      blt 2f
3:      movs tos, #0           /* false */
        b 4f
2:      movs tos, #-1          /* true */
4:      adds psp, psp, #12
        next

@ M*/ ( d1 n1 n2 -- d2 )
    head MSTARSLASH, "M*/", docolon
        .long TOR, TWODUP, XORR, TOR, QDNEGATE
        .long RFETCH, QNEGATE
        .long EXIT

/* /MOD ( n1 n2 -- rem quot ) */
    head SLASHMOD, "/MOD", docolon
        .long TOR, STOD, RFROM, SMSLASHREM, EXIT

/* / ( n1 n2 -- quot ) */
    head SLASH, "/", docolon
        .long SLASHMOD, NIP, EXIT

/* MOD ( n1 n2 -- rem ) */
    head MOD_WORD, "MOD", docolon
        .long SLASHMOD, DROP, EXIT

@ */MOD ( n1 n2 n3 -- rem quot )
    head STARSLASHMOD, "*/MOD", docolon
        .long TOR, MSTAR, RFROM, SMSLASHREM, EXIT

@ */ ( n1 n2 n3 -- quot )
    head STARSLASH, "*/", docolon
        .long STARSLASHMOD, NIP, EXIT

/* NIP ( x1 x2 -- x2 ) */
    head NIP, "NIP", docode
        adds psp, psp, #4
        next

/* TUCK ( x1 x2 -- x2 x1 x2 ) */
    head TUCK, "TUCK", docolon
        .long SWOP, OVERR, EXIT

/* NUMERIC OUTPUT =================================================== */

/* HEX ( -- ) Set BASE to 16 */
    head HEX, "HEX", docolon
        .long LIT, 16, BASE_WORD, STORE, EXIT

/* DECIMAL ( -- ) Set BASE to 10 */
    head DECIMAL, "DECIMAL", docolon
        .long LIT, 10, BASE_WORD, STORE, EXIT

/* COUNT ( c-addr1 -- c-addr2 u ) */
    head COUNT, "COUNT", docolon
        .long DUP, CHARPLUS, SWOP, CFETCH, EXIT

/* HERE ( -- a-addr ) */
    head HERE_WORD, "HERE", docolon
        .long DP_WORD, FETCH, EXIT

/* ALLOT ( n -- ) */
    head ALLOT_WORD, "ALLOT", docolon
        .long DP_WORD, PLUSSTORE, EXIT

/* , ( x -- ) Append cell to dictionary */
    head COMMA_WORD, ",", docolon
        .long HERE_WORD, STORE, LIT, 4, ALLOT_WORD, EXIT

/* C, ( char -- ) Append char to dictionary */
    head CCOMMA, "C,", docolon
        .long HERE_WORD, CSTORE, LIT, 1, ALLOT_WORD, EXIT

/* UD/MOD ( ud1 u2 -- u3 ud4 ) 64/32 -> 32-bit rem, 64-bit quot */
    head UDSLASHMOD, "UD/MOD", docolon
        .long TOR, LIT, 0, RFETCH, UMSLASHMOD, ROT, ROT
        .long RFROM, UMSLASHMOD, ROT, EXIT

/* HOLD ( char -- ) Insert character into numeric buffer */
    head HOLD, "HOLD", docode
        ldr t1, [up, #32]       /* t1 = HP pointer (PAD) */
        subs t1, t1, #1
        str t1, [up, #32]       /* update HP */
        strb tos, [t1]          /* store char byte at HP */
        ldr tos, [psp], #4      /* pop char */
        next

/* <# ( -- ) Initialize formatted numeric output */
    head LESSNUM, "<#", docode
        mov32 t1, 0x20001200    /* PAD buffer */
        str t1, [up, #32]       /* HP = PAD */
        next

/* SIGN ( n -- ) Append minus sign if n is negative */
    head SIGN, "SIGN", docolon
        .long ZEROLESS, QBRANCH, 1f, LIT, '-', HOLD
1:      .long EXIT

/* >digit ( n -- char ) Convert 0..35 to ASCII digit */
    head TODIGIT, ">digit", docolon
        .long DUP, LIT, 9, GREATER, LIT, 7, ANDD, PLUS
        .long LIT, '0', PLUS, EXIT

/* # ( ud1 -- ud2 ) Extract one digit */
    head NUM, "#", docolon
        .long BASE_WORD, FETCH, UDSLASHMOD, ROT, TODIGIT, HOLD, EXIT

/* #S ( ud1 -- ud2 ) Extract all digits */
    head NUMS, "#S", docolon
1:      .long NUM, TWODUP, ORR_WORD, ZEROEQUAL, QBRANCH, 1b, EXIT

/* #> ( ud1 -- c-addr u ) Complete numeric formatting */
    head NUMGREATER, "#>", docode
        ldr t1, [up, #32]       /* c-addr = HP */
        mov32 t2, 0x20001200    /* PAD */
        subs t2, t2, t1         /* u = PAD - HP */
        str t1, [psp]           /* c-addr replaces d_lo */
        mov tos, t2             /* u in tos */
        next

/* U. ( u -- ) Print unsigned number */
    head UDOT, "U.", docolon
        .long LESSNUM, LIT, 0, NUMS, NUMGREATER, TYPE, SPACE_WORD, EXIT

/* . ( n -- ) Print signed number */
    head DOT, ".", docolon
        .long LESSNUM, DUP, ABS_WORD, LIT, 0, NUMS
        .long ROT, SIGN, NUMGREATER, TYPE, SPACE_WORD, EXIT

/* ? ( a-addr -- ) Display contents of address */
    head QUESTION, "?", docolon
        .long FETCH, DOT, EXIT

/* TERMINAL I/O & STRINGS =========================================== */

/* SPACE ( -- ) Emit a space */
    head SPACE_WORD, "SPACE", docolon
        .long BL_WORD, EMIT, EXIT

/* SPACES ( n -- ) Emit n spaces */
    head SPACES_WORD, "SPACES", docolon
        .long LIT, 0, MAX_WORD, QDUP, QBRANCH, 2f
1:      .long SPACE_WORD, ONEMINUS, DUP, QBRANCH, 2f, BRANCH, 1b
2:      .long DROP, EXIT

/* TYPE ( c-addr u -- ) Output string */
    head TYPE, "TYPE", docode
        ldr t1, [psp], #4      /* c-addr */
        cmp tos, #0
        beq 2f
        mov t2, tos            /* t2 = count */
1:      ldrb r0, [t1], #1
        push {r1-r3, lr}
        bl forth_tx_char
        pop {r1-r3, lr}
        subs t2, t2, #1
        bne 1b
2:      ldr tos, [psp], #4
        next

/* CR ( -- ) Output carriage return and newline */
    head CR_WORD, "CR", docolon
        .long LIT, 13, EMIT, LIT, 10, EMIT, EXIT

/* SOURCE ( -- c-addr u ) */
    head SOURCE, "SOURCE", docolon
        .long TICKSOURCE, TWOFETCH, EXIT

/* /STRING ( c-addr1 u1 n -- c-addr2 u2 ) Trim string */
    head SLASHSTRING, "/STRING", docolon
        .long ROT, OVERR, PLUS, ROT, ROT, MINUS, EXIT

/* >COUNTED ( src n dst -- ) Copy to counted string */
    head TOCOUNTED, ">COUNTED", docolon
        .long TWODUP, CSTORE, CHARPLUS, SWOP, CMOVE, EXIT

/* WORD ( char -- c-addr ) Parse next word delimited by char */
    head WORD, "WORD", docolon
        .long DUP, SOURCE, TOIN, FETCH, SLASHSTRING
        .long DUP, TOR, ROT, SKIP
        .long OVERR, TOR, ROT, SCAN
        .long DUP, QBRANCH, 1f, ONEMINUS
1:      .long RFROM, RFROM, ROT, MINUS, TOIN, PLUSSTORE
        .long TUCK, MINUS
        .long HERE_WORD, TOCOUNTED, HERE_WORD
        .long BL_WORD, OVERR, COUNT, PLUS, CSTORE, EXIT

/* (S") ( -- c-addr u ) Runtime for S" string literal */
    head XSQUOTE, "(S\")", docode
        str tos, [psp, #-4]!
        ldrb t1, [f_ip], #1      /* t1 = length */
        str f_ip, [psp, #-4]!    /* c-addr */
        mov tos, t1              /* u */
        adds f_ip, f_ip, t1
        adds f_ip, f_ip, #3
        bics f_ip, f_ip, #3      /* align IP to 4 bytes */
        next

/* S" ( -- ) Compile inline string */
    immed SQUOTE, "S\"", docolon
        .long LIT, XSQUOTE, COMMAXT
        .long LIT, '\"', WORD, CFETCH, ONEPLUS
        .long ALIGNED_WORD, ALLOT_WORD, EXIT

/* ." ( -- ) Compile string to print at runtime */
    immed DOTQUOTE, ".\"", docolon
        .long SQUOTE
        .long LIT, TYPE, COMMAXT, EXIT

/* DICTIONARY LOOKUP & COMPILER ===================================== */

/* NFA>LFA ( nfa -- lfa ) */
    head NFATOLFA, "NFA>LFA", docolon
        .long LIT, 5, MINUS, EXIT

/* NFA>CFA ( nfa -- cfa )
 * Result is aligned to 4 bytes and has Thumb mode bit set (+1)
 */
    head NFATOCFA, "NFA>CFA", docolon
        .long COUNT, LIT, 0x7F, ANDD, PLUS, ALIGNED_WORD, ONEPLUS, EXIT

/* IMMED? ( nfa -- flag ) */
    head IMMEDQ, "IMMED?", docolon
        .long ONEMINUS, CFETCH, EXIT

/* FIND ( c-addr -- c-addr 0 | xt 1 | xt -1 )
 * Searches dictionary from LATEST backward
 */
    head FIND, "FIND", docolon
        .long LATEST_WORD, FETCH
1:      /* stack: c-addr nfa */
        .long DUP, QBRANCH, 4f  /* 0 means reached end of dictionary */
        .long TWODUP, OVERR, CFETCH, CHARPLUS, SEQUAL
        .long ZEROEQUAL, QBRANCH, 2f
        /* Match found! */
        .long NIP, DUP, NFATOCFA, SWOP, IMMEDQ
        .long QBRANCH, 3f
        .long LIT, 1, EXIT      /* immediate word */
3:      .long LIT, -1, EXIT     /* normal word */
2:      /* No match, follow link to previous word */
        .long NFATOLFA, FETCH, BRANCH, 1b
4:      /* Not found */
        .long DROP, LIT, 0, EXIT

/* ' ( -- xt ) Find word in dictionary or abort */
    head TICK, "'", docolon
        .long BL_WORD, WORD, FIND, ZEROEQUAL
        .long XSQUOTE
        .byte 1
        .ascii "?"
        .balign 4
        .long QABORT, EXIT

/* CHAR ( -- char ) Parse ASCII char */
    head CHAR_WORD, "CHAR", docolon
        .long BL_WORD, WORD, CHARPLUS, CFETCH, EXIT

/* [CHAR] ( -- ) Compile char literal */
    immed BRACCHAR, "[CHAR]", docolon
        .long CHAR_WORD, LIT, LIT, COMMAXT, COMMA_WORD, EXIT

/* ( ( -- ) Skip comment until ) */
    immed PAREN, "(", docolon
        .long LIT, ')', WORD, DROP, EXIT

/* .( ( -- ) Print string until ) immediately */
    immed DOTPAREN, ".(", docolon
        .long LIT, ')', WORD, COUNT, TYPE, EXIT

/* \ ( -- ) Skip rest of line */
    immed BACKSLASH, "\\", docolon
        .long TICKSOURCE, CELLPLUS, FETCH, TOIN, STORE, EXIT

/* HIDE ( -- ) Hide latest definition */
    head HIDE, "HIDE", docolon
        .long LATEST_WORD, FETCH, DUP, CFETCH, LIT, 0x80, ORR_WORD
        .long SWOP, CSTORE, EXIT

/* REVEAL ( -- ) Reveal latest definition */
    head REVEAL, "REVEAL", docolon
        .long LATEST_WORD, FETCH, DUP, CFETCH, LIT, 0x7F, ANDD
        .long SWOP, CSTORE, EXIT

/* IMMEDIATE ( -- ) Make last definition immediate */
    head IMMEDIATE, "IMMEDIATE", docolon
        .long LIT, 1, LATEST_WORD, FETCH, ONEMINUS, CSTORE, EXIT

/* [ ( -- ) Enter interpretive mode */
    immed LEFTBRACKET, "[", docolon
        .long LIT, 0, STATE_WORD, STORE, EXIT

/* ] ( -- ) Enter compiling mode */
    head RIGHTBRACKET, "]", docolon
        .long LIT, -1, STATE_WORD, STORE, EXIT

/* LITERAL ( x -- ) Compile literal if in compile state */
    immed LITERAL, "LITERAL", docolon
        .long STATE_WORD, FETCH, QBRANCH, 1f
        .long LIT, LIT, COMMAXT, COMMA_WORD
1:      .long EXIT

/* CREATE ( -- ) Create empty dictionary entry in RAM */
    head CREATE, "CREATE", docolon
        .long ALIGN_WORD
        .long LATEST_WORD, FETCH, COMMA_WORD   /* LFA = previous LATEST */
        .long LIT, 0, CCOMMA                   /* flags = 0 */
        .long HERE_WORD, LATEST_WORD, STORE    /* LATEST = new NFA */
        .long BL_WORD, WORD, CFETCH, CHARPLUS, ALLOT_WORD
        .long ALIGN_WORD
        .long LIT, docreate, COMMACF, EXIT

/* : ( -- ) Begin a colon definition */
    head COLON, ":", docolon
        .long CREATE, HIDE, RIGHTBRACKET, STORCOLON, EXIT

/* ; ( -- ) End a colon definition */
    immed SEMICOLON, ";", docolon
        .long REVEAL, CEXIT, LEFTBRACKET, EXIT

/* (DOES>) ( -- ) Runtime action of DOES> */
    head XDOES, "(DOES>)", docolon
        .long RFROM, LATEST_WORD, FETCH, NFATOCFA, STORECF, EXIT

/* DOES> ( -- ) Change action of latest word */
    immed DOES, "DOES>", docolon
        .long LIT, XDOES, COMMAXT
        .long LIT, dodoes, COMMACF, EXIT

/* ['] ( -- ) Find word & compile as literal */
    immed BRACTICK, "[\']", docolon
        .long TICK, LIT, LIT, COMMAXT, COMMA_WORD, EXIT

/* POSTPONE ( -- ) Postpone word compilation */
    immed POSTPONE, "POSTPONE", docolon
        .long BL_WORD, WORD, FIND, DUP, ZEROEQUAL
        .long XSQUOTE
        .byte 1
        .ascii "?"
        .balign 4
        .long QABORT, ZEROLESS, QBRANCH, 1f
        /* non-immediate: compile LIT xt COMMAXT */
        .long LIT, LIT, COMMAXT, COMMA_WORD
        .long LIT, COMMAXT, COMMAXT, BRANCH, 2f
1:      /* immediate: compile xt directly */
        .long COMMAXT
2:      .long EXIT

/* RECURSE ( -- ) Recurse current definition */
    immed RECURSE, "RECURSE", docolon
        .long LATEST_WORD, FETCH, NFATOCFA, COMMAXT, EXIT

/* VARIABLE ( -- ) Define a Forth variable */
    head VARIABLE, "VARIABLE", docolon
        .long CREATE, LIT, 1, CELLS_WORD, ALLOT_WORD, EXIT

/* CONSTANT ( n -- ) Define a Forth constant */
    head CONSTANT, "CONSTANT", docolon
        .long CREATE, COMMA_WORD
        .long LIT, docon, LATEST_WORD, FETCH, NFATOCFA, STORECF, EXIT

/* USER ( n -- ) Define a Forth user variable */
    head USER, "USER", docolon
        .long CREATE, COMMA_WORD
        .long LIT, douser, LATEST_WORD, FETCH, NFATOCFA, STORECF, EXIT

/* CONTROL STRUCTURES =============================================== */

/* IF ( -- adrs ) */
    immed IFF, "IF", docolon
        .long LIT, QBRANCH, COMMABRANCH
        .long HERE_WORD, DUP, COMMADEST, EXIT

/* THEN ( adrs -- ) */
    immed THEN, "THEN", docolon
        .long HERE_WORD, SWOP, STOREDEST, EXIT

/* ELSE ( adrs1 -- adrs2 ) */
    immed ELSE, "ELSE", docolon
        .long LIT, BRANCH, COMMABRANCH
        .long HERE_WORD, DUP, COMMADEST
        .long SWOP, THEN, EXIT

/* BEGIN ( -- adrs ) */
    immed BEGIN, "BEGIN", docolon
        .long HERE_WORD, EXIT

/* UNTIL ( adrs -- ) */
    immed UNTIL, "UNTIL", docolon
        .long LIT, QBRANCH, COMMABRANCH, COMMADEST, EXIT

/* AGAIN ( adrs -- ) */
    immed AGAIN, "AGAIN", docolon
        .long LIT, BRANCH, COMMABRANCH, COMMADEST, EXIT

/* WHILE ( -- adrs ) */
    immed WHILE, "WHILE", docolon
        .long IFF, EXIT

/* REPEAT ( adrs1 adrs2 -- ) */
    immed REPEAT, "REPEAT", docolon
        .long SWOP, AGAIN, THEN, EXIT

/* >L ( x -- ) Push to leave stack */
    head TOL, ">L", docolon
        .long LP_WORD, FETCH, LIT, -4, PLUS, DUP, LP_WORD, STORE, STORE, EXIT

/* L> ( -- x ) Pop from leave stack */
    head LFROM, "L>", docolon
        .long LP_WORD, FETCH, DUP, FETCH, SWOP, LIT, 4, PLUS, LP_WORD, STORE, EXIT

/* DO ( limit init -- ) ( R: -- limit init ) */
    immed DO_WORD, "DO", docolon
        .long LIT, XDO, COMMAXT, HERE_WORD
        .long LIT, 0, TOL, EXIT

/* ENDLOOP ( adrs xt -- ) Common factor for LOOP and +LOOP */
    head ENDLOOP, "ENDLOOP", docolon
        .long COMMABRANCH, COMMADEST
1:      .long LFROM, QDUP, QBRANCH, 2f
        .long THEN, BRANCH, 1b
2:      .long EXIT

/* LOOP ( adrs -- ) */
    immed LOOP_WORD, "LOOP", docolon
        .long LIT, XLOOP, ENDLOOP, EXIT

/* +LOOP ( adrs -- ) */
    immed PLUSLOOP, "+LOOP", docolon
        .long LIT, XPLUSLOOP, ENDLOOP, EXIT

/* LEAVE ( -- ) */
    immed LEAVE_WORD, "LEAVE", docolon
        .long LIT, UNLOOP_WORD, COMMAXT
        .long LIT, BRANCH, COMMABRANCH
        .long HERE_WORD, DUP, COMMADEST, TOL, EXIT

/* CASE ( -- ) */
    immed CASE_WORD, "CASE", docolon
        .long LIT, TRUE_WORD, COMMAXT, IFF, EXIT

/* OF ( -- ) */
    immed OF_WORD, "OF", docolon
        .long LIT, OVERR, COMMAXT, LIT, EQUAL, COMMAXT
        .long IFF, LIT, DROP, COMMAXT, EXIT

/* ENDOF ( -- ) */
    immed ENDOF_WORD, "ENDOF", docolon
        .long SWOP, THEN, ELSE, EXIT

/* ENDCASE ( -- ) */
    immed ENDCASE_WORD, "ENDCASE", docolon
        .long LIT, DROP, COMMAXT, THEN, EXIT

/* OUTER INTERPRETER & REPL ========================================= */

/* >NUMBER ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 ) Convert string to number */
    head TONUMBER, ">NUMBER", docolon
1:      .long DUP, QBRANCH, 3f
        .long OVERR, CFETCH, DIGITQ
        .long ZEROEQUAL, QBRANCH, 2f, DROP, EXIT
2:      .long TOR, TWOSWAP, BASE_WORD, FETCH, UDSTAR
        .long RFROM, MPLUS, TWOSWAP
        .long LIT, 1, SLASHSTRING, BRANCH, 1b
3:      .long EXIT

/* WITHIN ( n1 min max -- flag ) min <= n1 < max */
    head WITHIN, "WITHIN", docolon
        .long OVERR, MINUS, TOR, MINUS, RFROM, ULESS, EXIT

/* ?NUMBER ( c-addr -- n -1 | c-addr 0 ) Convert counted string to number */
    head QNUMBER, "?NUMBER", docolon
        .long DUP, LIT, 0, DUP, ROT, COUNT
        .long QSIGN, TOR, TONUMBER
        .long QBRANCH, 1f
        /* Unconverted characters: failure */
        .long RFROM, DROP, TWODROP, DROP, LIT, 0, EXIT
1:      /* All characters converted: success */
        .long TWODROP, NIP, RFROM, QBRANCH, 2f, NEGATE
2:      .long LIT, -1, EXIT

/* INTERPRET ( c-addr u -- ) Outer text interpreter */
    head INTERPRET, "INTERPRET", docolon
        .long TICKSOURCE, TWOSTORE
        .long LIT, 0, TOIN, STORE
1:      .long BL_WORD, WORD, DUP, CFETCH, QBRANCH, 9f
        .long FIND, QDUP, QBRANCH, 3f
        /* Word found in dictionary: stack is xt 1 or xt -1 */
        .long LIT, 1, EQUAL, STATE_WORD, FETCH, ZEROEQUAL, ORR_WORD
        .long QBRANCH, 4f
        /* Execute immediately */
        .long EXECUTE, BRANCH, 1b
4:      /* Compile xt */
        .long COMMAXT, BRANCH, 1b
3:      /* Not in dictionary, attempt number conversion */
        .long QNUMBER, QBRANCH, 5f
        /* Number successfully parsed: stack is n */
        .long LITERAL, BRANCH, 1b
5:      /* Undefined word: c-addr is on stack */
        .long COUNT, TYPE, LIT, '?', EMIT, CR_WORD, ABORT_WORD, EXIT
9:      /* End of line reached: drop c-addr and exit */
        .long DROP, EXIT

/* EVALUATE ( c-addr u -- ) Evaluate string in memory */
    head EVALUATE, "EVALUATE", docolon
        .long TICKSOURCE, TWOFETCH, TOR, TOR
        .long TOIN, FETCH, TOR
        .long INTERPRET
        .long RFROM, TOIN, STORE
        .long RFROM, RFROM, TICKSOURCE, TWOSTORE
        .long EXIT

/* ACCEPT ( c-addr +n1 -- +n2 ) Line input from terminal */
    head ACCEPT_WORD, "ACCEPT", docode
        /* c-addr = [psp], +n1 = tos */
        ldr r2, [psp]          /* buffer */
        movs r3, #0            /* char count */
1:      push {r0-r3, lr}
        bl forth_rx_char
        mov r8, r0
        pop {r0-r3, lr}

        /* Check for CR (13) */
        cmp r8, #13
        beq 2f

        /* Check for LF (10): ignore if buffer is empty (e.g. after CR) */
        cmp r8, #10
        beq 4f

        /* Check for Backspace (8) or DEL (127) */
        cmp r8, #8
        beq 3f
        cmp r8, #127
        beq 3f

        /* Normal printable character */
        cmp r3, tos
        bge 1b                 /* buffer full, ignore */
        strb r8, [r2, r3]
        adds r3, r3, #1

        /* Echo character */
        push {r0-r3, lr}
        mov r0, r8
        bl forth_tx_char
        pop {r0-r3, lr}
        b 1b

4:      /* LF received: ignore if buffer is empty */
        cmp r3, #0
        beq 1b
        b 2f

3:      /* Backspace */
        cmp r3, #0
        ble 1b
        subs r3, r3, #1
        /* Echo backspace-space-backspace */
        push {r0-r3, lr}
        mov r0, #8
        bl forth_tx_char
        mov r0, #' '
        bl forth_tx_char
        mov r0, #8
        bl forth_tx_char
        pop {r0-r3, lr}
        b 1b

2:      /* CR/LF completed */
        push {r0-r3, lr}
        mov r0, #13
        bl forth_tx_char
        mov r0, #10
        bl forth_tx_char
        pop {r0-r3, lr}

        mov tos, r3            /* count in TOS */
        adds psp, psp, #4      /* pop buffer addr */
        next

/* QUIT ( -- ) Reset and enter outer interpreter REPL */
    head QUIT, "QUIT", docolon
        .long R0_WORD, RSP_STORE
        .long LIT, 0, STATE_WORD, STORE
1:      .long TIB_WORD, DUP, TIBSIZE_WORD, ACCEPT_WORD
        .long INTERPRET
        .long STATE_WORD, FETCH, ZEROEQUAL, QBRANCH, 2f
        .long XSQUOTE
        .byte 3
        .ascii " ok"
        .balign 4
        .long TYPE, CR_WORD
2:      .long BRANCH, 1b

/* RSP! ( a-addr -- ) Set return stack pointer */
    head RSP_STORE, "RP!", docode
        mov rsp, tos
        ldr tos, [psp], #4
        next

/* ABORT ( -- ) Reset data stack and QUIT */
    head ABORT_WORD, "ABORT", docolon
        .long S0_WORD, PSP_STORE, QUIT, EXIT

/* PSP! ( a-addr -- ) Set parameter stack pointer */
    head PSP_STORE, "SP!", docode
        mov psp, tos
        movs tos, #0
        next

/* ?ABORT ( flag c-addr u -- ) */
    head QABORT, "?ABORT", docolon
        .long ROT, QBRANCH, 1f, TYPE, CR_WORD, ABORT_WORD
1:      .long TWODROP, EXIT

/* WORDS ( -- ) List dictionary words across RAM and ROM */
    head WORDS, "WORDS", docolon
        .long LATEST_WORD, FETCH
1:      .long DUP, QBRANCH, 2f
        .long DUP, COUNT, LIT, 0x7F, ANDD, TYPE, SPACE_WORD
        .long NFATOLFA, FETCH, BRANCH, 1b
2:      .long DROP, CR_WORD, EXIT

/* COLD ( -- ) Cold start */
    head COLD, "COLD", docolon
        .long CR_WORD
        .long XSQUOTE
        .byte (banner_str_end - banner_str_start)
banner_str_start:
        .ascii "CamelForth Cortex-M4 v1.0 ("
        .ascii TARGET_MCU_NAME
        .ascii " DTC 32-bit)"
banner_str_end:
        .balign 4
        .long TYPE, CR_WORD
        .long XSQUOTE
        .byte (help_str_end - help_str_start)
help_str_start:
        .ascii "Type WORDS to list words"
help_str_end:
        .balign 4
        .long TYPE, CR_WORD
        .long ABORT_WORD, EXIT

/* EXTENSIONS ======================================================= */

/* AT-XY ( col row -- ) Position cursor */
    head AT_XY, "AT-XY", docolon
        .long LIT, 27, EMIT, LIT, '[', EMIT
        .long DOT, LIT, ';', EMIT, DOT, LIT, 'H', EMIT, EXIT

/* SAVEU ( -- ) Preserve current DP and LATEST into initial UINIT table */
    head SAVEU, "SAVEU", docolon
        .long HERE_WORD, UINIT_WORD, LIT, 4 * 4, PLUS, STORE
        .long LATEST_WORD, FETCH, UINIT_WORD, LIT, 7 * 4, PLUS, STORE
        .long EXIT

/* ROM DICTIONARY TAIL & USER INITIALIZATION ======================== */

    .balign 4
    .global ROM_LATEST
ROM_LATEST:
    .long link                 /* points to NFA of last word (SAVEU) */

    .global ROM_ENDDICT
ROM_ENDDICT:
    .long .
