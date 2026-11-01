\ Benchmarks for STM32G474 CamelForth Cortex-M4

CR .( --- Benchmark: Sieve of Eratosthenes --- ) CR
8190 CONSTANT SIZE
VARIABLE FLAGS SIZE ALLOT

: DO-PRIME
    FLAGS SIZE 1 FILL
    0 ( count )
    SIZE 0 DO
        FLAGS I + C@ IF
            I 2* 3 + ( prime )
            DUP I +
            BEGIN
                DUP SIZE <
            WHILE
                0 OVER FLAGS + C!
                OVER +
            REPEAT
            DROP DROP
            1+
        THEN
    LOOP
    . ."  primes found" CR
;

CR .( Run DO-PRIME to benchmark prime calculation: ) CR
