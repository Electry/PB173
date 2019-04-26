import subprocess

__BIN = "~/pb173/decode"

__TESTS_SINGLE_INSTR_BASIC = [
    #
    # Branching (JMP, JB, JE, JNE)
    #
    # rel8off
    ("EB 00",               r"jmp $rip+0x0"),
    ("EB 1A",               r"jmp $rip+0x1A"),
    ("72 00",               r"jb $rip+0x0"),
    ("72 1A",               r"jb $rip+0x1A"),
    ("74 00",               r"je $rip+0x0"),
    ("74 1A",               r"je $rip+0x1A"),
    ("75 00",               r"jne $rip+0x0"),
    ("75 1A",               r"jne $rip+0x1A"),
    # rel32off
    ("E9 00 00 00 00",      r"jmp $rip+0x0"),
    ("E9 12 34 56 78",      r"jmp $rip+0x78563412"),
    ("0F 82 00 00 00 00",   r"jb $rip+0x0"),
    ("0F 82 12 34 56 78",   r"jb $rip+0x78563412"),
    ("0F 84 00 00 00 00",   r"je $rip+0x0"),
    ("0F 84 12 34 56 78",   r"je $rip+0x78563412"),
    ("0F 85 00 00 00 00",   r"jne $rip+0x0"),
    ("0F 85 12 34 56 78",   r"jne $rip+0x78563412"),

    #
    # PUSH
    #
    # reg64
    ("50",                  r"push %rax"),
    ("52",                  r"push %rdx"),
    ("57",                  r"push %rdi"),
    # reg64 (extended GPRs)
    ("41 50",               r"push %r8"),
    ("41 57",               r"push %r15"),
    # imm8
    ("6A 00",               r"push $0x0"),
    ("6A 1A",               r"push $0x1a"),
    # imm64 (sign-extended 32b)
    ("68 00 00 00 00",      r"push $0x0"),
    ("68 12 34 56 78",      r"push $0x78563412"),
    # fs/gs
    ("0F A0",               r"push %fs"),
    ("0F A8",               r"push %gs"),
    # reg/mem64 direct     mod = 0b11
    ("FF F0",               r"push %rax"),
    ("FF F2",               r"push %rdx"),
    # reg/mem64 indirect   mod < 0b11
    ("FF 30",               r"push 0x0(%rax)"), # 0b00 -> no displacement
    ("FF 31",               r"push 0x0(%rcx)"),
    ("FF 70 00",            r"push 0x0(%rax)"), # 0b01 -> disp8
    ("FF 70 1A",            r"push 0x1a(%rax)"),
    ("FF B0 23 01 00 00",   r"push 0x123(%rax)"), # 0b10 -> disp32
    ("FF B2 23 01 00 00",   r"push 0x123(%rdx)"),
    ("FF 35 00 00 00 00",   r"push 0x0(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    ("FF 35 12 34 56 78",   r"push 0x78563412(%rip)"),
    # reg/mem64 (extended GPRs)
    ("41 FF F0",            r"push %r8"),
    ("41 FF F7",            r"push %r15"),
    ("41 FF 30",            r"push 0x0(%r8)"),
    ("41 FF 70 1A",         r"push 0x1a(%r8)"),
    ("41 FF B0 23 01 00 00",r"push 0x123(%r8)"),
    ("41 FF 35 00 00 00 00",r"push 0x0(%rip)"),

    #
    # POP
    #
    # reg64
    ("58",                  r"pop %rax"),
    ("5A",                  r"pop %rdx"),
    ("5F",                  r"pop %rdi"),
    # reg64 (extended GPRs)
    ("41 58",               r"pop %r8"),
    ("41 5F",               r"pop %r15"),
    # fs/gs
    ("0F A1",               r"pop %fs"),
    ("0F A9",               r"pop %gs"),
    # reg/mem64 direct     mod = 0b11
    ("8F C0",               r"pop %rax"),
    ("8F C2",               r"pop %rdx"),
    # reg/mem64 indirect   mod < 0b11
    ("8F 00",               r"pop 0x0(%rax)"), # 0b00 -> no displacement
    ("8F 01",               r"pop 0x0(%rcx)"),
    ("8F 40 00",            r"pop 0x0(%rax)"), # 0b01 -> disp8
    ("8F 40 1A",            r"pop 0x1a(%rax)"),
    ("8F 80 23 01 00 00",   r"pop 0x123(%rax)"), # 0b10 -> disp32
    ("8F 82 23 01 00 00",   r"pop 0x123(%rdx)"),
    ("8F 05 00 00 00 00",   r"pop 0x0(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    ("8F 05 12 34 56 78",   r"pop 0x78563412(%rip)"),
    # reg/mem64 (extended GPRs)
    ("41 8F C0",            r"pop %r8"),
    ("41 8F C7",            r"pop %r15"),
    ("41 8F 00",            r"pop 0x0(%r8)"),
    ("41 8F 40 1A",         r"pop 0x1a(%r8)"),
    ("41 8F 80 23 01 00 00",r"pop 0x123(%r8)"),
    ("41 8F 05 00 00 00 00",r"pop 0x0(%rip)"),

    #
    # CALL
    #
    # rel32off
    ("E8 00 00 00 00",      r"call $rip+0x0"),
    ("E8 12 34 56 78",      r"call $rip+0x78563412"),
    # reg/mem64 direct     mod = 0b11
    ("FF D0",               r"call *%rax"),
    ("FF D2",               r"call *%rdx"),
    # reg/mem64 indirect   mod < 0b11
    ("FF 10",               r"call *0x0(%rax)"), # 0b00 -> no displacement
    ("FF 11",               r"call *0x0(%rcx)"),
    ("FF 50 00",            r"call *0x0(%rax)"), # 0b01 -> disp8
    ("FF 50 1A",            r"call *0x1a(%rax)"),
    ("FF 90 23 01 00 00",   r"call *0x123(%rax)"), # 0b10 -> disp32
    ("FF 92 23 01 00 00",   r"call *0x123(%rdx)"),
    ("FF 15 00 00 00 00",   r"call *0x0(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    ("FF 15 12 34 56 78",   r"call *0x78563412(%rip)"),
    # reg/mem64 (extended GPRs)
    ("41 FF D0",            r"call *%r8"),
    ("41 FF D7",            r"call *%r15"),
    ("41 FF 10",            r"call *0x0(%r8)"),
    ("41 FF 50 1A",         r"call *0x1a(%r8)"),
    ("41 FF 90 23 01 00 00",r"call *0x123(%r8)"),
    ("41 FF 15 00 00 00 00",r"call *0x0(%rip)"),

    #
    # RET
    #
    ("C3",                  r"ret"),
    # imm16
    ("C2 00 00",            r"ret $0x0"),
    ("C2 12 34",            r"ret $0x3412"),

    #
    # NOP
    #
    ("90",                  r"nop"),
    # reg/mem64 direct     mod = 0b11
    ("0F 1F C0",            r"nop %eax"),
    ("0F 1F C2",            r"nop %edx"),
    ("48 0F 1F C0",         r"nop %rax"),
    ("48 0F 1F C2",         r"nop %rdx"),
    # reg/mem64 indirect   mod < 0b11
    ("0F 1F 00",            r"nop 0x0(%rax)"), # 0b00 -> no displacement
    ("0F 1F 01",            r"nop 0x0(%rcx)"),
    ("0F 1F 40 00",         r"nop 0x0(%rax)"), # 0b01 -> disp8
    ("0F 1F 40 1A",         r"nop 0x1a(%rax)"),
    ("0F 1F 80 23 01 00 00",r"nop 0x123(%rax)"), # 0b10 -> disp32
    ("0F 1F 82 23 01 00 00",r"nop 0x123(%rdx)"),
    ("0F 1F 05 00 00 00 00",r"nop 0x0(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    ("0F 1F 05 12 34 56 78",r"nop 0x78563412(%rip)"),
    # ...

    #
    # INT 3
    #
    ("CC",                  r"int 3"),

    #
    # Arithmetic
    #
    # XOR EAX imm32
    ("35 00 00 00 00",      r"xor $0x0, %eax"),
    ("35 12 34 56 78",      r"xor $0x78563412, %eax"),
    # XOR RAX imm32
    ("48 35 00 00 00 00",   r"xor $0x0, %rax"),
    ("48 35 12 34 56 78",   r"xor $0x78563412, %rax"),
    # ADD EAX imm32
    ("05 00 00 00 00",      r"add $0x0, %eax"),
    ("05 12 34 56 78",      r"add $0x78563412, %eax"),
    # ADD RAX imm32
    ("48 05 00 00 00 00",   r"add $0x0, %rax"),
    ("48 05 12 34 56 78",   r"add $0x78563412, %rax"),
    # MUL reg/mem64 direct     mod = 0b11
    ("F7 E0",               r"mul %eax"),
    ("F7 E2",               r"mul %edx"),
    ("48 F7 E0",            r"mul %rax"),
    ("48 F7 E2",            r"mul %rdx"),
    # MUL reg/mem64 indirect   mod < 0b11
    ("F7 20",               r"mul 0x0(%rax)"), # 0b00
    ("F7 21",               r"mul 0x0(%rcx)"),
    ("F7 60 00",            r"mul 0x0(%rax)"), # 0b01
    ("F7 60 1A",            r"mul 0x1a(%rax)"),
    ("F7 A0 23 01 00 00",   r"mul 0x123(%rax)"), # 0b10
    ("F7 A2 23 01 00 00",   r"mul 0x123(%rdx)"),
    ("F7 25 00 00 00 00",   r"mul 0x0(%rip)"), # 0b00 & 0b101=r/m
    ("F7 25 12 34 56 78",   r"mul 0x78563412(%rip)"),
    # CMP EAX imm32
    ("3D 00 00 00 00",      r"cmp $0x0, %eax"),
    ("3D 12 34 56 78",      r"cmp $0x78563412, %eax"),
    # CMP RAX imm32
    ("48 3D 00 00 00 00",   r"cmp $0x0, %rax"),
    ("48 3D 12 34 56 78",   r"cmp $0x78563412, %rax"),
    # CMP reg/mem64,reg64 direct     mod = 0b11
    ("39 C0",               r"cmp %eax, %eax"),
    ("39 C2",               r"cmp %eax, %edx"),
    ("39 C8",               r"cmp %ecx, %eax"),
    ("39 D0",               r"cmp %edx, %eax"),
    ("48 39 C0",            r"cmp %rax, %rax"),
    ("48 39 C2",            r"cmp %rax, %rdx"),
    ("48 39 C8",            r"cmp %rcx, %rax"),
    ("48 39 D0",            r"cmp %rdx, %rax"),
    # CMP reg/mem64,reg64 indirect   mod < 0b11
    ("39 00",               r"cmp %eax, 0x0(%rax)"), # 0b00
    ("39 01",               r"cmp %eax, 0x0(%rcx)"),
    ("39 40 00",            r"cmp %eax, 0x0(%rax)"), # 0b01
    ("39 40 1A",            r"cmp %eax, 0x1a(%rax)"),
    ("39 42 1A",            r"cmp %eax, 0x1a(%rdx)"),
    ("39 48 1A",            r"cmp %ecx, 0x1a(%rax)"),
    ("39 50 1A",            r"cmp %edx, 0x1a(%rax)"),
    ("39 80 23 01 00 00",   r"cmp %eax, 0x123(%rax)"), # 0b10
    ("39 82 23 01 00 00",   r"cmp %eax, 0x123(%rdx)"),
    ("39 05 00 00 00 00",   r"cmp %eax, 0x0(%rip)"), # 0b00 & 0b101=r/m
    ("39 05 12 34 56 78",   r"cmp %eax, 0x78563412(%rip)"),
    ("48 39 01",            r"cmp %rax, 0x0(%rcx)"), # w/rex
    ("48 39 40 1A",         r"cmp %rax, 0x1a(%rax)"),
    ("48 39 48 1A",         r"cmp %rcx, 0x1a(%rax)"),
    ("48 39 82 23 01 00 00",r"cmp %rax, 0x123(%rdx)"),

    #
    # MOV
    #
    # reg/mem64,reg64     direct     mod = 0b11
    ("89 C0",               r"mov %eax, %eax"),
    ("89 C2",               r"mov %eax, %edx"),
    ("89 C8",               r"mov %ecx, %eax"),
    ("89 D0",               r"mov %edx, %eax"),
    ("48 89 C0",            r"mov %rax, %rax"),
    ("48 89 C2",            r"mov %rax, %rdx"),
    ("48 89 C8",            r"mov %rcx, %rax"),
    ("48 89 D0",            r"mov %rdx, %rax"),
    ("49 89 D0",            r"mov %rdx, %r8"), # rex.b
    # reg/mem64,reg64     indirect   mod < 0b11
    ("89 00",               r"mov %eax, 0x0(%rax)"), # 0b00
    ("89 01",               r"mov %eax, 0x0(%rcx)"),
    ("89 40 00",            r"mov %eax, 0x0(%rax)"), # 0b01
    ("89 40 1A",            r"mov %eax, 0x1a(%rax)"),
    ("89 42 1A",            r"mov %eax, 0x1a(%rdx)"),
    ("89 48 1A",            r"mov %ecx, 0x1a(%rax)"),
    ("89 50 1A",            r"mov %edx, 0x1a(%rax)"),
    ("89 80 23 01 00 00",   r"mov %eax, 0x123(%rax)"), # 0b10
    ("89 82 23 01 00 00",   r"mov %eax, 0x123(%rdx)"),
    ("89 05 00 00 00 00",   r"mov %eax, 0x0(%rip)"), # 0b00 & 0b101=r/m
    ("89 05 12 34 56 78",   r"mov %eax, 0x78563412(%rip)"),
    ("48 89 01",            r"mov %rax, 0x0(%rcx)"), # w/rex
    ("48 89 40 1A",         r"mov %rax, 0x1a(%rax)"),
    ("48 89 48 1A",         r"mov %rcx, 0x1a(%rax)"),
    ("48 89 82 23 01 00 00",r"mov %rax, 0x123(%rdx)"),
    ("49 89 82 23 01 00 00",r"mov %rax, 0x123(%r10)"), # rex.b
    # reg64,reg/mem64    direct      mod = 0b11
    ("8B C0",               r"mov %eax, %eax"),
    ("8B C2",               r"mov %edx, %eax"),
    ("8B C8",               r"mov %eax, %ecx"),
    ("8B D0",               r"mov %eax, %edx"),
    ("48 8B C0",            r"mov %rax, %rax"),
    ("48 8B C2",            r"mov %rdx, %rax"),
    ("48 8B C8",            r"mov %rax, %rcx"),
    ("48 8B D0",            r"mov %rax, %rdx"),
    ("49 8B D0",            r"mov %r8, %rdx"), # rex.b
    # reg64,reg/mem64     indirect   mod < 0b11
    ("8B 00",               r"mov 0x0(%rax), %eax"), # 0b00
    ("8B 01",               r"mov 0x0(%rcx), %eax"),
    ("8B 40 00",            r"mov 0x0(%rax), %eax"), # 0b01
    ("8B 40 1A",            r"mov 0x1a(%rax), %eax"),
    ("8B 42 1A",            r"mov 0x1a(%rdx), %eax"),
    ("8B 48 1A",            r"mov 0x1a(%rax), %ecx"),
    ("8B 50 1A",            r"mov 0x1a(%rax), %edx"),
    ("8B 80 23 01 00 00",   r"mov 0x123(%rax), %eax"), # 0b10
    ("8B 82 23 01 00 00",   r"mov 0x123(%rdx), %eax"),
    ("8B 05 00 00 00 00",   r"mov 0x0(%rip), %eax"), # 0b00 & 0b101=r/m
    ("8B 05 12 34 56 78",   r"mov 0x78563412(%rip), %eax"),
    ("48 8B 01",            r"mov 0x0(%rcx), %rax"), # w/rex
    ("48 8B 40 1A",         r"mov 0x1a(%rax), %rax"),
    ("48 8B 48 1A",         r"mov 0x1a(%rax), %rcx"),
    ("48 8B 82 23 01 00 00",r"mov 0x123(%rdx), %rax"),
    ("49 8B 82 23 01 00 00",r"mov 0x123(%r10), %rax"), # rex.b
]

__TESTS_SINGLE_INSTR_NEGATIVE = [
    #
    # Branching (JMP, JB, JE, JNE)
    #
    # rel8off
    ("EB E6",               r"jmp $rip-0x1A"),
    ("72 E6",               r"jb $rip-0x1A"),
    ("74 E6",               r"je $rip-0x1A"),
    ("75 E6",               r"jne $rip-0x1A"),
    # rel32off
    ("E9 E6 FF FF FF",      r"jmp $rip-0x1A"),
    ("0F 82 E6 FF FF FF",   r"jb $rip-0x1A"),
    ("0F 84 E6 FF FF FF",   r"je $rip-0x1A"),
    ("0F 85 E6 FF FF FF",   r"jne $rip-0x1A"),

    #
    # PUSH
    #
    # imm8
    ("6A E6",               r"push $0xffffffffffffffe6"),
    # imm64 (sign-extended 32b)
    ("68 E6 FF FF FF",      r"push $0xffffffffffffffe6"),
    # reg/mem64 indirect   mod < 0b11
    ("FF 70 E6",            r"push -0x1a(%rax)"), # 0b01 -> disp8
    ("FF B2 E6 FF FF FF",   r"push -0x1a(%rdx)"), # 0b10 -> disp32
    ("FF 35 E6 FF FF FF",   r"push -0x1a(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    # reg/mem64 (extended GPRs)
    ("41 FF 70 E6",         r"push -0x1a(%r8)"),
    ("41 FF B0 E6 FF FF FF",r"push -0x1a(%r8)"),
    ("41 FF 35 E6 FF FF FF",r"push -0x1a(%rip)"),

    #
    # POP
    #
    # reg/mem64 indirect   mod < 0b11
    ("8F 40 E6",            r"pop -0x1a(%rax)"), # 0b01 -> disp8
    ("8F 82 E6 FF FF FF",   r"pop -0x1a(%rdx)"), # 0b10 -> disp32
    ("8F 05 E6 FF FF FF",   r"pop -0x1a(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    # reg/mem64 (extended GPRs)
    ("41 8F 40 E6",         r"pop -0x1a(%r8)"),
    ("41 8F 80 E6 FF FF FF",r"pop -0x1a(%r8)"),
    ("41 8F 05 E6 FF FF FF",r"pop -0x1a(%rip)"),

    #
    # CALL
    #
    # rel32off
    ("E8 E6 FF FF FF",      r"call $rip-0x1a"),
    # reg/mem64 indirect   mod < 0b11
    ("FF 50 E6",            r"call *-0x1a(%rax)"), # 0b01 -> disp8
    ("FF 90 E6 FF FF FF",   r"call *-0x1a(%rax)"), # 0b10 -> disp32
    ("FF 15 E6 FF FF FF",   r"call *-0x1a(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    # reg/mem64 (extended GPRs)
    ("41 FF 50 E6",         r"call *-0x1a(%r8)"),
    ("41 FF 90 E6 FF FF FF",r"call *-0x1a(%r8)"),
    ("41 FF 15 E6 FF FF FF",r"call *-0x1a(%rip)"),

    #
    # RET
    #
    ("C2 FF FF",            r"ret $0xffff"),

    #
    # NOP
    #
    ("0F 1F 40 E6",         r"nop -0x1a(%rax)"), # 0b01 -> disp8
    ("0F 1F 80 E6 FF FF FF",r"nop -0x1a(%rax)"), # 0b10 -> disp32
    ("0F 1F 05 E6 FF FF FF",r"nop -0x1a(%rip)"), # special - 0b00 + 0b101=r/m -> disp32(%rip)
    # ...

    #
    # Arithmetic
    #
    # XOR EAX imm32
    ("35 E6 FF FF FF",      r"xor $0xffffffe6, %eax"),
    # XOR RAX imm32
    ("48 35 E6 FF FF FF",   r"xor $0xffffffffffffffe6, %rax"),
    # ADD EAX imm32
    ("05 E6 FF FF FF",      r"add $0xffffffe6, %eax"),
    # ADD RAX imm32
    ("48 05 E6 FF FF FF",   r"add $0xffffffffffffffe6, %rax"),
    # MUL reg/mem64 indirect   mod < 0b11
    ("F7 60 E6",            r"mul -0x1a(%rax)"), # 0b01
    ("F7 A0 E6 FF FF FF",   r"mul -0x1a(%rax)"), # 0b10
    ("F7 25 E6 FF FF FF",   r"mul -0x1a(%rip)"), # 0b00 & 0b101=r/m
    # CMP EAX imm32
    ("3D E6 FF FF FF",      r"cmp $0xffffffe6, %eax"),
    # CMP RAX imm32
    ("48 3D E6 FF FF FF",   r"cmp $0xffffffffffffffe6, %rax"),
    # CMP reg/mem64,reg64 indirect   mod < 0b11
    ("39 40 E6",            r"cmp %eax, -0x1a(%rax)"), # 0b01
    ("39 80 E6 FF FF FF",   r"cmp %eax, -0x1a(%rax)"), # 0b10
    ("39 05 E6 FF FF FF",   r"cmp %eax, -0x1a(%rip)"), # 0b00 & 0b101=r/m
    ("48 39 40 E6",         r"cmp %rax, -0x1a(%rax)"),
    ("48 39 82 E6 FF FF FF",r"cmp %rax, -0x1a(%rdx)"),

    #
    # MOV
    #
    # reg/mem64,reg64     indirect   mod < 0b11
    ("89 40 E6",            r"mov %eax, -0x1a(%rax)"), # 0b01
    ("89 80 E6 FF FF FF",   r"mov %eax, -0x1a(%rax)"), # 0b10
    ("89 05 E6 FF FF FF",   r"mov %eax, -0x1a(%rip)"), # 0b00 & 0b101=r/m
    ("48 89 40 E6",         r"mov %rax, -0x1a(%rax)"),
    ("48 89 82 E6 FF FF FF",r"mov %rax, -0x1a(%rdx)"),
    ("49 89 82 E6 FF FF FF",r"mov %rax, -0x1a(%r10)"), # rex.b
    # reg64,reg/mem64     indirect   mod < 0b11
    ("8B 40 E6",            r"mov -0x1a(%rax), %eax"), # 0b01
    ("8B 80 E6 FF FF FF",   r"mov -0x1a(%rax), %eax"), # 0b10
    ("8B 05 E6 FF FF FF",   r"mov -0x1a(%rip), %eax"), # 0b00 & 0b101=r/m
    ("48 8B 40 E6",         r"mov -0x1a(%rax), %rax"),
    ("48 8B 82 E6 FF FF FF",r"mov -0x1a(%rdx), %rax"),
    ("49 8B 82 E6 FF FF FF",r"mov -0x1a(%r10), %rax"), # rex.b
]

__TESTS = [
    __TESTS_SINGLE_INSTR_BASIC,
    __TESTS_SINGLE_INSTR_NEGATIVE
]

def run():
    for test in __TESTS:
        for (argv, expected) in test:
            expected = expected.lower()
            result = subprocess.check_output(__BIN + " " + argv, shell=True).decode("ascii")
            sanitized = ' '.join(result.split()).lower()

            if expected not in sanitized:
                print("Test-case failed!")
                print("HEX: " + argv)
                print("Expected: " + expected)
                print("Got:      " + sanitized)
                return

    print("All tests completed SUCCESSFULLY!")

if __name__== "__main__":
    run()
