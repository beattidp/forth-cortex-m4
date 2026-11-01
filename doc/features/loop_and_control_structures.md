# Loop and Control Flow Architecture

## Overview
This document specifies the implementation of control flow structures in the Cortex-M4 CamelForth port, including conditionals (`IF/ELSE/THEN`), indefinite loops (`BEGIN/UNTIL`, `BEGIN/WHILE/REPEAT`, `BEGIN/AGAIN`), count loops (`DO/LOOP`, `DO/+LOOP`), and loop indices (`I`, `J`, `LEAVE`, `UNLOOP`).

---

## 1. Branching Primitives

### `branch` (Unconditional Branch)
- Code follows: `.long BRANCH, dest_addr`
- Fetches destination address directly into `f_ip` (`R4`):
  ```assembly
  head BRANCH, "branch", docode
      ldr f_ip, [f_ip]
      next
  ```

### `?branch` (Conditional Branch on Zero)
- Code follows: `.long QBRANCH, dest_addr`
- If `tos == 0`: branches to `dest_addr`.
- If `tos != 0`: increments `f_ip` by 4 bytes to skip the inline destination address.
  ```assembly
  head QBRANCH, "?branch", docode
      cmp tos, #0
      ite eq
      ldreq f_ip, [f_ip]
      addne f_ip, f_ip, #4
      ldr tos, [psp], #4
      next
  ```

---

## 2. Standard Count Loops: `DO` / `LOOP` / `+LOOP`

### The F83 / ANSI Overflow Detection Trick
To detect loop termination efficiently in hardware without conditional branches and without sign-check anomalies, the loop limit is transformed to `0x80000000`:
$$\text{fudge} = 0x80000000 - \text{limit}$$
$$\text{index}' = \text{init} + \text{fudge}$$

Both `fudge` and `index'` are stored on the Forth return stack (`rsp`):
`[rsp]` = `index'`
`[rsp + 4]` = `fudge`

### Runtimes
- **`(do)`**:
  ```assembly
  head XDO, "(do)", docode
      ldr t1, [psp], #4      /* t1 = limit */
      mov32 t2, 0x80000000
      subs t2, t2, t1        /* t2 = fudge = 0x80000000 - limit */
      adds tos, tos, t2      /* tos = index = init + fudge */
      str t2, [rsp, #-4]!    /* push fudge */
      str tos, [rsp, #-4]!   /* push index */
      ldr tos, [psp], #4     /* pop new TOS */
      next
  ```
- **`(loop)`**:
  Increment index by 1:
  ```assembly
  head XLOOP, "(loop)", docode
      ldr t1, [rsp]          /* t1 = index */
      adds t1, t1, #1
      bvs 1f                 /* overflow across 0x80000000 terminates loop */
      str t1, [rsp]
      ldr f_ip, [f_ip]       /* take backward inline branch */
      next
  1:  adds rsp, rsp, #8      /* clean up R-stack */
      adds f_ip, f_ip, #4    /* skip inline branch */
      next
  ```
- **`I` (Innermost Loop Index)**:
  $$\text{current index} = \text{index}' - \text{fudge}$$
  ```assembly
  head II, "I", docode
      str tos, [psp, #-4]!
      ldr t1, [rsp]          /* index' */
      ldr t2, [rsp, #4]      /* fudge */
      subs tos, t1, t2       /* current loop value */
      next
  ```
- **`J` (Second Outer Loop Index)**:
  $$\text{current index} = \text{outer index}' - \text{outer fudge}$$
  ```assembly
  head JJ, "J", docode
      str tos, [psp, #-4]!
      ldr t1, [rsp, #8]
      ldr t2, [rsp, #12]
      subs tos, t1, t2
      next
  ```
- **`UNLOOP`**:
  Discards `index'` and `fudge` from return stack (`adds rsp, rsp, #8; next`).

---

## 3. High-Level Compiler Words

- **`DO`**: Compiles `(do)`, records `HERE`, pushes marker `0` to leave stack (`>L`).
- **`LOOP`**: Compiles `(loop)`, compiles backward branch to `DO`, resolves any pending `LEAVE` addresses using `L>`.
- **`+LOOP`**: Compiles `(+loop)`, compiles backward branch to `DO`, resolves `LEAVE` addresses.
- **`LEAVE`**: Compiles `UNLOOP`, compiles `branch`, records branch address on leave stack (`>L`).
