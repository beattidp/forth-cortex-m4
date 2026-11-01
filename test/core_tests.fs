\ Core Forth Verification Script for STM32G474 CamelForth
\ Can be pasted directly into serial terminal @ 115200 baud

CR .( --- Testing Stack Operations --- ) CR
1 2 3 ROT . . . CR \ Expect: 1 3 2
1 2 SWAP . . CR    \ Expect: 1 2
5 DUP * . CR       \ Expect: 25

CR .( --- Testing Arithmetic & Logic --- ) CR
10 20 + . CR       \ Expect: 30
100 35 - . CR      \ Expect: 65
6 7 * . CR         \ Expect: 42
100 8 /MOD . . CR  \ Expect: 12 4 (quotient remainder)
-5 ABS . CR        \ Expect: 5

CR .( --- Testing RAM Word Compilation --- ) CR
: SQUARE DUP * ;
9 SQUARE . CR      \ Expect: 81

: CUBE DUP SQUARE * ;
3 CUBE . CR        \ Expect: 27

CR .( --- Testing Variables and Constants in RAM --- ) CR
VARIABLE MYVAR
12345 MYVAR !
MYVAR @ . CR       \ Expect: 12345

54321 CONSTANT MYCONST
MYCONST . CR       \ Expect: 54321

CR .( --- Testing Control Flow in RAM --- ) CR
: COUNTDOWN ( n -- )
    BEGIN
        DUP . SPACE
        1-
        DUP 0<
    UNTIL
    DROP
;
5 COUNTDOWN CR     \ Expect: 5 4 3 2 1 0

CR .( --- All Basic Verification Tests Complete! --- ) CR
