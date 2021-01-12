;;-----------------_DEFINITIONS ONLY_-----------------------
;; IMPORT FUNCTIONS FROM C
%macro IMPORTFROMC 1-*
%rep  %0
    %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
    extern _%1
    %1 equ _%1
    %else
    extern %1
    %endif
%rotate 1 
%endrep
%endmacro

;; EXPORT TO C FUNCTIONS
%macro EXPORT2C 1-*
%rep  %0
    %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
    global _%1
    _%1 equ %1
    %else
    global %1
    %endif
%rotate 1 
%endrep
%endmacro

%define                         break xchg bx, bx
%define                         BIT(x)  (1 << (x))


IMPORTFROMC KernelMain


PAGE_SIZE                       equ     1 << 12
PAGE_MASK                       equ     ~(PAGE_SIZE - 1)
SIZE_OF_DWORD                   equ     4

LEAST_SIGNIFICANT_BYTE_MASK     equ     0xFF
MOST_SIGNIFICANT_9_BITS_MASK    equ     0xFF80_0000
LEAST_SIGNIFICANT_9_BITS_MASK   equ     0x1FF

PAGING_TABLE_ENTRY_BYTES        equ     3   ;; Each entry in the page table is 8 (2^3) bytes long

FREE_MEMORY_LIMIT               equ     0x8_0000

BIT_PRESENT                     equ     BIT(0)
BIT_READ_WRITE                  equ     BIT(1)
BIT_USER_SUPERVISOR             equ     BIT(2)

CR0.PG                          equ     BIT(31)
CR0.PE                          equ     BIT(0)
CR4.PAE                         equ     BIT(5)
OSFXSR                          equ     BIT(9)

IA32_EFER_MSR                   equ 0xC0000080
IA32_EFER_MSR.LME               equ     BIT(8)


KILO                            equ     1024
MEGA                            equ     (1024 * (KILO))
GIGA                            equ     (1024 * (MEGA))

TOP_OF_STACK                    equ 0x200000
KERNEL_BASE_PHYSICAL            equ 0x200000
;;-----------------^DEFINITIONS ONLY^-----------------------


segment .text
[BITS 32]
ASMEntryPoint:
    cli

    MOV     DWORD [0x000B8000], 'O1S1'
%ifidn __OUTPUT_FORMAT__, win32
    MOV     DWORD [0x000B8004], '3121'                  ; 32 bit build marker
%else
    MOV     DWORD [0x000B8004], '6141'                  ; 64 bit build marker
%endif

    MOV     ESP, TOP_OF_STACK                           ; just below the kernel

    lgdt    [Gdt]

    mov     ax, 0x20
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    jmp     0x18:ReloadGdt
    ReloadGdt:

;; void     MapPage(DWORD VaAddressHigh, DWORD VaAddressLow, DWORD PaAddressHigh, DWORD PaAddressLow, DWORD *StartPageAddress, DWORD Count)
    
    ;;; Get a new free page of memory in eax
    call    AllocNewPage
    mov     [CR3_ADDRESS], eax       ;; Save CR3

    push    ((5 * MEGA)/PAGE_SIZE)       ;; Count
    push    eax       ;; StartPageAddress
    push    0       ;; PaAddressLow
    push    0       ;; PaAddressHigh
    push    0       ;; VaAddressLow
    push    0       ;; VaAddressHigh
    call    MapPageRange
    add     esp, 6 * SIZE_OF_DWORD

    mov     eax, [CR3_ADDRESS]
    mov     cr3, eax

    ;   Enable PAE
    mov     eax, cr4
    or      eax, CR4.PAE | OSFXSR
    mov     cr4, eax

    ;   Enable Long Mode Enable bit in IA32_EFER msr
    mov     ecx, IA32_EFER_MSR
    rdmsr   
    or      eax, IA32_EFER_MSR.LME
    wrmsr

    ;   Enable Paging
    mov     eax, cr0
    or      eax, CR0.PG
    mov     cr0, eax


    ;   Change data selectors for 64 bit data ring 0
    mov     ax, 0x30
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ;   Change code selector for 64 bit code ring 0
    jmp     0x28:_Init64

    _Init64:
    [bits 64]
    mov     eax, 0xDADADA

    mov     rsp, TOP_OF_STACK
    mov     rcx, BootStructure
    call    KernelMain

    break
    cli
    hlt

    Gdt:
        .Limit:     dw  TableEnd - TableStart
        .Base:      dd  TableStart
    TableStart:
        .Zero:      dq  0x0
        .Code16:    dq  0x8F9A000000FFFF
        .Data16:    dq  0x8F92000000FFFF
        .Code32:    dq  0xCF9A000000FFFF
        .Data32:    dq  0xCF92000000FFFF
        .Code64:    dq  0xAF9A000000FFFF
        .Data64:    dq  0xAF92000000FFFF
    TableEnd:

    BootStructure:
    LAST_ALLOCATED_PAGE     dd      0x5_0000
    CR3_ADDRESS             dd      0x0


    
