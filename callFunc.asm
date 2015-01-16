
    extern printf

    SECTION .bss

mainstack:      resq 1 ; Stored mainstack rsp
currentthread:  resq 1 ; Pointer to current thread

rax_0: resq 1
rcx_0: resq 1
rdx_0: resq 1
rsi_0: resq 1
rdi_0: resq 1
rsp_0: resq 1
r8_0: resq 1
r9_0: resq 1
r10_0: resq 1
r11_0: resq 1

    SECTION .data
__S0: db `Hit here`, 10, 0

    SECTION .text

    ; extern void yield();
    global yield
yield:
    push    rbp
    mov     rbp, rsp

    ; Get curThread pointer
    mov     rdx, qword [currentthread]
    ; Get return address
    mov     rax, [rbp+8]
    ; Set validity of thread
    mov     byte [rdx+56], 1 ; ThreadData->stillValid
    ; Set return address to continue execution
    mov     [rdx+8], rax  ; ThreadData->curFuncAddr
    ; Determine current value of rsp from perspective of curThread
    mov     rcx, rsp
    ; Need to remove rbp, return address from consideration
    add     rcx, 16
    ; Set curThread StackCur value
    mov     [rdx+32], rcx   ; ThreadData->t_StackCur

    ; Restore rbp
    pop     rbp
    ; Pop return address off the stack
    add     rsp, 8

    ; Save rbp for thread
    mov     [rdx+48], rbp ; ThreadData->t_rbp

    jmp     schedulerReturn


    ; extern void callFunc(size_t argBytes, void* funcAddr, uint8_t* stackPtr, ThreadData* curThread);
    global callFunc
callFunc:
    push    rbp                     ; set up stack frame
    mov     rbp,rsp
    sub     rsp, 128



    mov     qword [rbp-8], rbx      ; Register preservation
    mov     qword [rbp-16], r12
    mov     qword [rbp-24], r13
    mov     qword [rbp-32], r14
    mov     qword [rbp-40], r15
    ; push    rbx
    ; push    r12
    ; push    r13
    ; push    r14
    ; push    r15


    ; mov     qword [rax_0], rax
    ; mov     qword [rcx_0], rcx
    ; mov     qword [rdx_0], rdx
    ; mov     qword [rsi_0], rsi
    ; mov     qword [rdi_0], rdi
    ; mov     qword [rsp_0], rsp
    ; mov     qword [r8_0], r8
    ; mov     qword [r9_0], r9
    ; mov     qword [r10_0], r10
    ; mov     qword [r11_0], r11
    ; mov     rdi, __S0
    ; call    printf
    ; mov     rax, qword [rax_0]
    ; mov     rcx, qword [rcx_0]
    ; mov     rdx, qword [rdx_0]
    ; mov     rsi, qword [rsi_0]
    ; mov     rdi, qword [rdi_0]
    ; mov     rsp, qword [rsp_0]
    ; mov     r8, qword [r8_0]
    ; mov     r9, qword [r9_0]
    ; mov     r10, qword [r10_0]
    ; mov     r11, qword [r11_0]


    ; Populate registers for operation. ThreadData* thread is initially in rdi
    mov     rcx, rdi            ; ThreadData* thread
    mov     rdi, qword [rcx+80] ; ThreadData->stackArgsSize
    mov     r12, qword [rcx]    ; ThreadData->funcAddr
    mov     rdx, qword [rcx+24] ; ThreadData->t_StackBot
    mov     rax, qword [rcx+88] ; ThreadData->regVars
    ; First set the currentthread value
    mov     qword [currentthread], rcx ; ThreadData* thread

    ; Determine if we are starting a new thread, or if we're continuing
    ; execution
    mov     r8, qword [rcx+8] ; ThreadData->curFuncAddr (0 if start of thread)
    test    r8, r8
    ; If not zero, then continue where we left off
    jne     continueThread

    ; If we get here, we're starting the execution of a new thread

    mov     qword [rcx+8], r12  ; ThreadData->curFuncAddr, init to start of func

    ; Set stack pointer to be before arguments
    sub     rdx, rdi
    ; Allocate 8 bytes on stack for return address
    sub     rdx, 8
    mov     qword [rdx], schedulerReturn

    ; Store the value of the main stack pointer, and store rbp as top value
    push    rbp
    mov     qword [mainstack], rsp
    mov     rsp, rdx

    ; Move the register function arguments into the relevant registers
    mov     rdi, qword [rax]
    mov     rsi, qword [rax+8]
    mov     rdx, qword [rax+16]
    mov     rcx, qword [rax+24]
    mov     r8, qword [rax+32]
    mov     r9, qword [rax+40]
    movsd   xmm0, qword [rax+48]
    movsd   xmm1, qword [rax+56]
    movsd   xmm2, qword [rax+64]
    movsd   xmm3, qword [rax+72]
    movsd   xmm4, qword [rax+80]
    movsd   xmm5, qword [rax+88]
    movsd   xmm6, qword [rax+96]
    movsd   xmm7, qword [rax+104]

    jmp     r12                     ; Call function


continueThread:
    ; ThreadData* thread is in rcx
    ; In order to restart execution, we need to set rsp to correct value,
    ; and then "return" to the previous stage of execution. As part of setting
    ; up for a clean return, push rbp as the last thing on the mainstack
    push    rbp
    ; Save mainstack rsp
    mov     qword [mainstack], rsp
    ; Set rsp to StackCur of current thread
    mov     rsp, qword [rcx+32] ; ThreadData->t_StackCur
    ; Set rbp to t_rbp of current thread
    mov     rbp, qword [rcx+48] ; ThreadData->t_rbp
    ; Set stillValid to 0, to account for possibly naturally returning from
    ; the function at the end of its execution. A thread is still valid if
    ; stillValid != 0 OR curFuncAddr == 0 (meaning thread hasn't started yet)
    mov     byte [rcx+56], 0    ; ThreadData->stillValid
    ; Get "return" address to return to thread execution point
    mov     rcx, qword [rcx+8] ; ThreadData->curFuncAddr
    jmp     rcx


schedulerReturn:
    ; Restore the value of the main stack pointer
    mov     rsp, qword [mainstack]
    ; Restore current rbp
    pop     rbp

    mov     rbx, qword [rbp-8]                     ; Restore registers
    mov     r12, qword [rbp-16]
    mov     r13, qword [rbp-24]
    mov     r14, qword [rbp-32]
    mov     r15, qword [rbp-40]

    mov     rsp, rbp                ; takedown stack frame
    pop     rbp
    ret
