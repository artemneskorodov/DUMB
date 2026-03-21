;-------------------------------------------------------------------------------
section .text
;-------------------------------------------------------------------------------
;
; Global API
;

; input (RSI = 'string addr', RDX = 'string len') -> RAX = 'value'
global input

; output(RSI = 'string addr', RDX = 'string len', RCX = 'value')
global output

;===============================================================================
;   Expects:
;       RSI - address of string to print
;       RDX - length of string
;-------------------------------------------------------------------------------
;   Returns:
;       RAX - value
;-------------------------------------------------------------------------------
;   Destroys:
;       TODO fix destroys
;===============================================================================
input:
    ; Calling write of user provided string
    mov rax, 1 ; write syscall
    mov rdi, 1 ; stdout
    syscall

    ; Reading value to buffer
    mov rax, 0 ; read syscall
    mov rdi, 1 ; stdin
    mov rsi, Buffer
    mov rdx, BufferLen
    syscall

    ; string_to_int( RSI = 'string', RDI = 'string length' )
    ; returns value in RAX
    mov rdi, rax
    call string_to_int

    ret
;===============================================================================
;   Expects:
;       RSI - address of string to print
;       RDX - length of string
;       RCX - value to print
;-------------------------------------------------------------------------------
;   Returns:
;       None
;-------------------------------------------------------------------------------
;   Destroys:
;       TODO fix destroys
;===============================================================================
output:
    mov r10, rcx
    ; Calling write of user provided string
    mov rax, 1
    mov rdi, 1
    syscall

    ; Printing value
    ; int_to_string( RAX = 'value to print' )
    ; returns: RSI - address of string, RDX - length of string
    mov rax, r10
    call int_to_string
    mov rax, 1
    mov rdi, 1
    syscall

    ret

;===============================================================================
;   Expects:
;       RSI - string
;       RDI - length of string
;-------------------------------------------------------------------------------
;   Returns:
;       RAX - value represented by string
;-------------------------------------------------------------------------------
;   Destroys:
;       RCX, RDX, RBX, RBX
;===============================================================================
string_to_int:
    ; Result in RAX
    xor rax, rax
    ; Multiplier in RCX
    mov rcx, 10
    ; Counter in RDX
    mov rdx, 0
    ; Zero RBX to use BL for digits
    xor rbx, rbx
    ; Checking for minus sign
    mov rbp, 1
    mov bl, byte [rsi + rdx]
    cmp bl, '-'
    jne .loop_start
    ; Multiplier in rbp
    mov rbp, -1
    inc rdx
    ; Iterating through digit
    .loop_start:
        ; Checking for overflow
        cmp rdx, rdi
        jge .done
        ; Symbol in BL
        mov bl, byte [rsi + rdx]
        ; Checking that number
        cmp bl, '0'
        jl .done
        cmp bl, '9'
        jg .done
        ; ASCII to number
        sub bl, '0'
        ; Adding digit to result
        imul rax, rcx
        add rax, rbx

        inc rdx
        jmp .loop_start

    .done:
    imul rax, rbp
    ret

;===============================================================================
;   Expected:
;       RAX - number
;-------------------------------------------------------------------------------
;   Returns:
;       RSI - address of string
;       RDX - length of string
;-------------------------------------------------------------------------------
;   Destroys:
;       TODO fix destroys
;===============================================================================
int_to_string:
    ; Divider in RCX
    mov rcx, 10
    ; End of buffer in
    mov rsi, Buffer
    add rsi, 64 - 1

    ; Adding new line to ending
    mov byte [rsi], 10
    dec rsi

    ; Adding '0' and finishing if zero
    test rax, rax
    jnz .skip_zero

    mov byte [rsi], '0'
    dec rsi
    jmp .done

    .skip_zero:
    ; Minus sign
    mov bl, 0
    cmp rax, 0
    jg .convert_loop
    mov rbx, -1
    imul rax, rbx
    mov bl, '-'

    ; Adding digits
    .convert_loop:
        ; Checking if zero
        test rax, rax
        jz .done
        ; RDX = RAX % RCX
        xor rdx, rdx
        div rcx
        ; Converting value to ASCII
        add dl, '0'
        mov byte [rsi], dl
        ; Position--
        dec rsi
        jmp .convert_loop

    .done:
    test bl, bl
    jz .skip_add_minus
    mov byte [rsi], bl
    dec rsi
    .skip_add_minus:
    ; Last digit is not used
    inc rsi
    ; Length in RDX
    mov rdx, Buffer + 64
    sub rdx, rsi

    ret


section .data
    Buffer    db   64 dup(0)
    BufferLen equ  $ - Buffer