extern GeneralExceptionHandler

%assign i 0
%rep 256
    global _Handle %+ i
    _Handle %+ i %+ :
    push    rax
    push    qword i
    jmp     ExecuteInterrupt
%assign i i+1
%endrep

ExecuteInterrupt:
    push    rbp
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rax, ds
    push    rax
    mov     rax, es
    push    rax
    mov     rax, fs
    push    rax
    mov     rax, gs
    push    rax
    mov     rax, cr0
    push    rax
    mov     rax, cr2
    push    rax
    mov     rax, cr3
    push    rax
    mov     rcx, rsp
    call    GeneralExceptionHandler
    pop     rax
    mov     cr3, rax
    pop     rax
    ;mov    cr2, rax
    pop     rax
    mov     cr0, rax
    pop     rax ;;gs
    pop     rax ;;fs
    pop     rax ;;es
    pop     rax ;;ds
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rbp
    pop     rax ;;-> pop VectorNumber
    pop     rax
    iretq
    pidaux  dq 1

EXPORT2C ASMEntryPoint, __cli, __sti, __magic, __enableSSE, __assignEax, __assignRipToEax, __assignRax

    ;TODO!!! define page tables; see https://wiki.osdev.org ,Intel's manual, http://www.brokenthorn.com/Resources/ ,http://www.jamesmolloy.co.uk/tutorial_html/

    ;TODO!!! activate pagging
    
    ;TODO!!! transition to 64bits-long mode

[BITS 32]
;; Paging methods

;; 
;; void     Memset(BYTE *Address, DWORD Size, BYTE Value)
Memset:

    push    ebp
    mov     ebp, esp
    push    ecx
    push    edx
    push    edi

    ;; Stack from ebp: ebp, ret_addr, param1 (Address), param2 (Size), param3 (Value)
    mov     eax, [ebp + 4 * SIZE_OF_DWORD]      ;; eax is Value
    mov     ecx, [ebp + 3 * SIZE_OF_DWORD]      ;; ecx is Size
    mov     edi, [ebp + 2 * SIZE_OF_DWORD]      ;; edi is Aaddress

    and     eax, LEAST_SIGNIFICANT_BYTE_MASK    ;; Isolate last byte of eax
    rep     stosb

    pop     edi
    pop     edx
    pop     ecx
    pop     ebp
    ret

;; PVOID    AllocNewPage()
AllocNewPage:
    push    ebx
    ;; There are no parameters to retrieve, so there's no need to save the esp

    mov     ebx,    [LAST_ALLOCATED_PAGE]
    cmp     ebx,    FREE_MEMORY_LIMIT
    jae     .NotenoughMemory

    ;; Mark that we have allocated a page
    mov     eax,    ebx
    add     eax,    PAGE_SIZE
    mov     [LAST_ALLOCATED_PAGE], eax
    
    ;; Clear the newly allocated page
    push    dword 0     ;; Value
    push    PAGE_SIZE   ;; Size
    push    ebx         ;; Address
    call    Memset
    ;; Clear stack parameters
    add     esp, 3 * SIZE_OF_DWORD
    mov     eax, ebx    ;; return the allocated page

    pop     ebx
    ret

    .NotenoughMemory:
        mov     edx, 0xDEAD
        break
        cli
        hlt

