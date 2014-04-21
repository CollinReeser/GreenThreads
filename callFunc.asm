    extern printf      ; the C function, to be called
    extern average
    extern example_func
    extern initThreadData
    extern hitASM

    SECTION .data       ; Data section, initialized variables

fmt:    db "Entered asm func", 10, 0 ;

    SECTION .text

    global testCallFunc
testCallFunc:
    push    ebp     ; set up stack frame
    mov     ebp,esp

    push    fmt
    call    printf
    add     esp, 4

    sub     esp, 1
    mov     byte [esp], 21
    sub     esp, 1
    mov     byte [esp], 32
    sub     esp, 1
    mov     byte [esp], 43
    sub     esp, 1
    mov     byte [esp], 54
    sub     esp, 1
    mov     byte [esp], 65
    mov     eax, 5
    push    eax
    call    average
    add     esp, 9

    call    hitASM

    sub     esp, 1
    mov     byte [esp], 10
    push    example_func
    mov     eax, 1
    push    eax
    call    initThreadData
    add     esp, 9

    call    hitASM

    mov     esp, ebp    ; takedown stack frame
    pop     ebp     ; same as "leave" op

    ret         ; return


    ; extern void newProc(size_t argBytes, void* funcAddr, uint8_t* args);
    global newProc
newProc:
    push    ebp                     ; set up stack frame
    mov     ebp,esp

    push    ebx                     ; Register preservation
    push    esi

    ; Get function arguments. Args START at [ebp + 8]
    mov     eax, dword [ebp + 8]    ; argBytes (counter)
    mov     esi, dword [ebp + 16]   ; args (pointer)
    ; The args are passed in in the order they would appear in the signature
    ; for the function pointed to by funcAddr, so we set up esi to push the
    ; args to the stack in reverse order
    add     esi, eax
    sub     esi, 1
    mov     ecx, dword [ebp + 12]   ; funcAddr (address)
    mov     edx, dword [ebp + 8]    ; argBytes (literal)

    .loop:
    mov     bl, byte [esi]          ; Get byte at current args pointer index
    sub     esp, 1                  ; Allocate space on stack
    mov     byte [esp], bl          ; Move byte onto stack

    sub     esi, 1                  ; Decrement pointer to previous byte
    sub     eax, 1                  ; Decrement counter
    jne     .loop                   ; Loop if not done
    .endloop:

    push    ecx                     ; Push function address
    push    edx                     ; Push argBytes

    call    initThreadData          ; Initialize the thread struct

    ; Add 8 bytes from funcAddr and argBytes values back to the stack
    add     esp, 8
    ; Add argBytes bytes back to the stack. We can't use edx because it's not
    ; guaranteed to be preserved by the calling conventions
    add     esp, [ebp + 8]

    pop     esi                     ; Restore registers
    pop     ebx

    mov     esp, ebp                ; takedown stack frame
    pop     ebp
    ret
