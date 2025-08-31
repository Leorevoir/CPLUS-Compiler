# TODO:

- fix IR (too many temp)
- inlining when applicable?
- codegen x86-64 Intel-Syntax assembly
- build a script-linker to inline the right `_start`
- goal x86-64 codegen syntax is [**here**](./examples/fibonacci.s)

## Linker:

we may generate multiple `_start`
the most common will ofc be:

```asm
.globl _start
_start:
    call main
    mov rdi, rax
    mov rax, 60
    syscall
```

but we also need other `_start` for the cases:

- user needs argc, argv
- user needs argc, argv, env
- user doesnt need `_start` label because they already provided one.
