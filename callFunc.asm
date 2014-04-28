    extern printf      ; the C function, to be called
    extern average
    extern example_func
    extern addThreadData
    extern hitASM

    SECTION .data       ; Data section, initialized variables

fmt:    db "Entered asm func", 10, 0 ;
val:    db "Val: %u", 10, 0

    SECTION .bss

mainstack: resd 1

    SECTION .text

    ; extern void callFunc(size_t argBytes, void* funcAddr, uint8_t* args);
    global callFunc
callFunc:
    push    ebp                     ; set up stack frame
    mov     ebp,esp

    push    ebx                     ; Register preservation
    push    esi

    ; Get function arguments. Args START at [ebp + 8]
    mov     eax, dword [ebp + 8]    ; argBytes (counter)
    mov     ecx, dword [ebp + 12]   ; funcAddr (address)
    mov     esi, dword [ebp + 16]   ; stack bottom (pointer)

    ; Set stack pointer to be before arguments
    sub     esi, eax
    ; Allocate 4 bytes for return address
    sub     esi, 4
    mov     dword [esi], .return_loc

    ; Store the value of the main stack pointer
    mov     [mainstack], esp
    mov     esp, esi

    jmp     ecx                     ; Call function

.return_loc:
    ; Restore the value of the main stack pointer
    mov     esp, [mainstack]

    ; Reset stack to where esi and ebx are. We get the num of bytes, and mul
    ; by 4 since we were pushing dwords above to satisfy alignment
    mov     eax, [ebp + 8]
    imul    eax, 4
    add     esp, eax

    pop     esi                     ; Restore registers
    pop     ebx

    mov     esp, ebp                ; takedown stack frame
    pop     ebp
    ret


    ; extern void newProc(size_t argBytes, void* funcAddr, uint8_t* args);
    global newProc
newProc:
    push    ebp                     ; set up stack frame
    mov     ebp,esp

    push    ebx                     ; Register preservation
    push    esi

    ; Get function arguments. Args START at [ebp + 8]
    mov     eax, dword [ebp + 8]    ; argBytes (counter)
    mov     ecx, dword [ebp + 12]   ; funcAddr (address)
    mov     esi, dword [ebp + 16]   ; args (pointer)
    ; The args are passed in in the order they would appear in the signature
    ; for the function pointed to by funcAddr, so we set up esi to push the
    ; args to the stack in reverse order
    add     esi, eax
    sub     esi, 1
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

    call    addThreadData           ; Initialize the thread struct

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