;; void     MapPage(DWORD VaAddressHigh, DWORD VaAddressLow, DWORD PaAddressHigh, DWORD PaAddressLow, DWORD *StartPageAddress)
MapPage:
    push    ebp
    mov     ebp, esp

    ;; Create the stack frame
    push    ecx
    push    edx
    push    esi
    push    edi

    ;; Stack from ebp: ebp, ret_addr, param1 (VaAddressHigh), param2 (VaAddressLow), param3 (PaAddressHigh), , param4 (PaAddressLow), , param5 (StartPageAddress)

    ;; Save the StartPageAddress in edi
    mov     edi,    [ebp + 6 * SIZE_OF_DWORD]

    ;; Move VaAddressHigh:VaAddressLow into esi:edx
    mov     esi,    [ebp + 2 * SIZE_OF_DWORD]
    mov     edx,    [ebp + 3 * SIZE_OF_DWORD]
  ;;shrd    dest, source, count
    shrd    edx,  esi,    7     ;; Last 12 bits of the low address, are of no interest for us right now
                                ;; instead drag the least significant 6 bits of the high address over
                                ;; in order to obtain the Directory Ptr, Dicrectory and Table indices in edx
                                ;; and keep the PML4 index in esi
                                ;; (https://www.felixcloutier.com/x86/shrd)

    shr     esi, 6      ;; As stated in (https://www.felixcloutier.com/x86/shrd) the shrd instruction
                        ;; leaves source (esi in our case), untouched, so we'll have to shift it ourselves

    ;; Isolate the current index
    and     esi, LEAST_SIGNIFICANT_9_BITS_MASK

    push    edx         ;; Save the other indices for later use
    mov     ecx,    4   ;; 4 Level Paging Structure

    .NextPagingStructure:
        ;; In esi we have the current index of the next paging table
        ;; In edi we have the pointer to the current page

        shl     esi, PAGING_TABLE_ENTRY_BYTES   ;; Multiply the index with 64 (shift left with 8) to obtain the distance between the paging table start and the entry pointed by the index

        ;; Now esi + edi point to the entry in the paging table we want to modify.
        ;; remember, the memory is in little-endian => the least significant dword is stored first in memory

        mov     eax, [edi + esi] ;; read the least significant dword in memory, check if the present bit is set
        cmp     eax, 0
        jz      .NextStructureNotAllocated
        .StructureAlreadyAllocated:                ;; In this case we want to obtain the address of the next table

        mov     eax, [edi + esi]                   ;; As we are in 32 bit mode, the page frame << 12 cannot exceed 2^32 bits
                                                   ;; otherwise we cannot access it. => the low part of the entry is enough
                                                   ;; for us

        and     eax, PAGE_MASK                     ;; Clear any unrequired bits
        jmp     .ObtainedNextTableAddress
        
        .NextStructureNotAllocated:
        cmp     ecx, 1
        je      .AssignPhysicalAddress
        .AssignNewPage:
            ;; Allocate a new page and set the Present, Read/Write and User/Supervisor bits
            call    AllocNewPage
            or      eax, (BIT_PRESENT | BIT_READ_WRITE | BIT_USER_SUPERVISOR)
            ;; Save the entry
            mov     [edi + esi], eax
            jmp     .ObtainedNextTableAddress;

        .AssignPhysicalAddress:

            ;; From the stack, get the physical address we want to map
            mov     eax, [ebp + 4 * SIZE_OF_DWORD]      ;; eax = PaAddressHigh
            mov     [edi + esi + SIZE_OF_DWORD], eax    ;; store the high part of the physical address, nothing to do here

            mov     eax, [ebp + 5 * SIZE_OF_DWORD]      ;; eax = PaAddressLow
            and     eax, PAGE_MASK                      ;; ensure the physical address is page alligned
            or      eax, (BIT_PRESENT | BIT_READ_WRITE | BIT_USER_SUPERVISOR)    ;; Set the Present, Read/Write and User/Supervisor bits
            mov     [edi + esi], eax                    ;; save the low part in the low part of the page table entry !!REMEMBER, WE'RE IN LITTLE ENDIAN!!

        .ObtainedNextTableAddress:
        ;; Clear the least significant bits to further use eax as an address
        and     eax, PAGE_MASK
        mov     edi, eax    ;; Save the address of the next paging table in edi for the next itteration

        ;;  get the nex table index
        pop     edx
        mov     esi, edx
        shr     esi, 23     ;; Isolate the next index
        shl     edx, 9      ;; Remove the current index from the shadow. I.E. Advance the indeces
        push    edx         ;; Save the remaining indeces for further itterations

        dec     ecx                     ;; Itterate to the next level of the paging structure
        jnz     .NextPagingStructure

    pop     eax         ;; Pop residual value from the stack
    
    ;;  Restore registers
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebp
    ret

;; void     MapPage(DWORD VaAddressHigh, DWORD VaAddressLow, DWORD PaAddressHigh, DWORD PaAddressLow, DWORD *StartPageAddress, DWORD Count)
;;  FOR THE SAKE OF SIMPLICITY WE ASSUME WE DO NOT MAP MEMORY > 4GB RIGHT NOW
MapPageRange:
    push    ebp
    mov     ebp, esp
    push    ecx
    push    edx
    push    esi
    push    edi

    ;; Stack from ebp: ebp, ret_addr, param1 (VaAddressHigh), param2 (VaAddressLow), param3 (PaAddressHigh), , param4 (PaAddressLow), , param5 (StartPageAddress), param6 (Count)
    mov     ecx, [ebp + 7 * SIZE_OF_DWORD]  ;; ecx == Count
    mov     edi, [ebp + 3 * SIZE_OF_DWORD]  ;; edi == VaAddressLow
    mov     esi, [ebp + 5 * SIZE_OF_DWORD]  ;; esi == PaAddressLow
    mov     edx, [ebp + 6 * SIZE_OF_DWORD]  ;; edx == StartPageAddress

    .MapNextPage:
        push    edx                         ;; StartPageAddress
        push    esi                         ;; PaAddressLow
        push    0                           ;; PaAddressHigh
        push    edi                         ;; VaAddressLow
        push    0                           ;; VaAddressHigh
        call    MapPage                     ;; Map page
        
        add     esp, 5 * SIZE_OF_DWORD      ;; Clear stack

        
        add     edi, PAGE_SIZE              ;; Map the next VaAddress
        add     esi, PAGE_SIZE              ;; to the next PaAddress

    dec ecx
    jnz .MapNextPage

    ;; Restore registers
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebp
    ret

;;--------------------------------------------------------
[BITS 32]

__cli:
    CLI
    RET

__sti:
    STI
    RET

__magic:
    break
    RET

__assignRipToEax:
    mov     eax, [esp]
    ret

__assignEax:
    mov     EAX, [ESP + 4]
    RET


__osExited:
    break
    cli
    hlt

__enableSSE:                ;; enable SSE instructions (CR4.OSFXSR = 1)  
    MOV     EAX, CR4
    OR      EAX, 0x00000200
    MOV     CR4, EAX
    RET
    
[BITS 64]
    __assignRax:
    mov     rax, rcx
    RET


EXPORT2C ASMEntryPoint, __cli, __sti, __magic, __enableSSE, __assignEax, __assignRipToEax, __assignRax


