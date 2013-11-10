/* RealBoy Emulator: Free, Fast, Yet Accurate, Game Boy/Game Boy Color Emulator.
 * Copyright (C) 2013 Sergio Andrés Gómez del Real
 *
 * This program is free software; you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by   
 * the Free Software Foundation; either version 2 of the License, or    
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

/* Misc defines */
.set NULL, 0

/* Offsets to registers in address space */
.set DIV_REG, 0xff04 # DIV
.set TIMA_REG, 0xff05 # TIMA
.set TMA_REG, 0xff06 # TMA
.set TAC_REG, 0xff07 # TAC
.set LCDC_REG, 0xff40 # LCD Control
.set LCDS_REG, 0xff41 # LCD Status
.set SCR_X, 0xff43 # SCROLL X
.set LY_REG, 0xff44 # LY
.set LYC_REG, 0xff45 # LYC
.set SPD_REG, 0xff4d # Speed
.set VRAM_DMA_SRC, 0xff51 # VRAM DMA Source
.set VRAM_DMA_DST, 0xff53 # VRAM DMA Destination
.set VRAM_DMA, 0xff55 # VRAM DMA
.set IR_REG, 0xff0f # Interrupt Request
.set IE_REG, 0xffff # Interrupt Enable

/* Interrupt masks in LCD Status register */
.set LY_LYC_FLAG, 0x4 # LY/LYC coincidence
.set H_BLN_INT, 0x08 # HBLANK interrupt
.set V_BLN_INT, 0x10 # VBLANK interrupt
.set OAM_INT, 0x20 # OAM interrupt
.set LY_LYC_INT, 0x40 # LY/LYC coincidence interrupt

/* Bit-control masks for set, reset and bit operations */
.set BIT_1, 0x01
.set BIT_2, 0x02
.set BIT_3, 0x04
.set BIT_4, 0x08
.set BIT_5, 0x10
.set BIT_6, 0x20
.set BIT_7, 0x40
.set BIT_8, 0x80

/* Flag (F) register masks */
.set F_ZERO, 0x80 # zero flag
.set F_SUBTRACT, 0x40 # add/subtract flag
.set F_HCARRY, 0x20 # half carry flag
.set F_CARRY, 0x10 # carry flag

/* General-purpose registers offsets */
.set F, 0 # flags
.set A, 1 # accumulator
.set C, 8 # general
.set B, 9 # general
.set E, 16 # general
.set D, 17 # general
.set L, 24 # general
.set H, 25 # general

/* Register pairs offsets */
.set AF, 0
.set BC, 8
.set DE, 16
.set HL, 24

/* Stack Pointer and Program Counter registers offsets */
.set SP, 32
.set PC, 40

/* Addressing modes */
.set REG, 0x02 # operand is register
.set REG_IND, 0x04 # operand is in memory pointed by register
.set IMM, 0x08 # operand is immediate
.set IMM_IND, 0x10 # operand is in memory pointed by immediate value

/* Addressing bytes or words */
.set BYTE, ~0xff
.set WORD, ~0xffff

.set DELAY, 0x4
.set RD_XOR_WR, 0x2
.set RD_WR, 0x1

.code64

/* Global symbols */
.globl rom_exec, addr_sp_ptrs, regs_sets, z80_ldex, z80_cb, gb_vbln_clks, gb_line_clks, gb_clk_rate, gb_oam_clks, gb_hblank_clks, gb_vram_clks, tac_counter, tac_on, tac_reload, nb_spr, lcd_vbln_hbln_ctrl, cpu_cur_mode, hbln_dma, hbln_dma_src, hbln_dma_dst, hdma_on
.type rom_exec, @function
.type addr_sp_ptrs, @object
.type regs_sets, @object
.type z80_ldex, @object
.type z80_cb, @object
.type gb_vbln_clks, @object
.type gb_line_clks, @object
.type gb_clk_rate, @object
.type gb_oam_clks, @object
.type gb_hblank_clks, @object
.type gb_vram_clks, @object
.type gb_hblank_clks, @object
.type tac_counter, @object
.type tac_on, @object
.type tac_reload, @object
.type nb_spr, @object

/* Text section */
.text

/*
 * Prepare to execute
 */
rom_exec:
	pushq %rbp # ABI says these belong to calling function
	pushq %rbx # ...
	pushq %r12 # ...
	pushq %r13 # ...
	pushq %r14 # ...
	pushq %r15 # ...
	movq $addr_sp, %r13 # set PC to 0x0000
	addq %rdi, %r13
	movq $z80_ldex, %r15 # %r15 holds pointer to opcode records
	movq (%r13), %r12 # %r12 holds the instruction's opcode
	call exec_next # GO GO GO!
	popq %r15 # comply with ABI; restore registers that belong to calling function
	popq %r14 # ...
	popq %r13 # ...
	popq %r12 # ...
	popq %rbx # ...
	popq %rbp # ...
	ret # return and change game

/*
 * Execution routine.
 */
exec_next:
	movq %r15, %rbx # pointer to instructions' records
	andq $0xff, %r12 # mask in opcode (record offset)
	shlq $5, %r12 # multiply by 32; each record is 32 bytes
	addq %r12, %rbx # point to instruction's record
	movq (%rbx), %r14 # save cycle duration of instruction
	movq %r14, %r12 # copy
	shrq $56, %r14 # cycle duration is the most significant byte
	shrq $40, %r12 # get byte that tells if instruction is a memory access
	andq $0xff, %r12
	movq %rbx, %rdx # pass record as argument
	addq $28, %rbx # instruction's function is in the last 4 bytes of record
	movl (%ebx), %ebx # copy function
	testq $1, gbddb # are we debugging?
	jz no_ddb # no
	movq %r13, %rsi # argument to debugger
	movq %rdx, %rbp # save reg
	call gddb_main
	movq %rbp, %rdx # restore reg
no_ddb:
	movq $0, inst_is_cb
	testq $DELAY, %r12 # should we delay instruction execution due to memory access?
	jz 1f # no; execute instruction normally
	jmp execute_precise # the instruction requires more precision than per-instruction execution
1:
	call *%rbx # execute instruction
	testb $1, inst_is_cb # if this instruction
	jnz no_ddb
	/* The reason for doing the following is very subtle, but it must be done */
	movq regs_sets+PC, %r13 # copy PC register
	movq $addr_sp_ptrs, %rdi # copy address spaces
	movq %r13, %r11 # save
	shrq $9, %r13 # get...
	andq $0x78, %r13 # ...offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
proc_ints:
	cmpb $0x76, (%r13) # special case if current instruction is halt (opcode 0x76)
	je 1f # halt
	testq $1, ime_flag # interrupts enabled?
	jz end_ints # no; skip interrupt processing
1:
	movq $1, %rcx # bit mask 'counter'
	movq $ints_offs, %r10 # pointer to offsets to interrupt addresses
	jmp 2f # process interrupts
1:
	testq $0x10, %rcx # the end?
	jnz 4f # yes
	shlq $1, %rcx # next bit
	addq $8, %r10
2:
	testq %rcx, IR_REG+addr_sp # is there a request at bit %rcx?
	jz 1b # no; next bit
	testq %rcx, IE_REG+addr_sp # is the interrupt requested enabled?
	jz 1b # no; next bit
	/* We have an interrupt */
	movq $regs_sets, %r11 # pointer to registers
	cmpb $0x76, (%r13) # special case if current instruction is halt (opcode 0x76)
	jne 3f # not halt
	incq %r13 # we are executing halt, so increment PC
	incq PC(%r11) # and increment PC register
	testq $1, ime_flag # interrupts enabled?
	jz end_ints # no; skip interrupt processing
3:
	movq $0, ime_flag # disable interrupts
	notq %rcx # we must clear the interrupt request bit
	andb %cl, IR_REG+addr_sp # clear interrupt request bit
	subq $2, SP(%r11) # decrement stack pointer
	movq PC(%r11), %rdx # copy PC register
	movq SP(%r11), %rbp # copy SP register
	movq $addr_sp_ptrs, %rdi # copy address spaces
	movq %rbp, %rax # save
	shrq $9, %rbp # get...
	andq $0x78, %rbp # ...offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # pointer to stack
	movw %dx, (%rbp) # push return address to stack
	xorq %r13, %r13 # PC register = 0
	addq (%r10), %r13 # new PC for interrupt
	movq $addr_sp_ptrs, %rdi # copy address spaces
	movq %r13, %rdx # save
	movq %r13, PC(%r11) # copy new PC register
	shrq $9, %r13 # get...
	andq $0x78, %r13 # ...offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %rdx, %r13 # restore
	addq (%rdi), %r13 # new PC
4:
end_ints:
	movq cpu_cur_mode, %rcx # if current mode is CGB, timer and divider stuff are 2 times faster
	shrq %cl, %r14 # double ticks if necessary
	movq (%r13), %r12 # copy opcode for next instruction
lcd_refrshing:
	testq $0x80, LCDC_REG+addr_sp # is LCD on?
	jnz 1f # yes
	jmp timer_divider_update # no; execute next instruction
1:
	movq %r14, temp_var
	movq LCDS_REG+addr_sp, %rbx # copy status register
	andq $3, %rbx # mask in lower 3 bits
	cmpq $1, %rbx # are we at vblank?
	je v_blank # yes
	cmpq $3, %rbx # are we at transfer?
	je vram # yes
	cmpq $2, %rbx # are we at oam?
	je oam # yes
h_blank:
	subq %r14, gb_hblank_clks # is h_blank period over?
	js hblank_exit # yes
	jmp end_lcd_refrshing # no; execute next instruction
hblank_exit:
	testq $1, gboy_mode # if CGB, check for HBlank-Driven DMA
	jz 1f
h_blank_chk_dma:
	testq $1, hdma_on # check for VRAM DMA
	jz 1f
	movq VRAM_DMA+addr_sp, %rbx
	andq $0x7f, %rbx
	decb %bl
	andb $0x80, VRAM_DMA+addr_sp # clear length
	orb %bl, VRAM_DMA+addr_sp # copy new length
	movq hbln_dma_src, %rsi # source address
	movq hbln_dma_dst, %rdi # destination address
	movq $2, %rcx # two double words (16 byte)
	rep
	movsq
	addq $16, hbln_dma_src
	addq $16, hbln_dma_dst
	movq VRAM_DMA_SRC+addr_sp, %rbx
	xchg %bx, %bx
	addq $16, %rbx
	xchg %bx, %bx
	movw %bx, VRAM_DMA_SRC+addr_sp
	movq VRAM_DMA_DST+addr_sp, %rbx
	xchg %bx, %bx
	addq $16, %rbx
	xchg %bx, %bx
	movw %bx, VRAM_DMA_DST+addr_sp
	decq hbln_dma
	jns 1f
	movq $0, hdma_on
	movb $0xff, VRAM_DMA+addr_sp # reset register
1:
	movq gb_hblank_clks, %r14 # load h_blank ticks
	incb LY_REG+addr_sp # increment LY
	andq $~LY_LYC_FLAG, LCDS_REG+addr_sp # unset flag
	movq LY_REG+addr_sp, %rbx # copy current line
	cmpb %bl, LYC_REG+addr_sp # compare registers
	jne 1f # not equal
	orq $LY_LYC_FLAG, LCDS_REG+addr_sp # equal; set flag
	testq $0x40, LCDS_REG+addr_sp # interrupt enabled?
	jz 1f # no; don't request interrupt
	orq $2, IR_REG+addr_sp # request LCD STAT interrupt
1:
	cmpb $144, LY_REG+addr_sp # end with scan lines?
	jb 1f # no
	addq %r14, gb_vbln_clks # add ticks
	movq gb_hblank_clks+8, %r14 # load hblank ticks from backup
	movq %r14, gb_hblank_clks # restore hblank ticks
	orq $1, LCDS_REG+addr_sp # go vblank
	movq gb_vbln_clks+8, %r14
	movq gb_line_clks, %rbx
	subq %rbx, %r14
	movq %r14, lcd_vbln_hbln_ctrl
	testq $V_BLN_INT, LCDS_REG+addr_sp # is vblank enabled?
	jz hblank_end
	orq $2, IR_REG+addr_sp # request LCDS interrupt
hblank_end:
	orq $1, IR_REG+addr_sp # request vblank interrupt
	jmp end_lcd_refrshing # execute next instruction
1:
	addq %r14, gb_oam_clks
	movq gb_hblank_clks+8, %r14
	movq %r14, gb_hblank_clks
	orq $2, LCDS_REG+addr_sp # change mode to oam
	testq $OAM_INT, LCDS_REG+addr_sp # is oam enabled?
	jz 1f # XXX
	orq $2, IR_REG+addr_sp # request lcd status interrupt
1:
	jmp end_lcd_refrshing # execute next instruction
v_blank:
	subq %r14, gb_vbln_clks # is v_blank period over?
	js 2f # yes
	movq lcd_vbln_hbln_ctrl, %rax # ticks for horizontal line control
	movq gb_vbln_clks, %r14 # copy vblank ticks
	cmpq %r14, %rax # need to increment LY?
	ja 1f # yes; increment LY
	jmp end_lcd_refrshing # no; execute next instruction
1:
	movq gb_line_clks, %r14 # copy ticks-per-horizontal-line
	subq %r14, lcd_vbln_hbln_ctrl # one line less
	incb LY_REG+addr_sp # increment line (LY)
	andq $~LY_LYC_FLAG, LCDS_REG+addr_sp # unset flag
	movq LY_REG+addr_sp, %rbx # copy current line
	cmpb %bl, LYC_REG+addr_sp # compare registers
	jne 1f # not equal
	orq $LY_LYC_FLAG, LCDS_REG+addr_sp # equal; set flag
	testq $0x40, LCDS_REG+addr_sp # interrupt enabled?
	jz 1f # no; don't request interrupt
	orq $2, IR_REG+addr_sp # request LCD STAT interrupt
1:
	jmp end_lcd_refrshing # execute next instruction
2:
	movq gb_vbln_clks, %r14 # copy tick
	addq %r14, gb_oam_clks # add difference to new period
	movq gb_vbln_clks+8, %rax # restart
	movq %rax, gb_vbln_clks #  v_blank period
	movb $0, LY_REG+addr_sp # reset current line
	andq $~LY_LYC_FLAG, LCDS_REG+addr_sp # unset flag
	movq LY_REG+addr_sp, %rbx # copy current line
	cmpb %bl, LYC_REG+addr_sp # compare registers
	jne 1f # not equal
	orq $LY_LYC_FLAG, LCDS_REG+addr_sp # equal; set flag
	testq $0x40, LCDS_REG+addr_sp # interrupt enabled?
	jz 1f # no; don't request interrupt
	orq $2, IR_REG+addr_sp # request LCD STAT interrupt
1:
	andq $~3, LCDS_REG+addr_sp # reset period
	orq $2, LCDS_REG+addr_sp # set to oam period
vbln_end:
	call proc_evts
	testq $1, %rax # return value; if 'q' pressed, change rom
	jnz change_game
	cmpb $0, skip_next_frame # skip next frame?
	jne 1f # yes
	call vid_frame_update # no; update frame
1:
	call frame_skip # call frame skip function
	movb %al, skip_next_frame # save return value
	jmp end_lcd_refrshing # execute next instruction
oam:
	subq %r14, gb_oam_clks # is oam period over?
	js oam_exit # yes
	jmp end_lcd_refrshing # no; execute next instruction
oam_exit:
	#movq $0, nb_spr
	#testq $2, addr_sp+LCDC_REG
	#jz 1f
	#call get_nb_spr # get number of sprites for line
1:
	movq gb_oam_clks, %r14 # load excess from oam ticks
	movq gb_vram_clks, %r10 # copy vram ticks
	addq %r14, %r10 # add the excess (negative value) to vram ticks
	movq $spr_extr_cycles, %r14 # pointer to extra cycles
	movq nb_spr, %r11 # copy number of sprites
	shlq $3, %r11 # scale
	addq %r11, %r14 # offset to extra cycles
	movq (%r14), %r11 # copy extra cycles
	movq %r11, spr_cur_extr # save extra cycles
	addq (%r14), %r10 # add extra cycles to vram ticks
	movq addr_sp+SCR_X, %r14
	andq $0x4, %r14
	addq %r14, %r10
	movq %r10, gb_vram_clks # save number of cycles for mode 2 (VRAM)
	movq gb_oam_clks+8, %r14 # copy oam ticks' backup
	movq %r14, gb_oam_clks # restore oam ticks
	orq $3, LCDS_REG+addr_sp # change mode to vram read mode
	jmp end_lcd_refrshing # execute next instruction
vram:
	subq %r14, gb_vram_clks # is transfer period over?
	js vram_exit # yes
	jmp end_lcd_refrshing # no; execute next instruction
vram_exit:
	movq gb_vram_clks, %r14 # load excess from vram ticks
	movq gb_hblank_clks, %r10 # copy hblank ticks
	addq %r14, %r10 # add the excess (negative value) to hblank ticks
	subq spr_cur_extr, %r10 # subtract extra cycle ticks
	movq addr_sp+SCR_X, %r14
	andq $0x4, %r14
	subq %r14, %r10
	movq %r10, gb_hblank_clks # save hblank ticks
	movq gb_vram_clks+8, %r14 # copy vram ticks' backup
	movq %r14, gb_vram_clks # restore vram ticks
	andq $~3, LCDS_REG+addr_sp # change mode to hblank
	movq skip_next_frame, %rdi # argument for render_scanline
	call render_scanline
	movq LCDS_REG+addr_sp, %rdi # is hblank interrupt enabled?
	andq $H_BLN_INT, %rdi # is hblank interrupt enabled?
	shrq $2, %rdi # shift bit to position
	orq %rdi, IR_REG+addr_sp # request/ignore lcd status interrupt
vram_end:
end_lcd_refrshing:
	movq temp_var, %r14
timer_divider_update:
	movq cpu_cur_mode, %rcx # if current mode is CGB, timer and divider stuff are 2 times faster
	shlq %cl, %r14 # double ticks if necessary
	testq $1, tac_on # test for TAC on bit
	jz 5f # skip all TAC, TIMA and TMA stuff
	xorq %rcx, %rcx # zero
	movq %r14, %rcx
	movq div_ctrl, %rdx
	jmp 4f
1:
	decq tac_counter
	jnz 3f
2:
	movq tac_reload, %rax
	movq %rax, tac_counter # reload counter
	addb $1, addr_sp+TIMA_REG
	jnc 3f
	orq $4, IR_REG+addr_sp # request timer interrupt
	movb addr_sp+TMA_REG, %al
	movb %al, addr_sp+TIMA_REG # reload
3:
	subb $4, %cl
	jz 5f
4:
	addb $4, %dl # advance div control
	lahf # load flags to %ah
	testb $0x10, %ah # test if div passed a 16 cycle boundary
	jnz 1b
	jmp 3b
5:
	addb %r14b, div_ctrl # advance div control
	jnc exec_next
	incb DIV_REG+addr_sp
	jmp exec_next

/* Play another game! */
change_game:
	# reset values
	movq gb_vram_clks+8, %r14
	movq %r14, gb_vram_clks # reset vram ticks
	movq gb_hblank_clks+8, %r14
	movq %r14, gb_hblank_clks # reset hblank ticks
	movq gb_oam_clks+8, %r14
	movq %r14, gb_oam_clks # reset oam ticks
	movq gb_vbln_clks+8, %rax
	movq %rax, gb_vbln_clks # reset v_blank ticks
	movq $1, ime_flag # enable interrupts
	cld
	movq $set_zero_addr, %rdi
	movq $0, %rax
	movq $(set_zero_addr_end-set_zero_addr), %rcx
	shrq $3, %rcx
	rep
	stosq # reset registers
	movq $0, fullscreen
	ret

.p2align 4
/*
 * We must execute the instruction in a per-cycle basis
 */
execute_precise:
	movq $0, write_is_delayed
	movq %r14, %rbp # copy ticks
	shrq $2, %rbp # scale
	decq %rbp # start counting with one less
beg_exe:
	testq $3, write_is_delayed
	jz 1f
	testq $1, write_is_delayed
	jnz mem_delayed
	jmp end_execute_precise
1:
	decq %rbp # advance tick counter
	jnz 1f
	testq $RD_WR, %r12 # we should execute instruction if it reads AND writes memory
	jz 2f
	orq %r12, write_is_delayed # write to memory is delayed one more cycle
	call *%rbx
	jmp 2f
	#jmp mem_delayed
1:
	jns 2f
	orq %r12, write_is_delayed
	call *%rbx # we should execute instruction now if memory is read XOR written
last_run:
2:
	addb $4, div_ctrl # advance div control (per-cycle control)
	lahf # load flags to %ah
	testb $0x10, %ah # test if div passed a 16 byte boundary
	jz 2f
	testq $1, tac_on # test for TAC on bit
	jz 2f # skip all TAC, TIMA and TMA stuff
	decq tac_counter
	jnz 2f
	movq tac_reload, %rax
	movq %rax, tac_counter # reload counter
	addb $1, addr_sp+TIMA_REG
	jnc 2f
	orq $4, IR_REG+addr_sp # request timer interrupt
	movb addr_sp+TMA_REG, %al
	movb %al, addr_sp+TIMA_REG # reload
2:
	testq $0xff, div_ctrl # test
	jnz 3f
	incb DIV_REG+addr_sp
3:
	jmp beg_exe
mem_delayed:
	movb %dl, (%rsi) # do memory write
	cmpq $0xff00, %r11
	jb 1f
	cmpq $0xff80, %r11
	jae 1f
	movq %r11, %rdi
	movq %rdx, %rsi
	andq $0xff, %rdi
	andq $0xff, %rsi
	call io_ctrl
1:
	movq $2, write_is_delayed
	jmp last_run
end_execute_precise:
lcd_refrshing_del:
	movq cpu_cur_mode, %rcx # if current mode is CGB, timer and divider stuff are 2 times faster
	shrq %cl, %r14 # double ticks if necessary
	testq $0x80, LCDC_REG+addr_sp # is LCD on?
	jz proc_ints_del # no
1:
	movq LCDS_REG+addr_sp, %rax # copy status register
	andq $3, %rax # mask in lower 3 bits
	cmpq $1, %rax # are we at vblank?
	je v_blank_del # yes
	cmpq $3, %rax # are we at transfer?
	je vram_del # yes
	cmpq $2, %rax # are we at oam?
	je oam_del # yes
h_blank_del:
	subq %r14, gb_hblank_clks # is h_blank period over?
	js hblank_exit_del # yes
	jmp proc_ints_del # no; execute next instruction
hblank_exit_del:
	testq $1, gboy_mode # if CGB, check for HBlank-Driven DMA
	jz 1f
h_blank_chk_dma_del:
	testq $1, hdma_on # check for VRAM DMA
	jz 1f
	movq VRAM_DMA+addr_sp, %rbx
	andq $0x7f, %rbx
	decb %bl
	andb $0x80, VRAM_DMA+addr_sp # clear length
	orb %bl, VRAM_DMA+addr_sp # copy new length
	movq hbln_dma_src, %rsi # source address
	movq hbln_dma_dst, %rdi # destination address
	movq $2, %rcx # two double words (16 byte)
	rep
	movsq
	addq $16, hbln_dma_src
	addq $16, hbln_dma_dst
	movq VRAM_DMA_SRC+addr_sp, %rbx
	xchg %bx, %bx
	addq $16, %rbx
	xchg %bx, %bx
	movw %bx, VRAM_DMA_SRC+addr_sp
	movq VRAM_DMA_DST+addr_sp, %rbx
	xchg %bx, %bx
	addq $16, %rbx
	xchg %bx, %bx
	movw %bx, VRAM_DMA_DST+addr_sp
	decq hbln_dma
	jns 1f
	movq $0, hdma_on
	movb $0xff, VRAM_DMA+addr_sp # reset register
1:
	movq gb_hblank_clks, %rax # load h_blank ticks
	incb LY_REG+addr_sp # increment LY
	andq $~LY_LYC_FLAG, LCDS_REG+addr_sp # unset flag
	pushq %rax
	movq LY_REG+addr_sp, %rax # copy current line
	cmpb %al, LYC_REG+addr_sp # compare registers
	popq %rax
	jne 1f # not equal
	orq $LY_LYC_FLAG, LCDS_REG+addr_sp # equal; set flag
	testq $0x40, LCDS_REG+addr_sp # interrupt enabled?
	jz 1f # no; don't request interrupt
	orq $2, IR_REG+addr_sp # request LCD STAT interrupt
1:
	cmpb $144, LY_REG+addr_sp # end with scan lines?
	jb 1f # no
	addq %rax, gb_vbln_clks # add ticks
	movq gb_hblank_clks+8, %rax # load hblank ticks from backup
	movq %rax, gb_hblank_clks # restore hblank ticks
	orq $1, LCDS_REG+addr_sp # go vblank
	movq gb_vbln_clks+8, %rax
	pushq %rbx
	movq gb_line_clks, %rbx
	subq %rbx, %rax
	popq %rbx
	movq %rax, lcd_vbln_hbln_ctrl
	testq $V_BLN_INT, LCDS_REG+addr_sp # is vblank enabled?
	jz hblank_end_del
	orq $2, IR_REG+addr_sp # request LCDS interrupt
hblank_end_del:
	orq $1, IR_REG+addr_sp # request vblank interrupt
	jmp proc_ints_del # execute next instruction
1:
	addq %rax, gb_oam_clks
	movq gb_hblank_clks+8, %rax
	movq %rax, gb_hblank_clks
	orq $2, LCDS_REG+addr_sp # change mode to oam
	testq $OAM_INT, LCDS_REG+addr_sp # is oam enabled?
	jz 1f # XXX
	orq $2, IR_REG+addr_sp # request lcd status interrupt
1:
	jmp proc_ints_del # execute next instruction
v_blank_del:
	subq %r14, gb_vbln_clks # is v_blank period over?
	js 2f # yes
	movq lcd_vbln_hbln_ctrl, %rax # ticks for horizontal line control
	cmpq gb_vbln_clks, %rax # need to increment LY?
	ja 1f # yes; increment LY
	jmp proc_ints_del # no; execute next instruction
1:
	movq gb_line_clks, %rax # copy ticks-per-horizontal-line
	subq %rax, lcd_vbln_hbln_ctrl # one line less
	incb LY_REG+addr_sp # increment line (LY)
	andq $~LY_LYC_FLAG, LCDS_REG+addr_sp # unset flag
	movq LY_REG+addr_sp, %rax # copy current line
	cmpb %al, LYC_REG+addr_sp # compare registers
	jne 1f # not equal
	orq $LY_LYC_FLAG, LCDS_REG+addr_sp # equal; set flag
	testq $0x40, LCDS_REG+addr_sp # interrupt enabled?
	jz 1f # no; don't request interrupt
	orq $2, IR_REG+addr_sp # request LCD STAT interrupt
1:
	jmp proc_ints_del # execute next instruction
2:
	movq gb_vbln_clks, %rax # copy tick
	addq %rax, gb_oam_clks # add difference to new period
	movq gb_vbln_clks+8, %rax # restart
	movq %rax, gb_vbln_clks #  v_blank period
	movb $0, LY_REG+addr_sp # reset current line
	andq $~LY_LYC_FLAG, LCDS_REG+addr_sp # unset flag
	movq LY_REG+addr_sp, %rax # copy current line
	cmpb %al, LYC_REG+addr_sp # compare registers
	jne 1f # not equal
	orq $LY_LYC_FLAG, LCDS_REG+addr_sp # equal; set flag
	testq $0x40, LCDS_REG+addr_sp # interrupt enabled?
	jz 1f # no; don't request interrupt
	orq $2, IR_REG+addr_sp # request LCD STAT interrupt
1:
	andq $~3, LCDS_REG+addr_sp # reset period
	orq $2, LCDS_REG+addr_sp # set to oam period
vbln_end_del:
	call proc_evts
	testq $1, %rax # return value; if 'q' pressed, change rom
	jnz change_game
	cmpb $0, skip_next_frame # skip next frame?
	jne 1f # yes
	call vid_frame_update # no; update frame
1:
	call frame_skip # call frame skip function
	movb %al, skip_next_frame # save return value
	jmp proc_ints_del # execute next instruction
oam_del:
	subq %r14, gb_oam_clks # is oam period over?
	js oam_exit_del # yes
	jmp proc_ints_del # no; execute next instruction
oam_exit_del:
	#movq $0, nb_spr
	#testq $2, addr_sp+LCDC_REG
	#jz 1f
	#call get_nb_spr # get number of sprites for line
1:
	movq gb_oam_clks, %rax # load excess from oam ticks
	movq gb_vram_clks, %r10 # copy vram ticks
	addq %rax, %r10 # add the excess (negative value) to vram ticks
	movq $spr_extr_cycles, %rax # pointer to extra cycles
	movq nb_spr, %r11 # copy number of sprites
	shlq $3, %r11 # scale
	addq %r11, %rax # offset to extra cycles
	movq (%rax), %r11 # copy extra cycles
	movq %r11, spr_cur_extr # save extra cycles
	addq (%rax), %r10 # add extra cycles to vram ticks
	movq addr_sp+SCR_X, %rax
	andq $0x4, %rax
	addq %rax, %r10
	movq %r10, gb_vram_clks # save number of cycles for mode 2 (VRAM)
	movq gb_oam_clks+8, %rax # copy oam ticks' backup
	movq %rax, gb_oam_clks # restore oam ticks
	orq $3, LCDS_REG+addr_sp # change mode to vram read mode
	jmp proc_ints_del # execute next instruction
vram_del:
	subq %r14, gb_vram_clks # is transfer period over?
	js vram_exit_del # yes
	jmp proc_ints_del # no; execute next instruction
vram_exit_del:
	movq gb_vram_clks, %rax # load excess from vram ticks
	movq gb_hblank_clks, %r10 # copy hblank ticks
	addq %rax, %r10 # add the excess (negative value) to hblank ticks
	subq spr_cur_extr, %r10 # subtract extra cycle ticks
	movq addr_sp+SCR_X, %rax
	andq $0x4, %rax
	subq %rax, %r10
	movq %r10, gb_hblank_clks # save hblank ticks
	movq gb_vram_clks+8, %rax # copy vram ticks' backup
	movq %rax, gb_vram_clks # restore vram ticks
	andq $~3, LCDS_REG+addr_sp # change mode to hblank
	movq skip_next_frame, %rdi # argument for render_scanline
	call render_scanline
	movq LCDS_REG+addr_sp, %rdi # is hblank interrupt enabled?
	andq $H_BLN_INT, %rdi # is hblank interrupt enabled?
	shrq $2, %rdi # shift bit to position
	orq %rdi, IR_REG+addr_sp # request/ignore lcd status interrupt
vram_end_del:
proc_ints_del:
	cmpb $0x76, (%r13) # special case if current instruction is halt (opcode 0x76)
	je 1f # halt
	testq $1, ime_flag # interrupts enabled?
	jz end_ints_del # no; skip interrupt processing
1:
	movq $1, %rcx # bit mask 'counter'
	movq $ints_offs, %r10 # pointer to offsets to interrupt addresses
	jmp 2f # process interrupts
1:
	testq $0x10, %rcx # the end?
	jnz 4f # yes
	shlq $1, %rcx # next bit
	addq $8, %r10
2:
	testq %rcx, IR_REG+addr_sp # is there a request at bit %rcx?
	jz 1b # no; next bit
	testq %rcx, IE_REG+addr_sp # is the interrupt requested enabled?
	jz 1b # no; next bit
	/* We have an interrupt */
	movq $regs_sets, %r11 # pointer to registers
	cmpb $0x76, (%r13) # special case if current instruction is halt (opcode 0x76)
	jne 3f # not halt
	incq %r13 # we are executing halt, so increment PC
	incq PC(%r11) # and increment PC register
	testq $1, ime_flag # interrupts enabled?
	jz end_ints_del # no; skip interrupt processing
3:
	movq $0, ime_flag # disable interrupts
	notq %rcx # we must clear the interrupt request bit
	andb %cl, IR_REG+addr_sp # clear interrupt request bit
	subq $2, SP(%r11) # decrement stack pointer
	movq PC(%r11), %rdx # copy PC register
	movq SP(%r11), %rbp # copy SP register
	movq $addr_sp_ptrs, %rdi # copy address spaces
	movq %rbp, %rax # save
	shrq $9, %rbp # get...
	andq $0x78, %rbp # ...offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # pointer to stack
	movw %dx, (%rbp) # push return address to stack
	xorq %r13, %r13 # PC register = 0
	addq (%r10), %r13 # new PC for interrupt
	movq $addr_sp_ptrs, %rdi # copy address spaces
	movq %r13, %rdx # save
	movq %r13, PC(%r11) # copy new PC register
	shrq $9, %r13 # get...
	andq $0x78, %r13 # ...offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %rdx, %r13 # restore
	addq (%rdi), %r13 # new PC
4:
end_ints_del:
	/* The reason for doing the following is very subtle, but it must be done */
	movq regs_sets+PC, %r13 # copy PC register
	movq $addr_sp_ptrs, %rdi # copy address spaces
	movq %r13, %r11 # save
	shrq $9, %r13 # get...
	andq $0x78, %r13 # ...offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	/* This test, before proc_ints, is also due to a subtlety XXX */
#testq $1, just_enabled # last instruction was reti?
#jz proc_ints # no
#movq $0, just_enabled # yes; execute one instruction before processing interrupts
#	jmp end_ints # skip interrupt processing
	movq (%r13), %r12 # copy opcode for next instruction
	jmp exec_next # execute next instruction

/* 
 * Group 1: Load
 */
/* 
 * ld HL, SP+dd
 */
	op_ld_sp_imm_hl:
	addq $2, regs_sets+PC # advance PC register
	addq $2, %r13 # advance PC
	movq regs_sets+SP, %r10 # get SP register
	movq %r10, regs_sets+HL # copy to HL
	andq $~0xff, regs_sets+F # reset flags
	xorq %rcx, %rcx # zero
	movsbw -1(%r13), %cx # copy byte signed-extended
	addw %cx, regs_sets+HL # add to HL
	addb %cl, %r10b # simulate CARRY and HCARRY
	pushf # push x86 flags
	popq %rcx # %rcx = x86's EFLAGS
	movq %rcx, %rdx # save x86 flags
	shlq $4, %rcx # move x86 carry bit to position in z80
	andq $F_CARRY, %rcx # mask in z80 carry bit
	orq %rcx, regs_sets+F # copy new z80 carry flag
	shlq $1, %rdx # move x86 hcarry bit to position in z80
	andq $F_HCARRY, %rdx # mask in z80 hcarry bit
	orq %rdx, regs_sets+F # copy new z80 hcarry flag
	ret
	

/* 
 * ld reg, reg
 *
 */
op_ld_reg_reg:
	movq $regs_sets, %rdi # pointer to register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq 2(%rdx), %rcx # get offset to destination register
	andq $0xff, %rcx # mask in byte
	addq %rcx, %rdi # point to register
	movb 4(%rdx), %cl # get offset to source register
	movq $regs_sets, %r10 # pointer to register set
	addq %rcx, %r10 # point to register
	movq (%r10), %r11 # get source register
	andq $0xff, %r11 # mask in byte
	andq $~0xff, (%rdi) # zero destination register
	orq %r11, (%rdi) # copy source register to destination register
	ret

/* 
 * ld (HL), imm
 *
 */
op_ld_imm_mem:
	movq $regs_sets, %rdi # register set
	addq $2, PC(%rdi) # advance PC register
	movq HL(%rdi), %rdi # copy HL register
	movq 1(%r13), %rdx # get immediate value
	addq $2, %r13 # advance PC
	andq $0xff, %rdx # mask in byte
	cmpq $0xff00, %rdi # writing to IO?
	jb 1f # no
	cmpq $0xff80, %rdi # writing to IO?
	jae 1f # no
	andq $0xff, %rdi # argument: IO address
	movq %rdx, %rsi # argument: new value
	call io_ctrl # handle IO
	ret
1:
	cmpq $0x7fff, %rdi # if memory write to 0-0x7fff, mbc functions
	jbe 2f # jump to mbc functions
	movq %rdi, %r11 # save address
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rdi, %r10 # get address
	movq %r11, %rdi # restore address
	addq (%r10), %rdi # get real address
	andq $~0xff, (%rdi) # zero byte
	orq %rdx, (%rdi) # copy byte
	movq %r11, %rdi # restore address
	ret
2:
	shrq $9, %rdi # get offset to mbc function
	andq $0x78, %rdi # mask bits
	movq $gb_mbc, %r10 # array of mbc functions
	addq %rdi, %r10 # point to mbc function
	movq (%r10), %rbx # copy function
	movq %rdx, %rdi # value as argument
	call *%rbx # handle mbc stuff
	ret

/* 
 * ld SP, HL
 */
op_ld_hl_sp:
	movq $regs_sets, %rdi # pointer to register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq HL(%rdi), %rbx # copy HL
	movq %rbx, SP(%rdi) # OPERATION: copy HL to SP
	ret

/* 
 * ld SP, imm
 */
op_ld_imm_sp:
	movq $regs_sets, %rdi # register set
	addq $3, PC(%rdi) # advance PC register
	movq 1(%r13), %rbx # get immediate value
	andq $0xffff, %rbx # mask in word
	movq %rbx, SP(%rdi) # copy new register
	addq $3, %r13
	ret

/* 
 * ld reg, imm
 */
op_ld_imm_reg:
	movq 2(%rdx), %rcx # offset to destination register
	andq $0xff, %rcx # mask in byte
	movq $regs_sets, %rdi # pointer to register set
	addq %rcx, %rdi # point to register
	movq 1(%r13), %rbx # get immediate value
	movq 24(%rdx), %rcx # BYTE or WORD
	movsx %ecx, %rcx # extend sign
	andq %rcx, (%rdi) # zero register
	notq %rcx # invert
	andq %rcx, %rbx # mask in byte/word
	orq %rbx, (%rdi) # copy new register
	movq 6(%rdx), %rcx # get instruction lenght
	andq $0xff, %rcx # mask in byte
	movq $regs_sets, %rdi # register set
	addq %rcx, PC(%rdi) # set PC
	addq %rcx, %r13 # new PC
	ret

/*
 * ld (imm), reg
 */
op_ld_reg_imm:
	movq 4(%rdx), %rcx # pointer to destination register
	andq $0xff, %rcx # mask in byte
	movq $regs_sets, %rdi # register set
	addq $3, PC(%rdi) # advance PC register
	addq %rcx, %rdi # point to register
	movq (%rdi), %r12 # copy register
	movq 24(%rdx), %rcx # BYTE or WORD
	movsx %ecx, %rcx # extend sign
	movq 1(%r13), %rdi # get immediate value
	addq $3, %r13 # advance PC
	andq $0xffff, %rdi # mask in word
	cmpq $0xff00, %rdi # writing to IO?
	jb 1f # no
	cmpq $0xff80, %rdi
	jae 1f
	andq $0xff, %rdi # argument
	andq $0xff, %r12
	movq %r12, %rsi # argument
	call io_ctrl
	ret
1:
	cmpq $0x7fff, %rdi # if memory write to 0-0x7fff, remap MBC
	jbe 2f # remap MBC
	movq %rdi, %r10 # save address
	shrq $9, %rdi # scale for address pointers
	andq $0x78, %rdi # mask bits
	movq $addr_sp_ptrs, %r11 # address pointers
	addq %rdi, %r11 # pointer to address
	movq %r10, %rdi # restore address
	addq (%r11), %rdi # get address
	andq %rcx, (%rdi) # mask word/byte
	notq %rcx # invert
	andq %rcx, %r12 # mask in byte/word
	orq %r12, (%rdi) # copy new register
	ret
2:
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $gb_mbc, %r10 # pointer to address
	addq %rdi, %r10
	movq (%r10), %rbx
	movq %r12, %rdi # value as argument
	call *%rbx # handle mbc stuff
	ret

/* 
 * ld mem, reg
 */
op_ld_reg_mem:
	movq $regs_sets, %rdi # register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq 2(%rdx), %rcx # offset to destination register
	andq $0xff, %rcx # mask in byte
	addq %rcx, %rdi # point to register
	movq (%rdi), %rdi # get register
	movq 4(%rdx), %rdx # source
	andq $0xff, %rdx # mask in byte
	movq $regs_sets, %r10 # register set
	addq %rdx, %r10 # point to register
	movb (%r10), %cl # register to copy
	cmpq $0xff00, %rdi # writing IO?
	jb 1f
	cmpq $0xff80, %rdi
	jae 1f
	andq $0xff, %rdi # argument
	movq %rcx, %rsi # argument
	call io_ctrl
	ret
1:
	cmpq $0x7fff, %rdi # if memory write to 0-0x7fff, handle mbc
	jbe 2f # it is a normal write
	movq %rdi, %r11 # save address
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rdi, %r10
	movq %r11, %rdi # restore address
	addq (%r10), %rdi
	andq $~0xff, (%rdi) # zero byte
	orq %rcx, (%rdi) # copy byte
	movq %r11, %rdi # restore address
	ret
2:
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $gb_mbc, %r10 # pointer to address
	addq %rdi, %r10
	movq (%r10), %rbx
	movq %rcx, %rdi # value as argument
	call *%rbx # handle mbc stuff
	ret

/* 
 * ld reg, mem
 */
op_ld_mem_reg:
	movq $regs_sets, %rdi # pointer to register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq 2(%rdx), %rcx # offset to destination register
	andq $0xff, %rcx # mask in byte
	addq %rcx, %rdi # point to destination register
	movq 4(%rdx), %rcx # offset to source register
	andq $0xff, %rcx # mask in byte
	movq $regs_sets, %r10 # register set
	addq %rcx, %r10 # point to source register
	movq (%r10), %rax # get source register (pointer to address)
	movq %rax, %r11 # save address
	shrq $9, %rax # get offset to pointer and base
	andq $0x78, %rax # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rax, %r10
	movq %r11, %rax
	addq (%r10), %rax
	movq (%rax), %rax # copy
	andq $~0xff, (%rdi) # zero byte in register
	orb %al, (%rdi) # OPERATION: copy byte
	ret

/* 
 * ldd (HL), A
 */
op_ldd_reg_mem:
	movq $regs_sets, %rdi # pointer to register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	pushq HL(%rdi) # push old HL
	decw HL(%rdi) # decrement new HL
	movq A(%rdi), %rcx # copy A register
	andq $0xff, %rcx
	popq %rdi # pop old HL
	cmpq $0xff00, %rdi # writing to IO?
	jb 1f
	cmpq $0xff80, %rdi
	jae 1f
	andq $0xff, %rdi # argument
	movq %rcx, %rsi # argument
	call io_ctrl
	ret
1:
	cmpq $0x7fff, %rdi # if memory write to 0-0x7fff, handle mbc
	jbe 2f # it is a normal write
	movq %rdi, %r11 # save address
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rdi, %r10
	movq %r11, %rdi
	addq (%r10), %rdi
	andq $~0xff, (%rdi) # zero byte
	orq %rcx, (%rdi) # copy byte
	ret
2:
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $gb_mbc, %r10 # pointer to address
	addq %rdi, %r10
	movq (%r10), %rbx
	movq %rcx, %rdi # push value as argument
	call *%rbx # handle mbc stuff
	ret

/* 
 * ldd A, (HL)
 */
op_ldd_mem_reg:
	movq $regs_sets, %rdi # register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	pushq HL(%rdi) # push old HL
	decw HL(%rdi) # decrement new HL
	incq %rdi # point to A
	popq %rax # pop old HL
	andq $0xffff, %rax # mask in register pair
	movq %rax, %r11 # save address
	shrq $9, %rax # get offset to pointer and base
	andq $0x78, %rax # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rax, %r10
	movq %r11, %rax
	addq (%r10), %rax
	movq (%rax), %rdx # copy
	andq $~0xff, (%rdi) # zero byte in register
	orb %dl, (%rdi) # copy byte
	ret

/* 
 * ldi (HL), A
 */
op_ldi_reg_mem:
	movq $regs_sets, %rdi # register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq HL(%rdi), %rax # save old HL
	incq HL(%rdi) # increment new HL
	movq A(%rdi), %rcx # copy A
	movq %rax, %rdi # copy old HL
	andq $0xff, %rcx
	cmpq $0xff00, %rdi # writing to IO?
	jb 1f
	cmpq $0xff80, %rdi
	jae 1f
	andq $0xff, %rdi # argument
	movq %rcx, %rsi # argument
	call io_ctrl
	ret
1:
	cmpq $0x7fff, %rdi # if memory write to 0-0x7fff, handle mbc
	jbe 2f # it is a normal write
	movq %rdi, %r11 # save address
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rdi, %r10
	movq %r11, %rdi
	addq (%r10), %rdi
	andq $~0xff, (%rdi) # zero byte
	orq %rcx, (%rdi) # copy byte
	ret
2:
	shrq $9, %rdi # get offset to pointer and base
	andq $0x78, %rdi # mask bits
	movq $gb_mbc, %r10 # pointer to address
	addq %rdi, %r10
	movq (%r10), %rbx
	movq %rcx, %rdi # push value as argument
	call *%rbx # handle mbc stuff
	ret

/* 
 * ldi A, (HL)
 */
op_ldi_mem_reg:
	movq $regs_sets, %rdi # register set
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq HL(%rdi), %rax # push old HL
	incq HL(%rdi) # decrement new HL
	incq %rdi # point to A
	andq $0xffff, %rax # mask in register pair
	movq %rax, %r11 # save address
	shrq $9, %rax # get offset to pointer and base
	andq $0x78, %rax # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rax, %r10
	movq %r11, %rax
	addq (%r10), %rax
	movq (%rax), %rdx # copy
	andq $~0xff, (%rdi) # zero byte in register
	orb %dl, (%rdi) # copy byte
	ret

/*
 * ld A, (nn)
 */
op_ld_imm_acc:
	leaq regs_sets+A, %rdi # pointer to A
	addq $3, regs_sets+PC # advance PC register
	movq 1(%r13), %rax # pointer to immediate
	addq $3, %r13 # advance PC
	andq $0xffff, %rax
	movq %rax, %r11 # save address
	shrq $9, %rax # get offset to pointer and base
	andq $0x78, %rax # mask bits
	movq $addr_sp_ptrs, %r10 # pointer to address
	addq %rax, %r10
	movq %r11, %rax
	addq (%r10), %rax
	movq (%rax), %rax # copy byte
	andq $~0xff, (%rdi) # zero register
	orb %al, (%rdi) # OPERATION
	ret

/* 
 * ld A, (0xff00+C)
 */
op_ld_io_reg_reg:
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	movq C(%r10), %rcx # copy C
	andq $~0xff, A(%r10) # zero A register
	andq $0xff, %rcx # mask in C
	movq $addr_sp, %rdi # pointer to address space
	addq %rcx, %rdi # pointer to byte
	addq $0xff00, %rdi # add
	movb (%rdi), %cl # copy byte
	orq %rcx, A(%r10) # copy register
	ret

/* 
 * ld A, (0xff00+imm)
 */
op_ld_io_reg_imm:
	movq $regs_sets, %r10 # pointer to registers
	addq $2, PC(%r10) # advance PC register
	movq 1(%r13), %rcx # copy C
	addq $2, %r13 # advance PC
	andq $~0xff, A(%r10) # zero register
	andq $0xff, %rcx # mask in byte
	movq $addr_sp, %rdi # pointer to address space
	addq %rcx, %rdi # pointer to byte
	addq $0xff00, %rdi # add
	movb (%rdi), %cl # copy byte
	orq %rcx, A(%r10) # copy register
	ret

/* 
 * ld (0xff00+C), A
 */
op_ld_reg_io_reg:
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	movq A(%r10), %rcx # copy A
	andq $0xff, %rcx # mask in A
	movq %rcx, %rsi # argument: new value to io port
	movq C(%r10), %rdi # copy register
	andq $0xff, %rdi # mask in C; argument
	call io_ctrl # call C function
	ret

/* 
 * ld (0xff00+imm), A
 */
op_ld_reg_io_imm:
	movq 1(%r13), %rdi # copy register
	movq $regs_sets, %r10 # pointer to registers
	addq $2, PC(%r10) # advance PC register
	addq $2, %r13 # advance PC
	movq A(%r10), %rsi # copy A
	andq $0xff, %rsi # mask in A; argument
	andq $0xff, %rdi # mask in byte
	call io_ctrl # call C function
	ret

/*
 * pop AF
 */
op_pop_af:
	movq $regs_sets, %r10 # register base
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	xorq %rcx, %rcx # zero
	movb 2(%rdx), %cl # offset to register
	addq %rcx, %r10 # pointer to register
	movq $regs_sets, %rdi # pointer to register set
	movq SP(%rdi), %rax # copy old SP register
	addq $2, SP(%rdi) # new SP register (address to pop from)
	movq $addr_sp_ptrs, %rdx
	movq %rax, %r11 # save address
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdx # point to address space
	movq %r11, %rax
	addq (%rdx), %rax # get offset
	movq (%rax), %rax # get byte
	andq $0xff00|F_HCARRY|F_CARRY|F_SUBTRACT|F_ZERO, %rax # mask in word
	movq %rax, (%r10) # copy new register pair
	ret

/*
 * pop BC, pop DE, pop HL
 */
op_pop:
	movq $regs_sets, %r10 # register base
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	xorq %rcx, %rcx # zero
	movb 2(%rdx), %cl # offset to register
	addq %rcx, %r10 # pointer to register
	movq $regs_sets, %rdi # pointer to register set
	movq SP(%rdi), %rax # copy old SP register
	addq $2, SP(%rdi) # new SP register (address to pop from)
	movq $addr_sp_ptrs, %rdx
	movq %rax, %r11 # save address
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdx # point to address space
	movq %r11, %rax
	addq (%rdx), %rax # get offset
	movq (%rax), %rax # get byte
	andq $0xffff, %rax # mask in word
	movq %rax, (%r10) # copy new register pair
	ret

/*
 * push BC, push DE, push AF, push HL
 *
 */
op_push:
	movq $regs_sets, %r10 # pointer to register set
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	movq $regs_sets, %rdi # pointer to register set
	xorq %rcx, %rcx # zero
	movb 2(%rdx), %cl # register offset
	addq %rcx, %r10 # point to register
	movq (%r10), %rbx # copy register pair
	subq $2, SP(%rdi) # new SP register
	movq SP(%rdi), %r12
	movq $addr_sp_ptrs, %rsi
	movq %r12, %rax # save
	shrq $9, %r12 # get
	andq $0x78, %r12 #  offset to pointer in addr_sp_ptrs
	addq %r12, %rsi # point to address space
	movq %rax, %r12 # restore
	addq (%rsi), %r12 # new PC
	movw %bx, (%r12)
	ret


/* 
 * Group 2: Arithmetic/Logic
 */
/* 
 * add A, reg8
 *
 */
op_add:
	movq $regs_sets, %r10 # pointer to registers
	andq $~0xff, (%r10) # clear flags
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	xorq %rax, %rax # zero
	movb 4(%rdx), %al # offset to source register
	movq $regs_sets+A, %rdi # pointer to A
	addq %rax, %r10 # pointer to source register
	movb (%r10), %dl # copy source register
	movq $regs_sets, %r10 # pointer to flags (F register)
	addb %dl, (%rdi) # OPERATION: add register to accumulator
	pushf # push x86 eflags
	popq %r11 # get x86 eflags
	movq %r11, %rdx # copy x86 eflags
	shlq $4, %rdx # move x86 carry bit into z80 carry position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, (%r10) # copy new z80 carry flag
	movq %r11, %rdx # copy x86 eflags
	shlq $1, %rdx # move x86 zero bit into z80 zero position
	andq $F_ZERO, %rdx # mask in zero bit
	orq %rdx, (%r10) # copy new z80 zero flag
	shlq $1, %r11 # move carry bit into position
	andq $F_HCARRY, %r11 # mask in carry bit
	orq %r11, (%r10) # new carry flag
	ret

/* 
 * add HL, reg16
 *
 */
op_add_wr:
	movq $regs_sets, %r10 # pointer to registers
	andq $~0x7f, (%r10) # reset flags except zero
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	xorq %rcx, %rcx
	xorq %rax, %rax
	movb 4(%rdx), %al # offset to source register
	movq $regs_sets+HL, %rdi # pointer to HL
	addq %rax, %r10 # pointer to source register
	movq (%r10), %rdx # copy source register
	movq $regs_sets, %r10 # pointer to flags (F register)
	movq (%rdi), %rax # copy HL
	addw %dx, (%rdi) # OPERATION: add register to HL
	pushf # push x86 eflags
	popq %r11 # get x86 eflags
	shlq $4, %r11 # move x86 carry bit into z80 carry position
	andq $F_CARRY, %r11 # mask in carry
	orq %r11, (%r10) # copy new carry flag
	addb %dl, %al # simulate half carry
	adcb %dh, %ah # simulate half carry
	pushf # save results
	popq %r11 # restore results
	shlq $1, %r11 # move x86 hcarry bit to position in z80
	andq $F_HCARRY, %r11 # mask in z80 hcarry bit
	orq %r11, (%r10) # copy new half carry flag
	ret

/*
 * add A, (HL)
 *
 */
op_add_mem:
	movq $regs_sets, %r10 # pointer to register set
	andq $~0xff, (%r10) # clear flags
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	xorq %rcx, %rcx # zero
	xorq %rax, %rax # zero
	movq $regs_sets+A, %rdi # pointer to A
	movq HL(%r10), %rax # copy HL
	movq $addr_sp_ptrs, %rdx
	movq %rax, %r11 # save
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdx # point to address space
	movq %r11, %rax
	addq (%rdx), %rax # get offset
	movq (%rax), %rax # get byte
	andq $0xff, %rax # mask byte
	addb %al, (%rdi) # OPERATION: add byte to accumulator
	pushf # push flags
	popq %rax # pop flags
	movq %rax, %rdx # copy flags
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, (%r10) # new carry flag
	movq %rax, %rdx # copy flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	orq %rdx, (%r10) # copy new zero flag
	shlq $1, %rax # move carry and half carry bit into position
	andq $F_HCARRY, %rax # mask in carry bit
	orq %rax, (%r10) # new carry flag
	ret

/*
 * add A, imm8
 *
 */
op_add_imm:
	movq $regs_sets, %r10 # pointer to flags
	andq $~0xff, (%r10) # clear flags
	addq $2, PC(%r10) # advance PC register
	movq 1(%r13), %rcx # copy immediate byte
	addq $2, %r13 # advance PC
	andq $0xff, %rcx
	movq $regs_sets+A, %rdi # pointer to A
	addb %cl, (%rdi) # OPERATION: add imm to accumulator
	pushf # push x86 flags
	popq %r11 # get x86 flags
	movq %r11, %rdx # copy x86 flags
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	orq %rdx, (%r10) # copy new zero flag
	shlq $1, %r11 # move carry bit into position
	andq $F_HCARRY, %r11 # mask in carry bit
	orq %r11, (%r10) # new carry flag
	ret

/* 
 * add SP, reg8
 */
op_add_sp_sign:
	addq $2, regs_sets+PC # advance PC register
	andq $~0xff, regs_sets+F # reset flags
	movq 1(%r13), %rcx # copy signed byte
	andq $0xff, %rcx
	addq $2, %r13 # advance PC
	movq regs_sets+SP, %rax # save
	cmpb $127, %cl
	ja 1f
	addw %cx, regs_sets+SP
	pushf # push flags
	addb %cl, %al
	pushf
	popq %rdx # pop flags
	shlq $4, %rdx # move carry bit to position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, regs_sets+F # new carry flag
	#addb %cl, %al # simulate half carry
	#pushf
	popq %rdx
	shlq $1, %rdx # move half carry bit to position
	andq $F_HCARRY, %rdx # mask in half carry bit
	orq %rdx, regs_sets+F # new carry flag
	ret
1:
	notb %cl
	incb %cl
	subw %cx, regs_sets+SP # OPERATION: add signed imm byte to SP
	pushf # push flags
	subb %cl, %al
	pushf # push flags
2:
	popq %rdx # pop flags
	shlq $4, %rdx # move carry bit to position
	andq $F_CARRY, %rdx # mask in carry bit
	notq %rdx
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, regs_sets+F # new carry flag
	#addb %cl, %al # simulate half carry
	#pushf
	popq %rdx
	shlq $1, %rdx # move half carry bit to position
	andq $F_HCARRY, %rdx # mask in half carry bit
	notq %rdx
	andq $F_HCARRY, %rdx # mask in carry bit
	orq %rdx, regs_sets+F # new carry flag
	ret

/* 
 * adc reg8, reg8
 *
 */
op_adc:
	pushq %rbp
	movq %rsp, %rbp
	xorq %rax, %rax # zero
	movb 4(%rdx), %al # copy offset to source register
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to registers
	addq %rax, %r10 # point to source register
	movb (%r10), %al # hold source register
	movq $regs_sets, %r10 # pointer to flags
	incq PC(%r10)
	movb regs_sets, %bl
	andb $F_CARRY, %bl
	shrb $4, %bl
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orb %bl, -8(%rbp) # copy z80 carry flag into x86 carry flag
	popf # pop new x86 flags including z80 carry flag
	adcb %al, (%rdi) # add to register
	pushf # push flags
	popq %r11 # pop flags
	movq %r11, %rdx
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	andq $~F_CARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rdx, (%r10) # copy new zero flag
	andq $~F_SUBTRACT, (%r10) # unset F_SUBTRACT
	movq %r11, %rdx
	shlq $1, %rdx # move carry bit into position
	andq $F_HCARRY, %rdx # mask in carry bit
	andq $~F_HCARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	incq %r13 # new PC
	popq %rbp
	ret

/*
 * adc A, (HL)
 *
 */
op_adc_mem:
	pushq %rbp
	movq %rsp, %rbp
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq HL(%r10), %rax # copy HL
	movq $addr_sp_ptrs, %rdx
	movq %rax, %r11 # save
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdx # point to address space
	movq %r11, %rax # restore
	addq (%rdx), %rax # get offset
	movq (%rax), %rax # get byte
	andq $0xff, %rax # mask byte
	movb regs_sets, %bl
	andb $F_CARRY, %bl
	shrb $4, %bl
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orb %bl, -8(%rbp) # copy z80 carry flag into x86 carry flag
	popf # pop new x86 flags incquding z80 carry flag
	adcb %al, (%rdi) # add to register
	pushf # push flags
	popq %rax # pop flags
	movq %rax, %rdx # push flags
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	andq $~F_CARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	movq %rax, %rdx # push flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rdx, (%r10) # copy new zero flag
	andq $~F_SUBTRACT, (%r10) # unset F_SUBTRACT
	movq %rax, %rdx # push flags
	shlq $1, %rdx # move carry bit into position
	andq $F_HCARRY, %rdx # mask in carry bit
	andq $~F_HCARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	incq %r13 # new PC
	popq %rbp
	ret

/*
 * adc A, imm8
 *
 */
op_adc_imm:
	pushq %rbp
	movq %rsp, %rbp
	movq 1(%r13), %rcx # copy immediate byte
	andq $0xff, %rcx
	xorq %rax, %rax # zero
	movb 4(%rdx), %al # copy offset to source register
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to flags
	incq PC(%r10)
	incq PC(%r10)
	movb regs_sets, %bl
	andb $F_CARRY, %bl
	shrb $4, %bl
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orb %bl, -8(%rbp) # copy z80 carry flag into x86 carry flag
	popf # pop new x86 flags incquding z80 carry flag
	adcb %cl, (%rdi) # add to register
	pushf # push flags
	popq %r11 # pop flags
	movq %r11, %rdx # push flags
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	andq $~F_CARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx # push flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rdx, (%r10) # copy new zero flag
	andq $~F_SUBTRACT, (%r10) # unset F_SUBTRACT
	movq %r11, %rdx # push flags
	shlq $1, %rdx # move carry bit into position
	andq $F_HCARRY, %rdx # mask in carry bit
	andq $~F_HCARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	addq $2, %r13 # new PC
	popq %rbp
	ret

/* 
 * sub
 *
 */
op_sub:
	xorq %rax, %rax # zero
	movb 4(%rdx), %al # copy offset to source register
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to registers
	andq $~0xff, (%r10) # clear flags
	incq PC(%r10) # advance PC register
	incq %r13 # advance PC
	addq %rax, %r10 # point to source register
	movb (%r10), %dl # hold source register
	movq $regs_sets, %r10 # pointer to flags
	subb %dl, (%rdi) # subtract to register
	pushf # push flags
	popq %r11 # pop flags
	movq %r11, %rdx
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx # pop flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	orq %rdx, (%r10) # copy new zero flag
	shlq $1, %r11 # move carry bit into position
	andq $F_HCARRY, %r11 # mask in carry bit
	orq %r11, (%r10) # new carry flag
	orq $F_SUBTRACT, (%r10) # set F_SUBTRACT
	ret

/*
 * sub A, (HL)
 *
 */
op_sub_mem:
	xorq %rcx, %rcx # zero
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to registers
	andq $~0xff, (%r10) # clear flags
	incq PC(%r10)
	incq %r13 # new PC
	movq HL(%r10), %rax # copy HL
	movq $addr_sp_ptrs, %rdx
	movq %rax, %r11 # save
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdx # point to address space
	movq %r11, %rax
	addq (%rdx), %rax # get offset
	movq (%rax), %rax # get byte
	andq $0xff, %rax # mask byte
	subb %al, (%rdi) # subtract to register
	pushf # push flags
	popq %rax # pop flags
	movq %rax, %rdx # push flags
	shlq $4, %rdx # move carry and half carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, (%r10) # new carry flag
	movq %rax, %rdx # pop flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	orq %rdx, (%r10) # copy new zero flag
	shlq $1, %rax # move carry and half carry bit into position
	andq $F_HCARRY, %rax # mask in carry bit
	orq %rax, (%r10) # new carry flag
	orq $F_SUBTRACT, (%r10) # set F_SUBTRACT
	ret

/*
 * sub A, imm8
 *
 */
op_sub_imm:
	movq 1(%r13), %rcx # copy immediate byte
	andq $0xff, %rcx
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to flags
	andq $~0xff, (%r10) # clear flags
	addq $2, PC(%r10)
	addq $2, %r13 # new PC
	subb %cl, (%rdi) # subtract to register
	pushf # push flags
	popq %r11 # pop flags
	movq %r11, %rdx
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	orq %rdx, (%r10) # copy new zero flag
	shlq $1, %r11 # move carry bit into position
	andq $F_HCARRY, %r11 # mask in carry bit
	orq %r11, (%r10) # new carry flag
	orq $F_SUBTRACT, (%r10) # set F_SUBTRACT
	ret

/* 
 * sbc
 *
 */
op_sbc:
	pushq %rbp
	movq %rsp, %rbp
	xorq %rax, %rax # zero
	movb 4(%rdx), %al # copy offset to source register
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	addq %rax, %r10 # point to source register
	movb (%r10), %al # hold source register
	movq $regs_sets, %r10 # pointer to flags
	movb regs_sets, %bl
	andb $F_CARRY, %bl
	shrb $4, %bl
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orb %bl, -8(%rbp) # copy z80 carry flag into x86 carry flag
	popf # pop new x86 flags including z80 carry flag
	sbbb %al, (%rdi) # subtract to register
	pushf # push flags
	popq %r11 # pop flags
	movq %r11, %rdx
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	andq $~F_CARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rdx, (%r10) # copy new zero flag
	or $F_SUBTRACT, (%r10) # unset F_SUBTRACT
	movq %r11, %rdx
	shlq $1, %rdx # move carry bit into position
	andq $F_HCARRY, %rdx # mask in carry bit
	andq $~F_HCARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	incq %r13 # new PC
	popq %rbp
	ret

/*
 * sbc A, (HL)
 *
 */
op_sbc_mem:
	pushq %rbp
	movq %rsp, %rbp
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq HL(%r10), %rax # copy HL
	movq $addr_sp_ptrs, %rdx
	movq %rax, %r11 # save
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdx # point to address space
	movq %r11, %rax # restore
	addq (%rdx), %rax # get offset
	movq (%rax), %rax # get byte
	andq $0xff, %rax # mask byte
	movb regs_sets, %bl
	andb $F_CARRY, %bl
	shrb $4, %bl
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orb %bl, -8(%rbp) # copy z80 carry flag into x86 carry flag
	popf # pop new x86 flags incquding z80 carry flag
	sbbb %al, (%rdi) # subtract to register
	pushf # push flags
	popq %rax # pop flags
	movq %rax, %rdx # push flags
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	andq $~F_CARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	movq %rax, %rdx # push flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rdx, (%r10) # copy new zero flag
	orq $F_SUBTRACT, (%r10) # unset F_SUBTRACT
	movq %rax, %rdx # push flags
	shlq $1, %rdx # move carry bit into position
	andq $F_HCARRY, %rdx # mask in carry bit
	andq $~F_HCARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	incq %r13 # new PC
	popq %rbp
	ret

/*
 * sbc A, imm8
 *
 */
op_sbc_imm:
	pushq %rbp
	movq %rsp, %rbp
	movq 1(%r13), %rcx # copy immediate byte
	andq $0xff, %rcx
	xorq %rax, %rax # zero
	movb 4(%rdx), %al # copy offset to source register
	movq $regs_sets+A, %rdi # pointer to A
	movq $regs_sets, %r10 # pointer to flags
	incq PC(%r10)
	incq PC(%r10)
	movb regs_sets, %bl
	andb $F_CARRY, %bl
	shrb $4, %bl
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orb %bl, -8(%rbp) # copy z80 carry flag into x86 carry flag
	popf # pop new x86 flags incquding z80 carry flag
	sbbb %cl, (%rdi) # subtract to register
	pushf # push flags
	popq %r11 # pop flags
	movq %r11, %rdx # push flags
	shlq $4, %rdx # move carry bit into position
	andq $F_CARRY, %rdx # mask in carry bit
	andq $~F_CARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	movq %r11, %rdx # push flags
	shlq $1, %rdx # move zero bit into position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rdx, (%r10) # copy new zero flag
	orq $F_SUBTRACT, (%r10) # unset F_SUBTRACT
	movq %r11, %rdx # push flags
	shlq $1, %rdx # move carry bit into position
	andq $F_HCARRY, %rdx # mask in carry bit
	andq $~F_HCARRY, (%r10) # zero carry flag
	orq %rdx, (%r10) # new carry flag
	addq $2, %r13 # new PC
	popq %rbp
	ret

/* 
 * xor accumulator, reg
 *
 */
op_xor_reg_accu:
	movq $regs_sets, %r10 # pointer to registers
	andq $~0xff, (%r10) # clear flags
	incq PC(%r10)
	incq %r13
	movq $regs_sets, %rdi # pointer to registers
	movq (%r10), %rbx # copy AF
	shrq $8, %rbx # copy A to low byte
	xorq %rcx, %rcx # zero
	movb 4(%rdx), %cl # copy register offset
	addq %rcx, %rdi # point to register
	movb (%rdi), %cl # copy register
	xorq %rcx, %rbx # xor A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~0xff, 1(%r10) # zero A
	orq %rbx, 1(%r10) # copy new A
	ret

/* 
 * cp accumulator, mem
 *
 */
op_cp_mem_ind:
	andq $~0xff, regs_sets+F # reset flags
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10) # set PC
	movq HL(%r10), %rax
	movq $addr_sp_ptrs, %rdi
	movq %rax, %r11 # save
	shrq $9, %rax # get
	andq $0x78, %rax #  offset to pointer in addr_sp_ptrs
	addq %rax, %rdi # point to address space
	movq %r11, %rax
	addq (%rdi), %rax # get offset
	movq (%rax), %rcx
	cmpb %cl, 1(%r10) # cp A-d8
	pushf # save flags
	popq %rax
	movq %rax, %rcx
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	movq %rax, %rcx
	shlq $4, %rcx # move zero bit into position
	andq $F_CARRY, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	movq %rax, %rcx
	shlq $1, %rcx # move zero bit into position
	andq $F_HCARRY, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	orq $F_SUBTRACT, (%r10) # zero flag
	incq %r13
	ret

/* 
 * cp accumulator, mem
 *
 */
op_cp_imm_acc:
	andq $~0xff, regs_sets+F # reset flags
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	incq PC(%r10)
	xorq %rcx, %rcx # zero
	movb 1(%r13), %cl # copy immediate value
	cmpb %cl, 1(%r10) # cp A-d8
	pushf # save flags
	popq %r11 # restore flags
	movq %r11, %rcx
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	movq %r11, %rcx
	shlq $4, %rcx # move zero bit into position
	andq $F_CARRY, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	movq %r11, %rcx
	shlq $1, %rcx # move zero bit into position
	andq $F_HCARRY, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	orq $F_SUBTRACT, (%r10) # zero flag
	addq $2, %r13
	ret

/* 
 * cp A, reg
 *
 */
op_cp_reg:
	andq $~0xff, regs_sets+F # reset flags
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq $regs_sets, %rdi # pointer to registers
	xorq %rcx, %rcx # zero
	movb 4(%rdx), %cl # copy register offset
	addq %rcx, %rdi # point to register
	movb (%rdi), %cl # copy register
	cmpb %cl, 1(%r10) # cp A-reg8
	pushf # save flags
	popq %r11 # restore flags
	movq %r11, %rcx
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	movq %r11, %rcx
	shlq $4, %rcx # move zero bit into position
	andq $F_CARRY, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	movq %r11, %rcx
	shlq $1, %rcx # move zero bit into position
	andq $F_HCARRY, %rcx # mask in zero flag
	orq %rcx, (%r10) # copy new zero flag value
	orq $F_SUBTRACT, (%r10) # zero flag
	incq %r13
	ret

/* 
 * and A, reg
 *
 */
op_and_reg_accu:
	andq $~0xff, regs_sets+F # reset flags
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq (%r10), %rbx # copy AF
	shrq $8, %rbx # copy A to low byte
	xorq %rcx, %rcx # zero
	movb 4(%rdx), %cl # copy register offset
	addq %rcx, %r10 # point to register
	movb (%r10), %cl # copy register
	andq %rcx, %rbx # and A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	movq $regs_sets, %r10
	orq %rcx, (%r10) # copy new zero flag value
	andq $~0xff, 1(%r10) # zero A
	orq %rbx, 1(%r10) # copy new A
	orq $F_HCARRY, (%r10) # set flag
	incq %r13
	ret

/* 
 * and accumulator, (HL)
 *
 */
op_and_mem_accu:
	movq $regs_sets, %r11 # pointer to registers
	incq PC(%r11)
	movq (%r11), %rdx # copy AF
	shrq $8, %rdx # copy A to low byte
	movq HL(%r11), %rcx # copy register
	movq $addr_sp_ptrs, %rdi
	movq %rcx, %r11 # save
	shrq $9, %rcx # get
	andq $0x78, %rcx #  offset to pointer in addr_sp_ptrs
	addq %rcx, %rdi # point to address space
	movq %r11, %rcx
	addq (%rdi), %rcx # get offset
	movq (%rcx), %rcx
	andq $0xff, %rcx # mask in byte
	movq $regs_sets, %r10 # pointer to registers
	andb %cl, 1(%r10) # or A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	movq $regs_sets, %r11 # pointer to registers
	andq $~F_ZERO, (%r11) # zero zero flag
	orq %rcx, (%r11) # copy new zero flag value
	andq $~F_SUBTRACT, (%r11) # zero flag
	orq $F_HCARRY, (%r11) # set flag
	andq $~F_CARRY, (%r11) # zero flag
	incq %r13
	ret

/* 
 * and accumulator, (imm)
 *
 */
op_and_imm_accu:
	movq 1(%r13), %rcx # copy immediate word
	andq $0xff, %rcx # mask in byte
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	incq PC(%r10)
	movq (%r10), %rdx # copy AF
	shrq $8, %rdx # copy A to low byte
	andq %rcx, %rdx # and A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~0xff, 1(%r10) # zero A
	orq %rdx, 1(%r10) # copy new A
	andq $~F_SUBTRACT, (%r10) # zero flag
	orq $F_HCARRY, (%r10) # set flag
	andq $~F_CARRY, (%r10) # zero flag
	addq $2, %r13
	ret

/* 
 * or accumulator, (imm)
 *
 */
op_or_imm_accu:
	movq 1(%r13), %rcx # copy immediate word
	andq $0xff, %rcx # mask in word
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	incq PC(%r10)
	movq (%r10), %rdx # copy AF
	shrq $8, %rdx # copy A to low byte
	orq %rcx, %rdx # or A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~0xff, 1(%r10) # zero A
	orq %rdx, 1(%r10) # copy new A
	andq $~F_SUBTRACT, (%r10) # zero flag
	andq $~F_HCARRY, (%r10) # zero flag
	andq $~F_CARRY, (%r10) # zero flag
	addq $2, %r13
	ret

/* 
 * xor accumulator, (imm)
 *
 */
op_xor_imm_accu:
	movq 1(%r13), %rcx # copy immediate word
	andq $0xff, %rcx # mask in word
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	incq PC(%r10)
	xorb %cl, 1(%r10) # xor A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~F_SUBTRACT, (%r10) # zero flag
	andq $~F_HCARRY, (%r10) # zero flag
	andq $~F_CARRY, (%r10) # zero flag
	addq $2, %r13
	ret

/* 
 * or accumulator, (HL)
 *
 */
op_or_mem_accu:
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq HL(%r10), %rcx # copy register HL
	movq $addr_sp_ptrs, %rdi
	movq %rcx, %r11 # save
	shrq $9, %rcx # get
	andq $0x78, %rcx #  offset to pointer in addr_sp_ptrs
	addq %rcx, %rdi # point to address space
	movq %r11, %rcx
	addq (%rdi), %rcx # get offset
	movq (%rcx), %rcx
	orb %cl, 1(%r10) # or A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~F_SUBTRACT, (%r10) # zero flag
	andq $~F_HCARRY, (%r10) # zero flag
	andq $~F_CARRY, (%r10) # zero flag
	incq %r13
	ret

/* 
 * xor accumulator, (HL)
 *
 */
op_xor_mem_accu:
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq HL(%r10), %rcx # copy register
	movq $addr_sp_ptrs, %rdi
	movq %rcx, %r11 # save
	shrq $9, %rcx # get
	andq $0x78, %rcx #  offset to pointer in addr_sp_ptrs
	addq %rcx, %rdi # point to address space
	movq %r11, %rcx # restore
	addq (%rdi), %rcx # get offset
	movq (%rcx), %rcx
	xorb %cl, 1(%r10) # xor A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~F_SUBTRACT, (%r10) # zero flag
	andq $~F_HCARRY, (%r10) # zero flag
	andq $~F_CARRY, (%r10) # zero flag
	incq %r13
	ret

/* 
 * or accumulator, reg
 *
 */
op_or_reg_accu:
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq (%r10), %rdi # copy AF
	shrq $8, %rdi # copy A to low byte
	xorq %rcx, %rcx # zero
	movb 4(%rdx), %cl # copy register offset
	addq %rcx, %r10 # point to register
	movb (%r10), %cl # copy register
	orq %rcx, %rdi # or A
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit into position
	andq $F_ZERO, %rcx # mask in zero flag
	movq $regs_sets, %r10
	andq $~F_ZERO, (%r10) # zero zero flag
	orq %rcx, (%r10) # copy new zero flag value
	andq $~0xff, 1(%r10) # zero A
	orq %rdi, 1(%r10) # copy new A
	andq $~F_SUBTRACT, (%r10) # zero flag
	andq $~F_HCARRY, (%r10) # zero flag
	andq $~F_CARRY, (%r10) # zero flag
	incq %r13
	ret

/* 
 * dec (HL)
 *
 */
op_dec_mem:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq HL(%rdi), %rcx # copy HL
	andq $~F_HCARRY, (%rdi) # clear HCARRY
	orq $F_SUBTRACT, (%rdi) # set SUBTRACT
	movq $addr_sp_ptrs, %r10
	movq %rcx, %r11 # save
	shrq $9, %rcx # get
	andq $0x78, %rcx #  offset to pointer in addr_sp_ptrs
	addq %rcx, %r10 # point to address space
	movq %r11, %rcx # restore
	addq (%r10), %rcx # get offset
	movq (%rcx), %rdx # get word
	subb $1, %dl # subtract, but don't write to memory yet
	pushf # save flags
	popq %rax # restore flags
	movq %rax, %rsi
	shlq $1, %rsi # move zero bit into position
	andq $F_ZERO, %rsi # mask in zero flag
	andq $~F_ZERO, (%rdi) # zero zero flag
	orq %rsi, (%rdi) # copy new zero flag value
	shlq $1, %rax # move zero bit into position
	andq $F_HCARRY, %rax # mask in zero flag
	andq $~F_HCARRY, (%rdi) # zero zero flag
	orq %rax, (%rdi) # copy new zero flag value
	orq $F_SUBTRACT, (%rdi) # set subtract flag
	incq %r13
	movq %rcx, %rsi
	ret

/* 
 * inc (HL)
 *
 */
op_inc_mem:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq HL(%rdi), %rcx # copy HL
	andq $~F_HCARRY, (%rdi) # clear HCARRY
	andq $~F_SUBTRACT, (%rdi) # clear SUBTRACT
	movq $addr_sp_ptrs, %r10
	movq %rcx, %r11 # save
	shrq $9, %rcx # get
	andq $0x78, %rcx #  offset to pointer in addr_sp_ptrs
	addq %rcx, %r10 # point to address space
	movq %r11, %rcx # restore
	addq (%r10), %rcx # get offset
	movq (%rcx), %rdx # get word
	addb $1, %dl # increment
	pushf # save flags
	popq %rax # restore flags
	movq %rax, %rsi
	shlq $1, %rsi # move zero bit into position
	andq $F_ZERO, %rsi # mask in zero flag
	andq $~F_ZERO, (%rdi) # zero zero flag
	orq %rsi, (%rdi) # copy new zero flag value
	shlq $1, %rax # move zero bit into position
	andq $F_HCARRY, %rax # mask in zero flag
	andq $~F_HCARRY, (%rdi) # zero zero flag
	orq %rax, (%rdi) # copy new zero flag value
	andq $~F_SUBTRACT, (%rdi) # zero zero flag
	incq %r13
	movq %rcx, %rsi
	ret

/* 
 * dec register
 *
 */
op_dec_reg:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq $regs_sets, %r10 # pointer to flags
	andq $~F_HCARRY, (%r10) # clear HCARRY
	orq $F_SUBTRACT, (%r10) # set SUBTRACT
	movq 2(%rdx), %rcx # register offset
	andq $0xff, %rcx # mask byte
	addq %rcx, %rdi # point to register
	movq (%rdi), %rdx # get word
	subb $1, (%rdi) # increment word
	pushf # push x86 flags
	popq %r11 # restore x86 flags
	movq %r11, %rdx # save x86 flags
	shlq $1, %rdx # move x86's zero bit into z80's zero position (0x80)
	andq $F_ZERO, %rdx # mask x86/z80 zero flag
	andq $~F_ZERO, (%r10) # clear z80's zero flag
	orq %rdx, (%r10) # copy new z80's zero flag value
	movq %r11, %rdx # restore x86 flags
	shlq $1, %rdx # move x86's AF bit into z80's HCARRY position
	andq $F_HCARRY, %rdx # mask x86/z80 AF/HCARRY flag
	andq $~F_HCARRY, (%r10) # clear z80's HCARRY flag
	orq %rdx, (%r10) # copy new z80's HCARRY flag value
	ret

/* 
 * inc register
 *
 */
op_inc_reg:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq $regs_sets, %r10 # pointer to flags
	andq $~F_HCARRY, (%r10) # clear HCARRY
	andq $~F_SUBTRACT, (%r10) # clear SUBTRACT
	movq 2(%rdx), %rcx # register offset
	andq $0xff, %rcx # mask byte
	addq %rcx, %rdi # point to register
	movq (%rdi), %rdx # get word
	addb $1, (%rdi) # increment word
	pushf # push x86 flags
	popq %r11 # restore x86 flags
	movq %r11, %rdx # save x86 flags
	shlq $1, %rdx # move x86's zero bit into z80's zero position (0x80)
	andq $F_ZERO, %rdx # mask x86/z80 zero flag
	andq $~F_ZERO, (%r10) # clear z80's zero flag
	orq %rdx, (%r10) # copy new z80's zero flag value
	movq %r11, %rdx # restore x86 flags
	shlq $1, %rdx # move x86's AF bit into z80's HCARRY position
	andq $F_HCARRY, %rdx # mask x86/z80 AF/HCARRY flag
	andq $~F_HCARRY, (%r10) # clear z80's HCARRY flag
	orq %rdx, (%r10) # copy new z80's HCARRY flag value
	ret

/* 
 * dec r16
 *
 */
op_inc_reg_sp:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	incw SP(%rdi) # OP: increment SP register
	ret

/* 
 * dec r16
 *
 */
op_dec_reg_sp:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	decw SP(%rdi)
	incq %r13
	ret

/* 
 * dec r16
 *
 */
op_dec_reg_wr:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq 2(%rdx), %rcx # register offset
	andq $0xff, %rcx # mask byte
	addq %rcx, %rdi # point to register
	decw (%rdi) # decrement word
	incq %r13
	ret

/* 
 * inc r16
 *
 */
op_inc_reg_wr:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi) # advance PC register
	incq %r13 # advance PC
	movq 2(%rdx), %rcx # register offset
	andq $0xff, %rcx # mask byte
	addq %rcx, %rdi # point to register
	incw (%rdi) # increment word
	ret

/* 
 * Group 3: Rotate/Shift/Swap
 */
/* 
 * sla (HL)
 *
 */
op_shf_lf_arth_mem:
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb (%rsi), %al # copy byte
	shrb $3, %al # move bit 7 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx
	salb %dl # rotate register with carry
	pushf # save flags
	popq %rcx # pop flags
	shlq $1, %rcx # shift bit on to zero position
	andq $F_ZERO, %rcx
	andq $~F_ZERO, (%rdi) # unset zero
	orq %rcx, (%rdi) # copy new zero bit
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * srl (HL)
 *
 */
op_shf_rgh_lg_mem:
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb (%rsi), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx
	shrb %dl # rotate register with carry
	pushf # save flags
	popq %rcx # pop flags
	shlq $1, %rcx # shift bit on to zero position
	andq $F_ZERO, %rcx
	andq $~F_ZERO, (%rdi) # unset zero
	orq %rcx, (%rdi) # copy new zero bit
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * sra (HL)
 *
 */
op_shf_rgh_arth_mem:
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb (%rsi), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx
	sarb %dl # rotate register with carry
	pushf # save flags
	popq %rcx # pop flags
	shlq $1, %rcx # shift bit on to zero position
	andq $F_ZERO, %rcx
	andq $~F_ZERO, (%rdi) # unset zero
	orq %rcx, (%rdi) # copy new zero bit
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * sra reg8
 *
 */
op_shf_rgh_arth:
	xorq %rcx, %rcx # zero
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movb (%r10), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	sarb (%r10) # rotate register with carry
	pushf # save flags
	popq %rcx # pop flags
	shlq $1, %rcx # shift bit on to carry-flag position
	andq $F_ZERO, %rcx
	andq $~F_ZERO, (%rdi) # unset zero
	orq %rcx, (%rdi) # copy new carry bit
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * srl reg8
 *
 */
op_shf_rgh_lg:
	xorq %rcx, %rcx # zero
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movb (%r10), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	shrb (%r10) # rotate register with carry
	pushf # save flags
	popq %rcx # pop flags
	shlq $1, %rcx # shift bit on to carry-flag position
	andq $F_ZERO, %rcx
	andq $~F_ZERO, (%rdi) # unset zero
	orq %rcx, (%rdi) # copy new carry bit
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * sla reg8
 *
 */
op_shf_lf_arth:
	xorq %rcx, %rcx # zero
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movb (%r10), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	salb (%r10) # rotate register with carry
	pushf # save flags
	popq %rcx # pop flags
	shlq $1, %rcx # shift bit on to carry-flag position
	andq $F_ZERO, %rcx
	andq $~F_ZERO, (%rdi) # unset zero
	orq %rcx, (%rdi) # copy new carry bit
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * rr reg8
 *
 */
op_rot_rgh_reg:
	pushq %rbp
	movq %rsp, %rbp
	xorq %rcx, %rcx # zero
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orq %rbx, -8(%rbp) # copy z80 carry flag into x86 carry flag
	movb (%r10), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	popf # pop new x86 flags incquding z80 carry flag
	rcrb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, (%r10)
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	popq %rbp
	ret

/* 
 * rl reg8
 *
 */
op_rot_lf_reg:
	pushq %rbp
	movq %rsp, %rbp
	xorq %rcx, %rcx # zero
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orq %rbx, -8(%rbp) # copy z80 carry flag into x86 carry flag
	movb (%r10), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	popf # pop new x86 flags incquding z80 carry flag
	rclb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, (%r10)
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	popq %rbp
	incq %r13
	ret

/* 
 * rrc reg8
 *
 */
op_rot_rgh_c_reg:
	movq %rsp, %rbp
	xorq %rcx, %rcx # zero
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movb (%r10), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	rorb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, (%r10)
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	ret

/* 
 * rlc reg8
 *
 */
op_rot_lf_c_reg:
	movq %rsp, %rbp
	xorq %rcx, %rcx # zero
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movb 2(%rdx), %cl
	addq %rcx, %r10 # point to register
	movb (%r10), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	rolb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, (%r10)
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	ret

/* 
 * rlca
 *
 */
op_rot_lf_c_acc:
	movq %rsp, %rbp
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	incq %r10 # point to register
	movb (%r10), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	rolb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * rla
 *
 */
op_rot_lf_acc:
	pushq %rbp
	movq %rsp, %rbp
	xorq %rcx, %rcx # zero
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	incq %r10 # point to register
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orq %rbx, -8(%rbp) # copy z80 carry flag into x86 carry flag
	movb (%r10), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	popf # pop new x86 flags incquding z80 carry flag
	rclb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	popq %rbp
	ret

/* 
 * rl (HL)
 *
 */
op_rot_lf_mem:
	pushq %rbp
	movq %rsp, %rbp
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orq %rbx, -8(%rbp) # copy z80 carry flag into x86 carry flag
	movb (%rsi), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx
	popf
	rclb %dl # rotate register with carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, %dl
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	popq %rbp
	ret

/* 
 * rlc (HL)
 *
 */
op_rot_lf_c_mem:
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	movb (%rsi), %al # copy byte
	shrb $3, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx
	rolb %dl # rotate register with carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, %dl
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	ret

/* 
 * rrc (HL)
 *
 */
op_rot_rgh_c_mem:
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	movb (%rsi), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx # read value
	rorb %dl # rotate register with carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, %dl
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	ret

/* 
 * rr (HL)
 *
 */
op_rot_rgh_mem:
	pushq %rbp
	movq %rsp, %rbp
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi
	addq (%rdi), %rsi # get offset
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orq %rbx, -8(%rbp) # copy z80 carry flag into x86 carry flag
	movb (%rsi), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	movq (%rsi), %rdx
	popf # pop new x86 flags incquding z80 carry flag
	rcrb %dl # rotate register with carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	cmpb $0, %dl
	jne 1f
	orq $F_ZERO, (%rdi)
1:
	incq %r13
	popq %rbp
	ret

/* 
 * rra
 *
 */
op_rot_rgh_acc:
	pushq %rbp
	movq %rsp, %rbp
	xorq %rcx, %rcx # zero
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	incq %r10 # point to register
	movq (%rdi), %rbx # copy flags
	andq $F_CARRY, %rbx # mask carry bit
	shrq $4, %rbx # move to carry position in x86 flags
	pushf # push x86 flags
	andq $~1, -8(%rbp) # zero carry flag
	orq %rbx, -8(%rbp) # copy z80 carry flag into x86 carry flag
	movb (%r10), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	popf # pop new x86 flags incquding z80 carry flag
	rcrb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	popq %rbp
	ret

/* 
 * rrca
 *
 */
op_rot_rgh_c_acc:
	movq %rsp, %rbp
	incq %rax # advance PC
	movq $regs_sets, %r10 # pointer to registers
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	incq %r10 # point to register
	movb (%r10), %al # copy byte
	shlb $4, %al # move bit 0 to carry position
	andb $F_CARRY, %al # mask in carry bit
	andq $~F_CARRY, (%rdi) # zero carry bit
	orb %al, (%rdi) # copy new carry
	rorb (%r10) # rotate register through carry
	andq $~F_ZERO, (%rdi) # unset zero
	andq $~F_SUBTRACT, (%rdi) # unset subtract
	andq $~F_HCARRY, (%rdi) # unset hcarry
	incq %r13
	ret

/* 
 * swap r8
 *
 */
op_swp:
	xorq %rcx, %rcx # zero
	movq $regs_sets, %r10 # pointer to registers
	movb 2(%rdx), %cl # copy register offset
	addq %rcx, %r10 # point to register
	movb (%r10), %cl # copy register
	movb %cl, %ch
	shrw $4, %cx
	andq $~0xff, (%r10) # zero register
	orb %cl, (%r10) # copy register
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit to position
	andq $F_ZERO, %rcx # mask in zero bit
	movq $regs_sets, %rdi # pointer to flags
	incq PC(%rdi)
	andq $~F_ZERO, (%rdi) # zero flag bit
	orq %rcx, (%rdi) # copy new zero bit
	andq $~F_CARRY, (%rdi) # zero flag bit
	andq $~F_HCARRY, (%rdi) # zero flag bit
	andq $~F_SUBTRACT, (%rdi) # zero flag bit
	incq %r13
	ret

/* 
 * swap (HL)
 *
 */
op_swp_mem:
	movq $regs_sets, %rdi
	incq PC(%rdi)
	xorq %rcx, %rcx # zero
	movq regs_sets+HL, %rsi # HL
	movq $addr_sp_ptrs, %rdx
	movq %rsi, %r10 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdx # point to address space
	movq %r10, %rsi
	addq (%rdx), %rsi # new PC
	movq (%rsi), %rcx # copy word
	andq $0xff, %rcx # mask in byte
	movb %cl, %ch
	shrw $4, %cx
	xorq %rdx, %rdx # zero register
	orb %cl, %dl # copy register
	pushf # save flags
	popq %rcx # restore flags
	shlq $1, %rcx # move zero bit to position
	andq $F_ZERO, %rcx # mask in zero bit
	andq $~F_ZERO, (%rdi) # zero flag bit
	orq %rcx, (%rdi) # copy new zero bit
	andq $~F_CARRY, (%rdi) # zero flag bit
	andq $~F_HCARRY, (%rdi) # zero flag bit
	andq $~F_SUBTRACT, (%rdi) # zero flag bit
	incq %r13
	ret

/* 
 * Group 4: Single-bit operations
 */
/*
 * set BIT_NUM, (HL)
 *
 */
op_set_mem:
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq HL(%r10), %rsi
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi # restore
	addq (%rdi), %rsi # new PC
	xorq %rcx, %rcx # zero
	movb 3(%rdx), %ch # extract bit to mask
	movq (%rsi), %rdx
	orb %ch, %dl # copy new register
	incq %r13
	ret

/*
 * set BIT_NUM, REG
 *
 */
op_set:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	xorq %rcx, %rcx # zero
	movb 2(%rdx), %cl # copy register offset
	addq %rcx, %rdi # point to register
	movb 3(%rdx), %ch # extract bit to mask
	orb %ch, (%rdi) # copy new register
	incq %r13
	ret

/*
 * rst BIT_NUM, REG
 *
 */
op_rst:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	xorq %rcx, %rcx # zero
	movb 2(%rdx), %cl # copy register offset
	addq %rcx, %rdi # point to register
	movb 3(%rdx), %ch # extract bit to mask
	notb %ch # mask out bit
	andb %ch, (%rdi) # copy new register
	incq %r13
	ret

/*
 * rst BIT_NUM, (HL)
 *
 */
op_rst_mem:
	incq %r13
	movq $regs_sets, %r10 # pointer to registers
	incq PC(%r10)
	movq HL(%r10), %rsi
	movq $addr_sp_ptrs, %rdi
	movq %rsi, %r11 # save
	shrq $9, %rsi # get
	andq $0x78, %rsi #  offset to pointer in addr_sp_ptrs
	addq %rsi, %rdi # point to address space
	movq %r11, %rsi # restore
	cmpq $0x8000, %rsi
	jb 1f
	addq (%rdi), %rsi # new PC
	xorq %rcx, %rcx # zero
	movb 3(%rdx), %ch # extract bit to mask
	notb %ch # mask out bit
	movq (%rsi), %rdx
	andb %ch, %dl # copy new register
	ret
# ugly hack XXX
1:
	addq (%rdi), %rsi # new PC
	movq (%rsi), %rdx
	ret

/*
 * bit BIT_NUM, REG
 *
 */
op_bit_mem:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq HL(%rdi), %rbx # get HL
	movq $addr_sp_ptrs, %rcx
	movq %rbx, %r11 # save
	shrq $9, %rbx # get
	andq $0x78, %rbx #  offset to pointer in addr_sp_ptrs
	addq %rbx, %rcx # point to address space
	movq  %r11, %rbx # restore
	addq (%rcx), %rbx # new PC
	movb 3(%rdx), %ch # extract byte to mask
	testb %ch, (%rbx) # test bit
	pushf # push flags
	popq %rbx # pop flags
	shlq $1, %rbx # copy zero flag to position
	andq $F_ZERO, %rbx # mask in zero flag
	andq $~F_ZERO, (%rdi) # clear zero flag
	orq %rbx, (%rdi) # copy zero flag
	andq $~F_SUBTRACT, (%rdi) # unset flag
	orq $F_HCARRY, (%rdi) # set flag
	incq %r13
	ret

/*
 * bit BIT_NUM, REG
 *
 */
op_bit:
	movq $regs_sets, %rbp # pointer to registers
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	xorq %rcx, %rcx # zero
	movb 2(%rdx), %cl # copy register offset
	addq %rcx, %rbp # point to register
	movb 3(%rdx), %ch # extract byte to mask
	testb %ch, (%rbp) # test bit
	pushf # push flags
	popq %rdx # pop flags
	shlq $1, %rdx # copy zero flag to position
	andq $F_ZERO, %rdx # mask in zero flag
	andq $~F_ZERO, (%rdi) # clear zero flag
	orq %rdx, (%rdi) # copy zero flag
	andq $~F_SUBTRACT, (%rdi) # unset flag
	orq $F_HCARRY, (%rdi) # set flag
	incq %r13
	ret

op_daa:
	incq %r13 # new PC
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq A(%rdi), %rax
	movb %al, %ah # %ah=old A
	movq %rax, %rcx
	andq $0xff, %rcx
	testq $F_SUBTRACT, (%rdi)
	jnz 8f
	testq $F_HCARRY, (%rdi)
	jnz 1f
	andb $0xf, %ah
	cmpb $9, %ah
	ja 1f
	jmp 2f
1:
	addq $6, %rcx
2:
	testq $F_CARRY, (%rdi)
	jnz 3f
	cmpq $0x9f, %rcx
	ja 3f
	jmp 4f
3:
	addq $0x60, %rcx
4:
	jmp end_daa

8:
	testq $F_HCARRY, (%rdi)
	jz 9f
	subq $6, %rcx
	andq $0xff, %rcx
9:
	testq $F_CARRY, (%rdi)
	jz 10f
	subq $0x60, %rcx
10:
end_daa:
	andq $~F_HCARRY, (%rdi)
	movb %cl, A(%rdi)
	andq $0x100, %rcx
	cmpq $0x100, %rcx
	jne 11f
	orq $F_CARRY, (%rdi)
11:
	testb $0xff, A(%rdi)
	jnz 12f
	orq $F_ZERO, (%rdi)
	jmp 13f
12:
	andq $~F_ZERO, (%rdi)
13:
	ret

/* 
 * Group 5: CPU Control
 */
/*
 * nop
 *
 */
op_nop:
	incq %r13 # new PC
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	ret

/*
 * stop
 *
 */
op_stop:
	incq %r13 # new PC
	testq $1, (SPD_REG+addr_sp) # change speed?
	jz 2f
	notb (SPD_REG+addr_sp)
	andb $0x80, (SPD_REG+addr_sp)
	notq cpu_cur_mode
	andq $1, cpu_cur_mode
2:
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	ret

/* 
 * Group 6: Jump Instructions
 */
/*
 * callz
 *
 */
op_call_z:
	addq $3, %r13 # PC when return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	incq PC(%rdi)
	incq PC(%rdi)
	movq (%rdi), %rcx # copy flags
	andq $F_ZERO, %rcx # mask in zero
	jz 1f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq -2(%r13), %r13 # copy immediate value
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
1:
	ret

/*
 * callnz
 *
 */
op_call_nz:
	addq $3, %r13 # PC when return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	incq PC(%rdi)
	incq PC(%rdi)
	movq (%rdi), %rcx # copy flags
	andq $F_ZERO, %rcx # mask in zero
	jnz 1f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq -2(%r13), %r13 # copy immediate value
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
1:
	ret

/*
 * callnc
 *
 */
op_call_nc:
	addq $3, %r13 # PC when return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	incq PC(%rdi)
	incq PC(%rdi)
	movq (%rdi), %rcx # copy flags
	andq $F_CARRY, %rcx # mask in carry
	jnz 1f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq -2(%r13), %r13 # copy immediate value
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
1:
	ret

/*
 * callc
 *
 */
op_call_c:
	addq $3, %r13 # PC when return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	incq PC(%rdi)
	incq PC(%rdi)
	movq (%rdi), %rcx # copy flags
	andq $F_CARRY, %rcx # mask in carry
	jz 1f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq -2(%r13), %r13 # copy immediate value
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
1:
	ret

/*
 * call
 *
 */
op_call:
	addq $3, %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $3, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq -2(%r13), %r13 # copy immediate value
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_0:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $0, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_8h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $8, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_10h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $16, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * ret
 *
 */
op_reti:
	movq $1, ime_flag # enable interrupts
op_ret:
	movq $regs_sets, %r10 # pointer to registers
	movq SP(%r10), %r13
	addq $2, SP(%r10) # SP+=4
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
	movq (%r13), %r13
	andq $0xffff, %r13
	movq %r13, PC(%r10) # new PC
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_18h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $24, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_20h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $32, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_28h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $40, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_30h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $48, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * call
 *
 * finished: yes
 * tested: no
 */
op_reset_38h:
	incq %r13 # PC when return
	movq $regs_sets, %r10 # pointer to registers
	subq $2, SP(%r10) # SP-=4 XXX SP-=2 for z80
	movq PC(%r10), %r12
	addq $1, %r12 # new PC
	movq SP(%r10), %rbp
	movq $addr_sp_ptrs, %rdi
	movq %rbp, %rax # save
	shrq $9, %rbp # get
	andq $0x78, %rbp #  offset to pointer in addr_sp_ptrs
	addq %rbp, %rdi # point to address space
	movq %rax, %rbp # restore
	addq (%rdi), %rbp # new PC
	movw %r12w, (%rbp)
	movq $56, %r13 # copy immediate value
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * ret z
 *
 */
op_ret_z:
	incq %r13 # new PC if no return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq (%rdi), %rdx # copy flags
	andq $F_ZERO, %rdx # mask in carry
	jz 2f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	movq SP(%r10), %r13
	addq $2, SP(%r10) # SP+=4
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
	movq (%r13), %r13
	andq $0xffff, %r13
	movq %r13, PC(%r10) # new PC
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
2:
	ret

/*
 * ret nz
 *
 */
op_ret_nz:
	incq %r13 # new PC if no return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq (%rdi), %rdx # copy flags
	andq $F_ZERO, %rdx # mask in carry
	jnz 2f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	movq SP(%r10), %r13
	addq $2, SP(%r10) # SP+=4
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
	movq (%r13), %r13
	andq $0xffff, %r13
	movq %r13, PC(%r10) # new PC
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
2:
	ret

/*
 * ret nc
 *
 */
op_ret_nc:
	incq %r13 # new PC if no return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq (%rdi), %rdx # copy flags
	andq $F_CARRY, %rdx # mask in carry
	jnz 2f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	movq SP(%r10), %r13
	addq $2, SP(%r10) # SP+=4
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
	movq (%r13), %r13
	andq $0xffff, %r13
	movq %r13, PC(%r10) # new PC
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
2:
	ret

/*
 * ret c
 *
 */
op_ret_c:
	incq %r13 # new PC if no return
	movq $regs_sets, %rdi # pointer to registers
	incq PC(%rdi)
	movq (%rdi), %rdx # copy flags
	andq $F_CARRY, %rdx # mask in carry
	jz 2f
	addq $12, %r14
	movq $regs_sets, %r10 # pointer to registers
	movq SP(%r10), %r13
	addq $2, SP(%r10) # SP+=4
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
	movq (%r13), %r13
	andq $0xffff, %r13
	movq %r13, PC(%r10) # new PC
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13
2:
	ret

/*
 * jmp a16
 *
 */
op_jmp:
	movq 1(%r13), %r13
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movq $regs_sets, %r10
	movw %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/*
 * jnz a16
 *
 */
op_jmp_nz:
	addq $3, %r13 # new PC
	movq $regs_sets, %rbx # copy flags
	incq PC(%rbx)
	incq PC(%rbx)
	incq PC(%rbx)
	movq regs_sets, %rbx # copy flags
	andq $F_ZERO, %rbx # mask in zero bit
	jnz 2f
	movq $regs_sets, %rbx # copy flags
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	movq -2(%r13), %r13 # copy signed byte
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movw %r13, PC(%rbx)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
2:
	ret

/*
 * jz a16
 *
 */
op_jmp_z:
	addq $3, %r13 # new PC
	movq regs_sets, %rbx # copy flags
	movq $regs_sets, %r10 # copy flags
	incq PC(%r10)
	incq PC(%r10)
	incq PC(%r10)
	andq $F_ZERO, %rbx # mask in zero bit
	jz 2f
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	movq -2(%r13), %r13 # copy signed byte
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movw %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
2:
	ret

/*
 * jc a16
 *
 */
op_jmp_c:
	addq $3, %r13 # new PC
	movq $regs_sets, %rbx # copy flags
	incq PC(%rbx)
	incq PC(%rbx)
	incq PC(%rbx)
	movq regs_sets, %rbx # copy flags
	andq $F_CARRY, %rbx # mask in zero bit
	jz 2f
	movq $regs_sets, %rbx # copy flags
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	movq -2(%r13), %r13 # copy signed byte
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movw %r13, PC(%rbx)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
2:
	ret

/*
 * jnc a16
 *
 */
op_jmp_nc:
	addq $3, %r13 # new PC
	movq $regs_sets, %rbx # copy flags
	incq PC(%rbx)
	incq PC(%rbx)
	incq PC(%rbx)
	movq regs_sets, %rbx # copy flags
	andq $F_CARRY, %rbx # mask in zero bit
	movq $regs_sets, %rbx # copy flags
	jnz 2f
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	movq -2(%r13), %r13 # copy signed byte
	andq $0xffff, %r13 # mask in word
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	movw %r13, PC(%rbx)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret
2:
	ret

/* 
 * jr r8
 *
 */
op_jmp_rl:
	addq $2, %r13 # new PC
	movq $regs_sets, %r10
	incq PC(%r10)
	incq PC(%r10)
	xorq %rcx, %rcx # zero
	movsbw -1(%r13), %cx # copy signed byte
	addw %cx, regs_sets+PC
	movq regs_sets+PC, %r13
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/* 
 * jrnz r8
 *
 */
op_jmp_rl_nz:
	addq $2, %r13 # new PC
	movq $regs_sets, %r10
	addq $2, PC(%r10)
	movq regs_sets, %rbx # copy flags
	andq $F_ZERO, %rbx # mask in zero bit
	jnz 2f
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	xorq %rcx, %rcx # zero
	movsbw -1(%r13), %cx # copy signed byte
	addw %cx, regs_sets+PC
	movq regs_sets+PC, %r13
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
2:
	ret

/* 
 * jrz r8
 *
 */
op_jmp_rl_z:
	addq $2, %r13 # new PC
	movq $regs_sets, %r10
	incq PC(%r10)
	incq PC(%r10)
	movq regs_sets, %rbx # copy flags
	andq $F_ZERO, %rbx # mask in zero bit
	jz 2f
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	xorq %rcx, %rcx # zero
	movsbw -1(%r13), %cx # copy signed byte
	addw %cx, regs_sets+PC
	movq regs_sets+PC, %r13
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
2:
	ret

/* 
 * jrnc r8
 *
 */
op_jmp_rl_nc:
	addq $2, %r13 # new PC
	movq $regs_sets, %r10
	incq PC(%r10)
	incq PC(%r10)
	movq regs_sets, %rbx # copy flags
	andq $F_CARRY, %rbx # mask in zero bit
	jnz 2f
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	xorq %rcx, %rcx # zero
	movsbw -1(%r13), %cx # copy signed byte
	addw %cx, regs_sets+PC
	movq regs_sets+PC, %r13
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
2:
	ret

/* 
 * jrc r8
 *
 */
op_jmp_rl_c:
	addq $2, %r13 # new PC
	movq $regs_sets, %r10
	incq PC(%r10)
	incq PC(%r10)
	movq regs_sets, %rbx # copy flags
	andq $F_CARRY, %rbx # mask in zero bit
	jz 2f
	movq %rsp, %rbp
	addq $4, %r14 # add extra ticks
	xorq %rcx, %rcx # zero
	movsbw -1(%r13), %cx # copy signed byte
	addw %cx, regs_sets+PC
	movq regs_sets+PC, %r13
	movq $addr_sp_ptrs, %rdi
	movq %r13, %r11 # save
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
2:
	ret

/* 
 * jmp (HL)
 *
 */
op_jmp_ind:
	movq regs_sets+HL, %r13 # HL register
	movq $addr_sp_ptrs, %rdi # pointers
	movq %r13, %r11 # save
	movq $regs_sets, %r10
	movq %r13, PC(%r10)
	shrq $9, %r13 # get
	andq $0x78, %r13 #  offset to pointer in addr_sp_ptrs
	addq %r13, %rdi # point to address space
	movq %r11, %r13 # restore
	addq (%rdi), %r13 # new PC
	ret

/* 
 * di (disable interrupts)
 *
 */
op_dis_ints:
	movq $0, ime_flag
	movq $regs_sets, %r10
	incq PC(%r10)
	incq %r13 # new PC
	ret

/* 
 * ei (enable interrupts)
 *
 */
op_ena_ints:
	movq $1, just_enabled
	movq $1, ime_flag
	movq $regs_sets, %r10
	incq PC(%r10)
	incq %r13 # new PC
	ret

/* 
 * clear F_CARRY
 */
op_clr_car_flg:
	incq %r13 # new PC
	incq regs_sets+PC # set PC
	testq $F_CARRY, regs_sets # clear carry flag
	jnz 1f
	orq $F_CARRY, regs_sets # set carry flag
	jmp 2f
1:
	andw $~F_CARRY, regs_sets
2:
	andw $~F_SUBTRACT, regs_sets
	andw $~F_HCARRY, regs_sets
	ret

/* 
 * set F_CARRY
 */
op_set_car_flg:
	incq %r13 # new PC
	incq regs_sets+PC # set PC
	orw $F_CARRY, regs_sets # set carry flag
	andw $~F_SUBTRACT, regs_sets
	andw $~F_HCARRY, regs_sets
	ret

/* 
 * flip accumulator
 */
op_cpl:
	incq %r13 # new PC
	incq regs_sets+PC # set PC
	notb regs_sets+A # flip accumulator
	orw $F_SUBTRACT, regs_sets+F # set sign flag
	orw $F_HCARRY, regs_sets+F # set half carry flag
	ret


/*
 * Halt CPU
 */
op_halt:
	movq $1, cpu_halt
	ret

/*
 * INVALID INSTRUCTION!
 */
op_inval:
	jmp .
	ret

/*
 * Opcode for escaping to 256 more opcodes.
 *
 */
op_escape:
	incq %r13 # next opcode
	movq $regs_sets, %r10
	incq PC(%r10)
	movb (%r13), %al # extract opcode
	andq $0xff, %rax # mask in opcode
	shlq $5, %rax
	movq $z80_cb, %rdx # get record for escaped opcode
	addq %rax, %rdx # pointer to record
	movq (%rdx), %rax # save cycle duration of instruction
	movq %rax, %r12
	shrq $56, %rax # cycle duration is the most significant byte
	shrq $40, %r12 # get byte that tells if instruction is a memory access
	andq $0xff, %r12
	movq %rax, %r14
	#addq %rax, %r14
	movl 28(%rdx), %ebx # pointer to function
	#call *%rbx
	movq $1, inst_is_cb
	ret

/* 
 * Instruction format: 
 * opcode, dest addr, dest, src addr, src, unit
 */
.p2align 3
.data
z80_ldex:
.byte 0x00, NULL, NULL, NULL, NULL, NULL, 1, 4 # nop
.ascii "nop\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_nop

.byte 0x01, REG, BC, IMM, NULL, NULL, 3, 12 # ld BC, nn
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_ld_imm_reg

.byte 0x02, REG_IND, BC, REG, A, DELAY|RD_XOR_WR, 1, 8 # ld (BC), A
.long BYTE
.ascii "ld (BC), A\0\0\0\0\0\0"
.long op_ld_reg_mem

.byte 0x03, REG, BC, NULL, NULL, NULL, 1, 8 # inc BC
.ascii "inc BC\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_inc_reg_wr

.byte 0x04, REG, B, NULL, NULL, NULL, 1, 4 # inc B
.ascii "inc B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x05, REG, B, NULL, NULL, NULL, 1, 4 # dec B
.ascii "dec B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_dec_reg

.byte 0x06, REG, B, IMM, NULL, NULL, 2, 8 # ld B, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x07, REG, A, NULL, NULL, NULL, 1, 4 # rlca
.ascii "rlca\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_acc

.byte 0x08, IMM_IND, NULL, REG, SP, DELAY|RD_XOR_WR, 3, 20 # ld (nn), SP
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_ld_reg_imm

.byte 0x09, REG, HL, REG, BC, NULL, 1, 8 # add HL, BC
.ascii "add HL, BC\0\0\0\0\0\0"
.long WORD
.long op_add_wr

.byte 0x0a, REG, A, REG_IND, BC, DELAY|RD_XOR_WR, 1, 8 # ld A, (BC)
.ascii "ld A, (BC)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x0b, REG, BC, NULL, NULL, NULL, 1, 8 # dec BC
.ascii "dec BC\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_dec_reg_wr

.byte 0x0c, REG, C, NULL, NULL, NULL, 1, 4 # inc C
.ascii "inc C\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x0d, REG, C, NULL, NULL, NULL, 1, 4 # dec C
.ascii "dec C\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_dec_reg

.byte 0x0e, REG, C, IMM, NULL, NULL, 2, 8 # ld C, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x0f, REG, A, NULL, NULL, NULL, 1, 4 # rrca
.ascii "rrca\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_acc

.byte 0x10, NULL, NULL, NULL, NULL, NULL, 1, 4 # stop
.ascii "stop\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_stop

.byte 0x11, REG, DE, IMM, NULL, NULL, 3, 12 # ld DE, nn
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_ld_imm_reg

.byte 0x12, REG_IND, DE, REG, A, DELAY|RD_XOR_WR, 1, 8 # ld (DE), A
.ascii "ld (DE), A\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x13, REG, DE, NULL, NULL, NULL, 1, 8 # inc DE
.ascii "inc DE\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_inc_reg_wr

.byte 0x14, REG, D, NULL, NULL, NULL, 1, 4 # inc D
.ascii "inc D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x15, REG, D, NULL, NULL, NULL, 1, 4 # dec D
.ascii "inc D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_dec_reg

.byte 0x16, REG, D, IMM, NULL, NULL, 2, 8 # ld D, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x17, REG, A, NULL, NULL, NULL, 1, 4 # rla
.ascii "rla\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_acc

.byte 0x18, NULL, NULL, NULL, NULL, NULL, 2, 12 # jr r8
.ascii "jr\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_rl

.byte 0x19, REG, HL, REG, DE, NULL, 1, 8 # add HL, DE
.ascii "add HL, DE\0\0\0\0\0\0"
.long WORD
.long op_add_wr

.byte 0x1a, REG, A, REG_IND, DE, DELAY|RD_XOR_WR, 1, 8 # ld A, (DE)
.ascii "ld A, (DE)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x1b, REG, DE, NULL, NULL, NULL, 1, 8 # dec DE
.ascii "dec DE\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_dec_reg_wr

.byte 0x1c, REG, E, NULL, NULL, NULL, 1, 4 # inc E
.ascii "inc E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x1d, REG, E, NULL, NULL, NULL, 1, 4 # dec E
.ascii "dec E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_dec_reg

.byte 0x1e, REG, E, IMM, NULL, NULL, 2, 8 # ld E, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x1f, REG, A, NULL, NULL, NULL, 1, 4 # rra
.ascii "rra\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_acc

.byte 0x20, NULL, F_ZERO, NULL, NULL, NULL, 2, 8 # jrnz r8
.ascii "jrnz\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_rl_nz

.byte 0x21, REG, HL, IMM, NULL, NULL, 3, 12 # ld HL, nn
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_ld_imm_reg

.byte 0x22, REG_IND, HL, REG, A, DELAY|RD_XOR_WR, 1, 8 # ldi (HL), A
.ascii "ldi (HL), A\0\0\0\0\0"
.long WORD
.long op_ldi_reg_mem

.byte 0x23, REG, HL, NULL, NULL, NULL, 1, 8 # inc HL
.ascii "inc HL\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_inc_reg_wr

.byte 0x24, REG, H, NULL, NULL, NULL, 1, 4 # inc H
.ascii "inc H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x25, REG, H, NULL, NULL, NULL, 1, 4 # dec H
.ascii "dec H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_dec_reg

.byte 0x26, REG, H, IMM, NULL, NULL, 2, 8 # ld h, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x27, NULL, NULL, NULL, NULL, NULL, 1, 4 # daa
.ascii "daa\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_daa

.byte 0x28, NULL, F_ZERO, NULL, NULL, NULL, 2, 8 # jrz r8
.ascii "jrz\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_rl_z

.byte 0x29, REG, HL, REG, HL, NULL, 1, 8 # add HL, HL
.ascii "add HL, HL\0\0\0\0\0\0"
.long WORD
.long op_add_wr

.byte 0x2a, REG, HL, REG, A, DELAY|RD_XOR_WR, 1, 8 # ldi A, (HL)
.ascii "ldi A, (HL)\0\0\0\0\0"
.long NULL
.long op_ldi_mem_reg

.byte 0x2b, REG, HL, NULL, NULL, NULL, 1, 8 # dec HL
.ascii "dec HL\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_dec_reg_wr

.byte 0x2c, REG, L, NULL, NULL, NULL, 1, 4 # inc L
.ascii "inc L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x2d, REG, L, NULL, NULL, NULL, 1, 4 # dec L
.ascii "dec L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_dec_reg

.byte 0x2e, REG, L, IMM, NULL, NULL, 2, 8 # ld L, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x2f, NULL, NULL, NULL, NULL, NULL, 1, 4 # cpl
.ascii "cpl\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_cpl

.byte 0x30, NULL, F_CARRY, NULL, NULL, NULL, 2, 8 # jrnc r8
.ascii "jrnc\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_rl_nc

.byte 0x31, REG, SP, IMM, NULL, NULL, 3, 12 # ld SP, nn
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_ld_imm_sp

.byte 0x32, REG_IND, HL, REG, A, DELAY|RD_XOR_WR, 1, 8 # ldd (HL), A
.ascii "ldd (HL), A\0\0\0\0\0"
.long BYTE
.long op_ldd_reg_mem

.byte 0x33, REG, SP, NULL, NULL, NULL, 1, 8 # inc SP
.ascii "inc SP\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg_sp

.byte 0x34, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 12 # inc (HL)
.ascii "inc (HL)\0\0\0\0\0\0\0\0"
.long WORD
.long op_inc_mem

.byte 0x35, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 12 # dec (HL)
.ascii "dec (HL)\0\0\0\0\0\0\0\0"
.long WORD
.long op_dec_mem

.byte 0x36, REG_IND, HL, IMM, NULL, DELAY|RD_XOR_WR, 2, 12 # ld (HL), n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_mem

.byte 0x37, NULL, NULL, NULL, NULL, NULL, 1, 4 # scf
.ascii "scf\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_set_car_flg

.byte 0x38, NULL, F_CARRY, NULL, NULL, NULL, 2, 8 # jrc r8
.ascii "jrc\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_rl_c

.byte 0x39, REG, HL, REG, SP, NULL, 1, 8 # add HL, SP
.ascii "add HL, SP\0\0\0\0\0\0"
.long WORD
.long op_add_wr

.byte 0x3a, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, NULL, 8 # ldd A, (HL)
.ascii "ldd A, (HL)\0\0\0\0\0"
.long BYTE
.long op_ldd_mem_reg

.byte 0x3b, REG, SP, NULL, NULL, NULL, 1, 8 # dec SP
.ascii "dec SP\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_dec_reg_sp

.byte 0x3c, REG, A, NULL, NULL, NULL, 1, 4 # inc A
.ascii "inc A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_inc_reg

.byte 0x3d, REG, A, NULL, NULL, NULL, 1, 4 # dec A
.ascii "dec A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_dec_reg

.byte 0x3e, REG, A, IMM, NULL, NULL, 2, 8 # ld A, n
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_imm_reg

.byte 0x3f, NULL, NULL, NULL, NULL, NULL, 1, 4 # ccf
.ascii "ccf\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_clr_car_flg

.byte 0x40, REG, B, REG, B, NULL, 1, 4 # ld B, B
.ascii "ld B, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x41, REG, B, REG, C, NULL, 1, 4 # ld B, C
.ascii "ld B, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x42, REG, B, REG, D, NULL, 1, 4 # ld B, D
.ascii "ld B, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x43, REG, B, REG, E, NULL, 1, 4 # ld B, E
.ascii "ld B, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x44, REG, B, REG, H, NULL, 1, 4 # ld B, H
.ascii "ld B, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x45, REG, B, REG, L, NULL, 1, 4 # ld B, L
.ascii "ld B, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x46, REG, B, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld B, (HL)
.ascii "ld B, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x47, REG, B, REG, A, NULL, 1, 4 # ld B, A
.ascii "ld B, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x48, REG, C, REG, B, NULL, 1, 4 # ld C, B
.ascii "ld C, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x49, REG, C, REG, C, NULL, 1, 4 # ld C, C
.ascii "ld C, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x4a, REG, C, REG, D, NULL, 1, 4 # ld C, D
.ascii "ld C, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x4b, REG, C, REG, E, NULL, 1, 4 # ld C, E
.ascii "ld C, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x4c, REG, C, REG, H, NULL, 1, 4 # ld C, H
.ascii "ld C, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x4d, REG, C, REG, L, NULL, 1, 4 # ld C, L
.ascii "ld C, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x4e, REG, C, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld C, (HL)
.ascii "ld C, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x4f, REG, C, REG, A, NULL, 1, 4 # ld C, A
.ascii "ld C, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x50, REG, D, REG, B, NULL, 1, 4 # ld D, B
.ascii "ld D, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x51, REG, D, REG, C, NULL, 1, 4 # ld D, C
.ascii "ld D, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x52, REG, D, REG, D, NULL, 1, 4 # ld D, D
.ascii "ld D, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x53, REG, D, REG, E, NULL, 1, 4 # ld D, E
.ascii "ld D, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x54, REG, D, REG, H, NULL, 1, 4 # ld D, H
.ascii "ld D, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x55, REG, D, REG, L, NULL, 1, 4 # ld D, L
.ascii "ld D, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x56, REG, D, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld D, (HL)
.ascii "ld D, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x57, REG, D, REG, A, NULL, 1, 4 # ld D, A
.ascii "ld D, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x58, REG, E, REG, B, NULL, 1, 4 # ld E, B
.ascii "ld E, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x59, REG, E, REG, C, NULL, 1, 4 # ld E, C
.ascii "ld E, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x5a, REG, E, REG, D, NULL, 1, 4 # ld E, D
.ascii "ld E, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x5b, REG, E, REG, E, NULL, 1, 4 # ld E, E
.ascii "ld E, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x5c, REG, E, REG, H, NULL, 1, 4 # ld E, H
.ascii "ld E, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x5d, REG, E, REG, L, NULL, 1, 4 # ld E, L
.ascii "ld E, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x5e, REG, E, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld E, (HL)
.ascii "ld E, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x5f, REG, E, REG, A, NULL, 1, 4 # ld E, A
.ascii "ld E, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x60, REG, H, REG, B, NULL, 1, 4 # ld H, B
.ascii "ld H, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x61, REG, H, REG, C, NULL, 1, 4 # ld H, C
.ascii "ld H, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x62, REG, H, REG, D, NULL, 1, 4 # ld H, D
.ascii "ld H, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x63, REG, H, REG, E, NULL, 1, 4 # ld H, E
.ascii "ld H, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x64, REG, H, REG, H, NULL, 1, 4 # ld H, H
.ascii "ld H, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x65, REG, H, REG, L, NULL, 1, 4 # ld H, L
.ascii "ld H, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x66, REG, H, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld H, (HL)
.ascii "ld H, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x67, REG, H, REG, A, NULL, 1, 4 # ld H, A
.ascii "ld H, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x68, REG, L, REG, B, NULL, 1, 4 # ld L, B
.ascii "ld L, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x69, REG, L, REG, C, NULL, 1, 4 # ld L, C
.ascii "ld L, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x6a, REG, L, REG, D, NULL, 1, 4 # ld L, D
.ascii "ld L, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x6b, REG, L, REG, E, NULL, 1, 4 # ld L, E
.ascii "ld L, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x6c, REG, L, REG, H, NULL, 1, 4 # ld L, H
.ascii "ld L, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x6d, REG, L, REG, L, NULL, 1, 4 # ld L, L
.ascii "ld L, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x6e, REG, L, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld L, (HL)
.ascii "ld L, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x6f, REG, L, REG, A, NULL, 1, 4 # ld L, A
.ascii "ld L, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x70, REG_IND, HL, REG, B, DELAY|RD_XOR_WR, 1, 8
.ascii "ld (HL), B\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x71, REG_IND, HL, REG, C, DELAY|RD_XOR_WR, 1, 8
.ascii "ld (HL), C\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x72, REG_IND, HL, REG, D, DELAY|RD_XOR_WR, 1, 8
.ascii "ld (HL), D\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x73, REG_IND, HL, REG, E, DELAY|RD_XOR_WR, 1, 8
.ascii "ld (HL), E\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x74, REG_IND, HL, REG, H, DELAY|RD_XOR_WR, 1, 8
.ascii "ld (HL), H\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x75, REG_IND, HL, REG, L, DELAY|RD_XOR_WR, 1, 8
.ascii "ld (HL), L\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x76, NULL, NULL, NULL, NULL, NULL, 1, 8 # HALT!
.ascii "halt\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_halt

.byte 0x77, REG_IND, HL, REG, A, DELAY|RD_XOR_WR, 1, 8 # ld (HL), A
.ascii "ld (HL), A\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_mem

.byte 0x78, REG, A, REG, B, NULL, 1, 4 # ld A, B
.ascii "ld A, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x79, REG, A, REG, C, NULL, 1, 4 # ld A, C
.ascii "ld A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x7a, REG, A, REG, D, NULL, 1, 4 # ld A, D
.ascii "ld A, D\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x7b, REG, A, REG, E, NULL, 1, 4 # ld A, E
.ascii "ld A, E\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x7c, REG, A, REG, H, NULL, 1, 4 # ld A, H
.ascii "ld A, H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x7d, REG, A, REG, L, NULL, 1, 4 # ld A, L
.ascii "ld A, L\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x7e, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # ld A, (HL)
.ascii "ld A, (HL)\0\0\0\0\0\0"
.long BYTE
.long op_ld_mem_reg

.byte 0x7f, REG, A, REG, A, NULL, 1, 4 # ld A, A
.ascii "ld A, A\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_reg

.byte 0x80, REG, A, REG, B, NULL, 1, 4 # add A, B
.ascii "add A, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x81, REG, A, REG, C, NULL, 1, 4 # add A, C
.ascii "add A, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x82, REG, A, REG, D, NULL, 1, 4 # add A, D
.ascii "add A, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x83, REG, A, REG, E, NULL, 1, 4 # add A, E
.ascii "add A, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x84, REG, A, REG, H, NULL, 1, 4 # add A, H
.ascii "add A, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x85, REG, A, REG, L, NULL, 1, 4 # add A, L
.ascii "add A, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x86, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # add A, (HL)
.ascii "add A, (HL)\0\0\0\0\0"
.long BYTE
.long op_add_mem

.byte 0x87, REG, A, REG, A, NULL, 1, 4 # add A, A
.ascii "add A, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add

.byte 0x88, REG, A, REG, B, NULL, 1, 4 # adc A, B
.ascii "adc A, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x89, REG, A, REG, C, NULL, 1, 4 # adc A, C
.ascii "adc A, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x8a, REG, A, REG, D, NULL, 1, 4 # adc A, D
.ascii "adc A, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x8b, REG, A, REG, E, NULL, 1, 4 # adc A, E
.ascii "adc A, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x8c, REG, A, REG, H, NULL, 1, 4 # adc A, H
.ascii "adc A, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x8d, REG, A, REG, L, NULL, 1, 4 # adc A, L
.ascii "adc A, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x8e, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # adc A, (HL)
.ascii "adc A, (HL)\0\0\0\0\0"
.long BYTE
.long op_adc_mem

.byte 0x8f, REG, A, REG, A, NULL, 1, 4 # adc A, A
.ascii "adc A, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc

.byte 0x90, REG, A, REG, B, NULL, 1, 4 # sub B
.ascii "sub A, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x91, REG, A, REG, C, NULL, 1, 4 # sub C
.ascii "sub A, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x92, REG, A, REG, D, NULL, 1, 4 # sub D
.ascii "sub A, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x93, REG, A, REG, E, NULL, 1, 4 # sub E
.ascii "sub A, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x94, REG, A, REG, H, NULL, 1, 4 # sub H
.ascii "sub A, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x95, REG, A, REG, L, NULL, 1, 4 # sub L
.ascii "sub A, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x96, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # sub (HL)
.ascii "sub A, (HL)\0\0\0\0\0"
.long BYTE
.long op_sub_mem

.byte 0x97, REG, A, REG, A, NULL, 1, 4 # sub A
.ascii "sub A, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub

.byte 0x98, REG, A, REG, B, NULL, 1, 4 # sbc B
.ascii "sbc A, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0x99, REG, A, REG, C, NULL, 1, 4 # sbc C
.ascii "sbc A, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0x9a, REG, A, REG, D, NULL, 1, 4 # sbc D
.ascii "sbc A, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0x9b, REG, A, REG, E, NULL, 1, 4 # sbc E
.ascii "sbc A, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0x9c, REG, A, REG, H, NULL, 1, 4 # sbc H
.ascii "sbc A, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0x9d, REG, A, REG, L, NULL, 1, 4 # sbc L
.ascii "sbc A, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0x9e, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # sbc (HL)
.ascii "sbc A, (HL)\0\0\0\0\0"
.long BYTE
.long op_sbc_mem

.byte 0x9f, REG, A, REG, A, NULL, 1, 4 # sbc A
.ascii "sbc A, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc

.byte 0xa0, REG, A, REG, B, NULL, 1, 4 # and B
.ascii "and A, B\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa1, REG, A, REG, C, NULL, 1, 4 # and C
.ascii "and A, C\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa2, REG, A, REG, D, NULL, 1, 4 # and D
.ascii "and A, D\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa3, REG, A, REG, E, NULL, 1, 4 # and E
.ascii "and A, E\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa4, REG, A, REG, H, NULL, 1, 4 # and H
.ascii "and A, H\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa5, REG, A, REG, L, NULL, 1, 4 # and L
.ascii "and A, L\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa6, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # and (HL)
.ascii "and A, (HL)\0\0\0\0\0"
.long NULL
.long op_and_mem_accu

.byte 0xa7, REG, A, REG, A, NULL, 1, 4 # and A
.ascii "and A, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_reg_accu

.byte 0xa8, REG, A, REG, B, NULL, 1, 4 # xor B
.ascii "xor B, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xa9, REG, A, REG, C, NULL, 1, 4 # xor C
.ascii "xor C, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xaa, REG, A, REG, D, NULL, 1, 4 # xor D
.ascii "xor D, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xab, REG, A, REG, E, NULL, 1, 4 # xor E
.ascii "xor E, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xac, REG, A, REG, H, NULL, 1, 4 # xor H
.ascii "xor H, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xad, REG, A, REG, L, NULL, 1, 4 # xor L
.ascii "xor L, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xae, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # xor (HL)
.ascii "xor (HL), A\0\0\0\0\0"
.long NULL
.long op_xor_mem_accu

.byte 0xaf, REG, A, REG, A, NULL, 1, 4 # xor A
.ascii "xor A, A\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_reg_accu

.byte 0xb0, REG, A, REG, B, NULL, 1, 4 # or B
.ascii "or A, B\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb1, REG, A, REG, C, NULL, 1, 4 # or C
.ascii "or A, C\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb2, REG, A, REG, D, NULL, 1, 4 # or D
.ascii "or A, D\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb3, REG, A, REG, E, NULL, 1, 4 # or E
.ascii "or A, E\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb4, REG, A, REG, H, NULL, 1, 4 # or H
.ascii "or A, H\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb5, REG, A, REG, L, NULL, 1, 4 # or L
.ascii "or A, L\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb6, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # or (HL)
.ascii "or A, (HL)\0\0\0\0\0\0"
.long NULL
.long op_or_mem_accu

.byte 0xb7, REG, A, REG, A, NULL, 1, 4 # or A
.ascii "or A, A\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_or_reg_accu

.byte 0xb8, REG, A, REG, B, NULL, 1, 4 # cp B
.ascii "cp A, B\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xb9, REG, A, REG, C, NULL, 1, 4 # cp C
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xba, REG, A, REG, D, NULL, 1, 4 # cp D
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xbb, REG, A, REG, E, NULL, 1, 4 # cp E
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xbc, REG, A, REG, H, NULL, 1, 4 # cp H
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xbd, REG, A, REG, L, NULL, 1, 4 # cp L
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xbe, REG, A, REG_IND, HL, DELAY|RD_XOR_WR, 1, 8 # cp (HL)
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_mem_ind

.byte 0xbf, REG, A, REG, A, NULL, 1, 4 # cp A
.ascii "cp A, C\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_reg

.byte 0xc0, NULL, NULL, NULL, NULL, NULL, 1, 8 # retnz
.ascii "retnz\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ret_nz

.byte 0xc1, REG, BC, NULL, NULL, NULL, 1, 12 # pop BC
.ascii "pop BC\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_pop

.byte 0xc2, NULL, F_ZERO, NULL, NULL, NULL, 3, 12 # jnz a16
.ascii "jnz\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_nz

.byte 0xc3, NULL, NULL, NULL, NULL, NULL, 3, 16 # jmp a16
.ascii "jmp\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp

.byte 0xc4, NULL, NULL, NULL, NULL, NULL, 3, 12 # callnz NN
.ascii "callnz\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_call_nz

.byte 0xc5, REG, BC, NULL, NULL, NULL, 1, 16 # push BC
.ascii "push BC\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_push

.byte 0xc6, REG, A, IMM, NULL, NULL, 2, 8 # add A, n
.ascii "add\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_add_imm

.byte 0xc7, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0
.ascii "rst \0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_reset_0

.byte 0xc8, NULL, NULL, NULL, NULL, NULL, 1, 8 # retz
.ascii "retz\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ret_z

.byte 0xc9, NULL, NULL, NULL, NULL, NULL, 1, 16 # ret
.ascii "ret\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ret

.byte 0xca, NULL, F_ZERO, NULL, NULL, NULL, 3, 12 # jz a16
.ascii "jz\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_z

.byte 0xcb, NULL, NULL, NULL, NULL, NULL, 1, 0 # escape opcode
.ascii "escape\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_escape

.byte 0xcc, NULL, NULL, NULL, NULL, NULL, 3, 12 # callz nn
.ascii "callz\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_call_z

.byte 0xcd, NULL, NULL, NULL, NULL, NULL, 3, 24 # call nn
.ascii "call\0\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_call

.byte 0xce, REG, A, IMM, NULL, NULL, 2, 8 # adc A, imm8
.ascii "adc\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_adc_imm

.byte 0xcf, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 8H
.ascii "rst 8H\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_8h

.byte 0xd0, C, NULL, NULL, NULL, NULL, 1, 8 # retnc
.ascii "retnc\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ret_nc

.byte 0xd1, REG, DE, NULL, NULL, NULL, 1, 12 # pop DE
.ascii "pop DE\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_pop

.byte 0xd2, NULL, F_CARRY, NULL, NULL, NULL, 3, 12 # jnc a16
.ascii "jnc\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_nc

.byte 0xd3, NULL, NULL, NULL, NULL, NULL, NULL, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xd4, NULL, NULL, NULL, NULL, NULL, 3, 12 # call NC, nn
.ascii "callnc\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_call_nc

.byte 0xd5, REG, DE, NULL, NULL, NULL, 1, 16 # push DE
.ascii "push DE\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_push

.byte 0xd6, REG, A, IMM, NULL, NULL, 2, 8 # sub A, n
.ascii "sub\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sub_imm

.byte 0xd7, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0x10
.ascii "rst 10H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_10h

.byte 0xd8, C, NULL, NULL, NULL, NULL, 1, 8 # retc
.ascii "retc\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ret_c

.byte 0xd9, REG, B, REG, NULL, NULL, 1, 16 # reti
.ascii "reti\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_reti

.byte 0xda, NULL, F_CARRY, NULL, NULL, NULL, 3, 12 # jc a16
.ascii "jc\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_jmp_c

.byte 0xdb, REG, B, REG, NULL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xdc, C, NULL, NULL, NULL, NULL, 3, 12 # callc nn
.ascii "callc\0\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_call_c

.byte 0xdd, REG, B, REG, NULL, NULL, 1, 8 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xde, REG, A, IMM, NULL, NULL, 2, 8 # sbc A, n
.ascii "sbc\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_sbc_imm

.byte 0xdf, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0x18
.ascii "rst 18H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_18h

.byte 0xe0, 1, NULL, REG, A, DELAY|RD_XOR_WR, 2, 12 # ld (0xff00+n), A
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_io_imm

.byte 0xe1, REG, HL, NULL, NULL, NULL, 1, 12 # pop HL
.ascii "pop HL\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_pop

.byte 0xe2, 0x18, B, REG, A, DELAY|RD_XOR_WR, 1, 8 # ld (0xff00+C), A
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_io_reg

.byte 0xe3, REG, B, REG, NULL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xe4, REG, B, REG, NULL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xe5, REG, HL, NULL, NULL, NULL, 1, 16 # push HL
.ascii "push HL\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_push

.byte 0xe6, REG, A, IMM, NULL, NULL, 2, 8 # and A, N
.ascii "and A,\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_and_imm_accu

.byte 0xe7, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0x20
.ascii "rst 20H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_20h

.byte 0xe8, REG, B, REG, NULL, NULL, 1, 16 # add SP, dd
.ascii "add SP,\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_add_sp_sign

.byte 0xe9, REG_IND, HL, NULL, NULL, DELAY|RD_XOR_WR, 1, 4 # jmp (HL)
.ascii "jmp (HL)\0\0\0\0\0\0\0\0"
.long WORD
.long op_jmp_ind

.byte 0xea, IMM_IND, NULL, REG, A, DELAY|RD_XOR_WR, 3, 16 # ld (nn), A
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_reg_imm

.byte 0xeb, REG, DE, REG, HL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_inval

.byte 0xec, REG, B, REG, NULL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xed, REG, B, REG, NULL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xee, REG, A, IMM, NULL, NULL, 2, 8 # xor A, n
.ascii "xor A,\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_xor_imm_accu

.byte 0xef, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0x28
.ascii "rst 28H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_28h

.byte 0xf0, REG, A, 1, NULL, DELAY|RD_XOR_WR, 2, 12 # ld A, (0xff00+n)
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_ld_io_reg_imm

.byte 0xf1, REG, AF, NULL, NULL, NULL, 1, 12 # pop AF
.ascii "pop AF\0\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_pop_af

.byte 0xf2, REG, A, 0x18, NULL, DELAY|RD_XOR_WR, 1, 8 # ld A, (0xff00+C)
.ascii "ld\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_ld_io_reg_reg

.byte 0xf3, REG, B, REG, NULL, NULL, 1, 4 # di
.ascii "di\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_dis_ints

.byte 0xf4, REG, B, REG, NULL, NULL, 1, 4 # INVALID OPCODE!
.ascii "INVALD\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_inval

.byte 0xf5, REG, AF, NULL, NULL, NULL, 1, 16 # push AF
.ascii "push AF\0\0\0\0\0\0\0\0\0"
.long WORD
.long op_push

.byte 0xf6, REG, A, IMM, NULL, NULL, 2, 8 # or A, n
.ascii "or A,\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_or_imm_accu

.byte 0xf7, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0x30
.ascii "rst 30H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_30h

.byte 0xf8, REG, B, REG, NULL, NULL, 1, 12 # ld HL, SP+dd
.ascii "ld sp,\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_ld_sp_imm_hl

.byte 0xf9, REG, SP, REG, HL, NULL, 1, 8 # ld SP, HL
.ascii "ld SP, HL\0\0\0\0\0\0\0"
.long WORD
.long op_ld_hl_sp

.byte 0xfa, REG, B, REG, NULL, DELAY|RD_XOR_WR, 3, 16 # ld A, (nn)
.ascii "ld A, (nn)\0\0\0\0\0\0"
.long NULL
.long op_ld_imm_acc

.byte 0xfb, NULL, NULL, NULL, NULL, NULL, 1, 4 # enable interrupts
.ascii "ei\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long op_ena_ints

.byte 0xfc, REG, B, REG, NULL, NULL, 1, 4 # use registers vector
.ascii "ld sp,\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long 0x00000000

.byte 0xfd, REG, B, REG, NULL, NULL, 1, 4 # use registers vector
.ascii "ld sp,\0\0\0\0\0\0\0\0\0\0"
.long NULL
.long 0x00000000

.byte 0xfe, REG, A, IMM, NULL, NULL, 2, 8 # cp A, n
.ascii "cp A,\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_cp_imm_acc

.byte 0xff, NULL, NULL, NULL, NULL, NULL, 1, 16 # rst 0x38
.ascii "rst 38H\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_reset_38h

z80_cb:
.byte 0x00, REG, B, NULL, NULL, NULL, 1, 8 # rlc B
.ascii "rlc B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x01, REG, C, NULL, NULL, NULL, 1, 8 # rlc C
.ascii "rlc C\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x02, REG, D, NULL, NULL, NULL, 1, 8 # rlc D
.ascii "rlc D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x03, REG, E, NULL, NULL, NULL, 1, 8 # rlc E
.ascii "rlc E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x04, REG, H, NULL, NULL, NULL, 1, 8 # rlc H
.ascii "rlc H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x05, REG, L, NULL, NULL, NULL, 1, 8 # rlc L
.ascii "rlc L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x06, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # rlc (HL)
.ascii "rlc (HL)\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_mem

.byte 0x07, REG, A, NULL, NULL, NULL, 1, 8 # rlc A
.ascii "rlc A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_c_reg

.byte 0x08, REG, B, NULL, NULL, NULL, 1, 8 # rrc B
.ascii "rrc B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x09, REG, C, NULL, NULL, NULL, 1, 8 # rrc C
.ascii "rrc C\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x0a, REG, D, NULL, NULL, NULL, 1, 8 # rrc D
.ascii "rrc D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x0b, REG, E, NULL, NULL, NULL, 1, 8 # rrc E
.ascii "rrc E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x0c, REG, H, NULL, NULL, NULL, 1, 8 # rrc H
.ascii "rrc H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x0d, REG, L, NULL, NULL, NULL, 1, 8 # rrc L
.ascii "rrc L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x0e, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # rrc (HL)
.ascii "rrc (HL)\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_mem

.byte 0x0f, REG, A, NULL, NULL, NULL, 1, 8 # rrc A
.ascii "rrc A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_c_reg

.byte 0x10, REG, B, NULL, NULL, NULL, 1, 8 # rl B
.ascii "rl B\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x11, REG, C, NULL, NULL, NULL, 1, 8 # rl C
.ascii "rl C\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x12, REG, D, NULL, NULL, NULL, 1, 8 # rl D
.ascii "rl D\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x13, REG, E, NULL, NULL, NULL, 1, 8 # rl E
.ascii "rl E\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x14, REG, H, NULL, NULL, NULL, 1, 8 # rl H
.ascii "rl H\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x15, REG, L, NULL, NULL, NULL, 1, 8 # rl L
.ascii "rl L\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x16, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # rl (HL)
.ascii "rl (HL)\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_mem

.byte 0x17, REG, A, NULL, NULL, NULL, 1, 8 # rl A
.ascii "rl A\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_lf_reg

.byte 0x18, REG, B, NULL, NULL, NULL, 1, 8 # rr B
.ascii "rr B\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x19, REG, C, NULL, NULL, NULL, 1, 8 # rr C
.ascii "rr C\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x1a, REG, D, NULL, NULL, NULL, 1, 8 # rr D
.ascii "rr D\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x1b, REG, E, NULL, NULL, NULL, 1, 8 # rr E
.ascii "rr E\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x1c, REG, H, NULL, NULL, NULL, 1, 8 # rr H
.ascii "rr H\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x1d, REG, L, NULL, NULL, NULL, 1, 8 # rr L
.ascii "rr L\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x1e, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # rr (HL)
.ascii "rr (HL)\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_mem

.byte 0x1f, REG, A, NULL, NULL, NULL, 1, 8 # rr A
.ascii "rr A\0\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rot_rgh_reg

.byte 0x20, REG, B, NULL, NULL, NULL, 1, 8 # sla B
.ascii "sla B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x21, REG, C, NULL, NULL, NULL, 1, 8 # sla C
.ascii "sla C\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x22, REG, D, NULL, NULL, NULL, 1, 8 # sla D
.ascii "sla D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x23, REG, E, NULL, NULL, NULL, 1, 8 # sla E
.ascii "sla E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x24, REG, H, NULL, NULL, NULL, 1, 8 # sla H
.ascii "sla H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x25, REG, L, NULL, NULL, NULL, 1, 8 # sla L
.ascii "sla L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x26, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # sla (HL)
.ascii "sla (HL)\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth_mem

.byte 0x27, REG, A, NULL, NULL, NULL, 1, 8 # sla A
.ascii "sla A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_lf_arth

.byte 0x28, REG, B, NULL, NULL, NULL, 1, 8 # sra B
.ascii "sra B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x29, REG, C, NULL, NULL, NULL, 1, 8 # sra C
.ascii "sra C\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x2a, REG, D, NULL, NULL, NULL, 1, 8 # sra D
.ascii "sra D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x2b, REG, E, NULL, NULL, NULL, 1, 8 # sra E
.ascii "sra E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x2c, REG, H, NULL, NULL, NULL, 1, 8 # sra H
.ascii "sra H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x2d, REG, L, NULL, NULL, NULL, 1, 8 # sra L
.ascii "sra L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x2e, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # sra (HL)
.ascii "sra (HL)\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth_mem

.byte 0x2f, REG, A, NULL, NULL, NULL, 1, 8 # sra A
.ascii "sra A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_arth

.byte 0x30, REG, B, NULL, NULL, NULL, 1, 8 # swap B
.ascii "swap B\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x31, REG, C, NULL, NULL, NULL, 1, 8 # swap C
.ascii "swap C\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x32, REG, D, NULL, NULL, NULL, 1, 8 # swap D
.ascii "swap D\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x33, REG, E, NULL, NULL, NULL, 1, 8 # swap E
.ascii "swap E\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x34, REG, H, NULL, NULL, NULL, 1, 8 # swap H
.ascii "swap H\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x35, REG, L, NULL, NULL, NULL, 1, 8 # swap L
.ascii "swap L\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x36, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # swap (HL)
.ascii "swap (HL)\0\0\0\0\0\0\0"
.long BYTE
.long op_swp_mem

.byte 0x37, REG, A, NULL, NULL, NULL, 1, 8 # swap A
.ascii "swap A\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_swp

.byte 0x38, REG, B, NULL, NULL, NULL, 1, 8 # srl B
.ascii "srl B\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x39, REG, C, NULL, NULL, NULL, 1, 8 # srl C
.ascii "srl C\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x3a, REG, D, NULL, NULL, NULL, 1, 8 # srl D
.ascii "srl D\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x3b, REG, E, NULL, NULL, NULL, 1, 8 # srl E
.ascii "srl E\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x3c, REG, H, NULL, NULL, NULL, 1, 8 # srl H
.ascii "srl H\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x3d, REG, L, NULL, NULL, NULL, 1, 8 # srl L
.ascii "srl L\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x3e, REG_IND, HL, NULL, NULL, DELAY|RD_WR, 1, 16 # srl (HL)
.ascii "srl (HL)\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg_mem

.byte 0x3f, REG, A, NULL, NULL, NULL, 1, 8 # srl A
.ascii "srl A\0\0\0\0\0\0\0\0\0\0\0"
.long BYTE
.long op_shf_rgh_lg

.byte 0x40, REG, B, BIT_1, NULL, NULL, 1, 8 # bit 1, B
.ascii "bit 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x41, REG, C, BIT_1, NULL, NULL, 1, 8 # bit 1, C
.ascii "bit 1, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x42, REG, D, BIT_1, NULL, NULL, 1, 8 # bit 1, D
.ascii "bit 1, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x43, REG, E, BIT_1, NULL, NULL, 1, 8 # bit 1, E
.ascii "bit 1, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x44, REG, H, BIT_1, NULL, NULL, 1, 8 # bit 1, H
.ascii "bit 1, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x45, REG, L, BIT_1, NULL, NULL, 1, 8 # bit 1, L
.ascii "bit 1, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x46, REG_IND, HL, BIT_1, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 1, (HL)
.ascii "bit 1, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x47, REG, A, BIT_1, NULL, NULL, 1, 8 # bit 1, A
.ascii "bit 1, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x48, REG, B, BIT_2, NULL, NULL, 1, 8 # bit 2, B
.ascii "bit 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x49, REG, C, BIT_2, NULL, NULL, 1, 8 # bit 2, C
.ascii "bit 2, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x4a, REG, D, BIT_2, NULL, NULL, 1, 8 # bit 2, D
.ascii "bit 2, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x4b, REG, E, BIT_2, NULL, NULL, 1, 8 # bit 2, E
.ascii "bit 2, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x4c, REG, H, BIT_2, NULL, NULL, 1, 8 # bit 2, H
.ascii "bit 2, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x4d, REG, L, BIT_2, NULL, NULL, 1, 8 # bit 2, L
.ascii "bit 2, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x4e, REG_IND, HL, BIT_2, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 2, (HL)
.ascii "bit 2, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x4f, REG, A, BIT_2, NULL, NULL, 1, 8 # bit 2, A
.ascii "bit 2, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x50, REG, B, BIT_3, NULL, NULL, 1, 8 # bit 3, B
.ascii "bit 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x51, REG, C, BIT_3, NULL, NULL, 1, 8 # bit 3, C
.ascii "bit 3, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x52, REG, D, BIT_3, NULL, NULL, 1, 8 # bit 3, D
.ascii "bit 3, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x53, REG, E, BIT_3, NULL, NULL, 1, 8 # bit 3, E
.ascii "bit 3, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x54, REG, H, BIT_3, NULL, NULL, 1, 8 # bit 3, H
.ascii "bit 3, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x55, REG, L, BIT_3, NULL, NULL, 1, 8 # bit 3, L
.ascii "bit 3, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x56, REG_IND, HL, BIT_3, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 3, (HL)
.ascii "bit 3, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x57, REG, A, BIT_3, NULL, NULL, 1, 8 # bit 3, A
.ascii "bit 3, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x58, REG, B, BIT_4, NULL, NULL, 1, 8 # bit 4, B
.ascii "bit 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x59, REG, C, BIT_4, NULL, NULL, 1, 8 # bit 4, C
.ascii "bit 4, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x5a, REG, D, BIT_4, NULL, NULL, 1, 8 # bit 4, D
.ascii "bit 4, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x5b, REG, E, BIT_4, NULL, NULL, 1, 8 # bit 4, E
.ascii "bit 4, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x5c, REG, H, BIT_4, NULL, NULL, 1, 8 # bit 4, H
.ascii "bit 4, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x5d, REG, L, BIT_4, NULL, NULL, 1, 8 # bit 4, L
.ascii "bit 4, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x5e, REG_IND, HL, BIT_4, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 4, (HL)
.ascii "bit 4, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x5f, REG, A, BIT_4, NULL, NULL, 1, 8 # bit 4, A
.ascii "bit 4, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x60, REG, B, BIT_5, NULL, NULL, 1, 8 # bit 5, B
.ascii "bit 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x61, REG, C, BIT_5, NULL, NULL, 1, 8 # bit 5, C
.ascii "bit 5, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x62, REG, D, BIT_5, NULL, NULL, 1, 8 # bit 5, D
.ascii "bit 5, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x63, REG, E, BIT_5, NULL, NULL, 1, 8 # bit 5, E
.ascii "bit 5, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x64, REG, H, BIT_5, NULL, NULL, 1, 8 # bit 5, H
.ascii "bit 5, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x65, REG, L, BIT_5, NULL, NULL, 1, 8 # bit 5, L
.ascii "bit 5, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x66, REG_IND, HL, BIT_5, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 5, (HL)
.ascii "bit 5, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x67, REG, A, BIT_5, NULL, NULL, 1, 8 # bit 5, A
.ascii "bit 5, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x68, REG, B, BIT_6, NULL, NULL, 1, 8 # bit 6, B
.ascii "bit 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x69, REG, C, BIT_6, NULL, NULL, 1, 8 # bit 6, C
.ascii "bit 6, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x6a, REG, D, BIT_6, NULL, NULL, 1, 8 # bit 6, D
.ascii "bit 6, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x6b, REG, E, BIT_6, NULL, NULL, 1, 8 # bit 6, E
.ascii "bit 6, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x6c, REG, H, BIT_6, NULL, NULL, 1, 8 # bit 6, H
.ascii "bit 6, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x6d, REG, L, BIT_6, NULL, NULL, 1, 8 # bit 6, L
.ascii "bit 6, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x6e, REG_IND, HL, BIT_6, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 6, (HL)
.ascii "bit 6, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x6f, REG, A, BIT_6, NULL, NULL, 1, 8 # bit 6, A
.ascii "bit 6, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x70, REG, B, BIT_7, NULL, NULL, 1, 8 # bit 7, B
.ascii "bit 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x71, REG, C, BIT_7, NULL, NULL, 1, 8 # bit 7, C
.ascii "bit 7, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x72, REG, D, BIT_7, NULL, NULL, 1, 8 # bit 7, D
.ascii "bit 7, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x73, REG, E, BIT_7, NULL, NULL, 1, 8 # bit 7, E
.ascii "bit 7, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x74, REG, H, BIT_7, NULL, NULL, 1, 8 # bit 7, H
.ascii "bit 7, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x75, REG, L, BIT_7, NULL, NULL, 1, 8 # bit 7, L
.ascii "bit 7, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x76, REG_IND, HL, BIT_7, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 7, (HL)
.ascii "bit 7, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x77, REG, A, BIT_7, NULL, NULL, 1, 8 # bit 7, A
.ascii "bit 7, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x78, REG, B, BIT_8, NULL, NULL, 1, 8 # bit 8, B
.ascii "bit 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x79, REG, C, BIT_8, NULL, NULL, 1, 8 # bit 8, C
.ascii "bit 8, C\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x7a, REG, D, BIT_8, NULL, NULL, 1, 8 # bit 8, D
.ascii "bit 8, D\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x7b, REG, E, BIT_8, NULL, NULL, 1, 8 # bit 8, E
.ascii "bit 8, E\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x7c, REG, H, BIT_8, NULL, NULL, 1, 8 # bit 8, H
.ascii "bit 8, H\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x7d, REG, L, BIT_8, NULL, NULL, 1, 8 # bit 8, L
.ascii "bit 8, L\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x7e, REG_IND, HL, BIT_8, NULL, DELAY|RD_XOR_WR, 1, 12 # bit 8, (HL)
.ascii "bit 8, (HL)\0\0\0\0\0"
.long BYTE
.long op_bit_mem

.byte 0x7f, REG, A, BIT_8, NULL, NULL, 1, 8 # bit 8, A
.ascii "bit 8, A\0\0\0\0\0\0\0\0"
.long BYTE
.long op_bit

.byte 0x80, REG, B, BIT_1, NULL, NULL, 1, 8 # rst 1, B
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x81, REG, C, BIT_1, NULL, NULL, 1, 8 # rst 1, C
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x82, REG, D, BIT_1, NULL, NULL, 1, 8 # rst 1, D
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x83, REG, E, BIT_1, NULL, NULL, 1, 8 # rst 1, E
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x84, REG, H, BIT_1, NULL, NULL, 1, 8 # rst 1, H
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x85, REG, L, BIT_1, NULL, NULL, 1, 8 # rst 1, L
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x86, REG_IND, HL, BIT_1, NULL, DELAY|RD_WR, 1, 16 # rst 1, (HL)
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0x87, REG, A, BIT_1, NULL, NULL, 1, 8 # rst 1, A
.ascii "rst 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x88, REG, B, BIT_2, NULL, NULL, 1, 8 # rst 2, B
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x89, REG, C, BIT_2, NULL, NULL, 1, 8 # rst 2, C
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x8a, REG, D, BIT_2, NULL, NULL, 1, 8 # rst 2, D
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x8b, REG, E, BIT_2, NULL, NULL, 1, 8 # rst 2, E
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x8c, REG, H, BIT_2, NULL, NULL, 1, 8 # rst 2, H
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x8d, REG, L, BIT_2, NULL, NULL, 1, 8 # rst 2, L
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x8e, REG_IND, HL, BIT_2, NULL, DELAY|RD_WR, 1, 16 # rst 2, (HL)
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0x8f, REG, A, BIT_2, NULL, NULL, 1, 8 # rst 2, A
.ascii "rst 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x90, REG, B, BIT_3, NULL, NULL, 1, 8 # rst 3, B
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x91, REG, C, BIT_3, NULL, NULL, 1, 8 # rst 3, C
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x92, REG, D, BIT_3, NULL, NULL, 1, 8 # rst 3, D
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x93, REG, E, BIT_3, NULL, NULL, 1, 8 # rst 3, E
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x94, REG, H, BIT_3, NULL, NULL, 1, 8 # rst 3, H
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x95, REG, L, BIT_3, NULL, NULL, 1, 8 # rst 3, L
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x96, REG_IND, HL, BIT_3, NULL, DELAY|RD_WR, 1, 16 # rst 3, (HL)
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0x97, REG, A, BIT_3, NULL, NULL, 1, 8 # rst 3, A
.ascii "rst 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x98, REG, B, BIT_4, NULL, NULL, 1, 8 # rst 4, B
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x99, REG, C, BIT_4, NULL, NULL, 1, 8 # rst 4, C
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x9a, REG, D, BIT_4, NULL, NULL, 1, 8 # rst 4, D
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x9b, REG, E, BIT_4, NULL, NULL, 1, 8 # rst 4, E
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x9c, REG, H, BIT_4, NULL, NULL, 1, 8 # rst 4, H
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x9d, REG, L, BIT_4, NULL, NULL, 1, 8 # rst 4, L
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0x9e, REG_IND, HL, BIT_4, NULL, DELAY|RD_WR, 1, 16 # rst 4, (HL)
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0x9f, REG, A, BIT_4, NULL, NULL, 1, 8 # rst 4, A
.ascii "rst 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa0, REG, B, BIT_5, NULL, NULL, 1, 8 # rst 5, B
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa1, REG, C, BIT_5, NULL, NULL, 1, 8 # rst 5, C
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa2, REG, D, BIT_5, NULL, NULL, 1, 8 # rst 5, D
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa3, REG, E, BIT_5, NULL, NULL, 1, 8 # rst 5, E
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa4, REG, H, BIT_5, NULL, NULL, 1, 8 # rst 5, H
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa5, REG, L, BIT_5, NULL, NULL, 1, 8 # rst 5, L
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa6, REG_IND, HL, BIT_5, NULL, DELAY|RD_WR, 1, 16 # rst 5, (HL)
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0xa7, REG, A, BIT_5, NULL, NULL, 1, 8 # rst 5, A
.ascii "rst 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa8, REG, B, BIT_6, NULL, NULL, 1, 8 # rst 6, B
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xa9, REG, C, BIT_6, NULL, NULL, 1, 8 # rst 6, C
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xaa, REG, D, BIT_6, NULL, NULL, 1, 8 # rst 6, D
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xab, REG, E, BIT_6, NULL, NULL, 1, 8 # rst 6, E
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xac, REG, H, BIT_6, NULL, NULL, 1, 8 # rst 6, H
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xad, REG, L, BIT_6, NULL, NULL, 1, 8 # rst 6, L
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xae, REG_IND, HL, BIT_6, NULL, DELAY|RD_WR, 1, 16 # rst 6, (HL)
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0xaf, REG, A, BIT_6, NULL, NULL, 1, 8 # rst 6, A
.ascii "rst 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb0, REG, B, BIT_7, NULL, NULL, 1, 8 # rst 7, B
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb1, REG, C, BIT_7, NULL, NULL, 1, 8 # rst 7, C
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb2, REG, D, BIT_7, NULL, NULL, 1, 8 # rst 7, D
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb3, REG, E, BIT_7, NULL, NULL, 1, 8 # rst 7, E
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb4, REG, H, BIT_7, NULL, NULL, 1, 8 # rst 7, H
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb5, REG, L, BIT_7, NULL, NULL, 1, 8 # rst 7, L
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb6, REG_IND, HL, BIT_7, NULL, DELAY|RD_WR, 1, 16 # rst 7, (HL)
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0xb7, REG, A, BIT_7, NULL, NULL, 1, 8 # rst 7, A
.ascii "rst 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb8, REG, B, BIT_8, NULL, NULL, 1, 8 # rst 8, B
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xb9, REG, C, BIT_8, NULL, NULL, 1, 8 # rst 8, C
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xba, REG, D, BIT_8, NULL, NULL, 1, 8 # rst 8, D
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xbb, REG, E, BIT_8, NULL, NULL, 1, 8 # rst 8, E
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xbc, REG, H, BIT_8, NULL, NULL, 1, 8 # rst 8, H
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xbd, REG, L, BIT_8, NULL, NULL, 1, 8 # rst 8, L
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xbe, REG_IND, HL, BIT_8, NULL, DELAY|RD_WR, 1, 16 # rst 8, (HL)
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst_mem

.byte 0xbf, REG, A, BIT_8, NULL, NULL, 1, 8 # rst 8, A
.ascii "rst 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_rst

.byte 0xc0, REG, B, BIT_1, NULL, NULL, 1, 8 # set 1, B
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc1, REG, C, BIT_1, NULL, NULL, 1, 8 # set 1, C
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc2, REG, D, BIT_1, NULL, NULL, 1, 8 # set 1, D
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc3, REG, E, BIT_1, NULL, NULL, 1, 8 # set 1, E
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc4, REG, H, BIT_1, NULL, NULL, 1, 8 # set 1, H
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc5, REG, L, BIT_1, NULL, NULL, 1, 8 # set 1, L
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc6, REG_IND, HL, BIT_1, NULL, DELAY|RD_WR, 1, 16 # set 1, (HL)
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xc7, REG, A, BIT_1, NULL, NULL, 1, 8 # set 1, A
.ascii "set 1, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc8, REG, B, BIT_2, NULL, NULL, 1, 8 # set 2, B
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xc9, REG, C, BIT_2, NULL, NULL, 1, 8 # set 2, C
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xca, REG, D, BIT_2, NULL, NULL, 1, 8 # set 2, D
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xcb, REG, E, BIT_2, NULL, NULL, 1, 8 # set 2, E
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xcc, REG, H, BIT_2, NULL, NULL, 1, 8 # set 2, H
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xcd, REG, L, BIT_2, NULL, NULL, 1, 8 # set 2, L
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xce, REG_IND, HL, BIT_2, NULL, DELAY|RD_WR, 1, 16 # set 2, (HL)
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xcf, REG, A, BIT_2, NULL, NULL, 1, 8 # set 2, A
.ascii "set 2, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd0, REG, B, BIT_3, NULL, NULL, 1, 8 # set 3, B
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd1, REG, C, BIT_3, NULL, NULL, 1, 8 # set 3, C
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd2, REG, D, BIT_3, NULL, NULL, 1, 8 # set 3, D
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd3, REG, E, BIT_3, NULL, NULL, 1, 8 # set 3, E
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd4, REG, H, BIT_3, NULL, NULL, 1, 8 # set 3, H
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd5, REG, L, BIT_3, NULL, NULL, 1, 8 # set 3, L
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd6, REG_IND, HL, BIT_3, NULL, DELAY|RD_WR, 1, 16 # set 3, (HL)
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xd7, REG, A, BIT_3, NULL, NULL, 1, 8 # set 3, A
.ascii "set 3, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd8, REG, B, BIT_4, NULL, NULL, 1, 8 # set 4, B
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xd9, REG, C, BIT_4, NULL, NULL, 1, 8 # set 4, C
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xda, REG, D, BIT_4, NULL, NULL, 1, 8 # set 4, D
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xdb, REG, E, BIT_4, NULL, NULL, 1, 8 # set 4, E
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xdc, REG, H, BIT_4, NULL, NULL, 1, 8 # set 4, H
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xdd, REG, L, BIT_4, NULL, NULL, 1, 8 # set 4, L
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xde, REG_IND, HL, BIT_4, NULL, DELAY|RD_WR, 1, 16 # set 4, (HL)
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xdf, REG, A, BIT_4, NULL, NULL, 1, 8 # set 4, A
.ascii "set 4, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe0, REG, B, BIT_5, NULL, NULL, 1, 8 # set 5, B
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe1, REG, C, BIT_5, NULL, NULL, 1, 8 # set 5, C
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe2, REG, D, BIT_5, NULL, NULL, 1, 8 # set 5, D
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe3, REG, E, BIT_5, NULL, NULL, 1, 8 # set 5, E
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe4, REG, H, BIT_5, NULL, NULL, 1, 8 # set 5, H
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe5, REG, L, BIT_5, NULL, NULL, 1, 8 # set 5, L
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe6, REG_IND, HL, BIT_5, NULL, DELAY|RD_WR, 1, 16 # set 5, (HL)
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xe7, REG, A, BIT_5, NULL, NULL, 1, 8 # set 5, A
.ascii "set 5, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe8, REG, B, BIT_6, NULL, NULL, 1, 8 # set 6, B
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xe9, REG, C, BIT_6, NULL, NULL, 1, 8 # set 6, C
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xea, REG, D, BIT_6, NULL, NULL, 1, 8 # set 6, D
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xeb, REG, E, BIT_6, NULL, NULL, 1, 8 # set 6, E
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xec, REG, H, BIT_6, NULL, NULL, 1, 8 # set 6, H
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xed, REG, L, BIT_6, NULL, NULL, 1, 8 # set 6, L
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xee, REG_IND, HL, BIT_6, NULL, DELAY|RD_WR, 1, 16 # set 6, (HL)
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xef, REG, A, BIT_6, NULL, NULL, 1, 8 # set 6, A
.ascii "set 6, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf0, REG, B, BIT_7, NULL, NULL, 1, 8 # set 7, B
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf1, REG, C, BIT_7, NULL, NULL, 1, 8 # set 7, C
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf2, REG, D, BIT_7, NULL, NULL, 1, 8 # set 7, D
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf3, REG, E, BIT_7, NULL, NULL, 1, 8 # set 7, E
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf4, REG, H, BIT_7, NULL, NULL, 1, 8 # set 7, H
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf5, REG, L, BIT_7, NULL, NULL, 1, 8 # set 7, L
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf6, REG_IND, HL, BIT_7, NULL, DELAY|RD_WR, 1, 16 # set 7, (HL)
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xf7, REG, A, BIT_7, NULL, NULL, 1, 8 # set 7, A
.ascii "set 7, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf8, REG, B, BIT_8, NULL, NULL, 1, 8 # set 8, B
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xf9, REG, C, BIT_8, NULL, NULL, 1, 8 # set 8, C
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xfa, REG, D, BIT_8, NULL, NULL, 1, 8 # set 8, D
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xfb, REG, E, BIT_8, NULL, NULL, 1, 8 # set 8, E
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xfc, REG, H, BIT_8, NULL, NULL, 1, 8 # set 8, H
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xfd, REG, L, BIT_8, NULL, NULL, 1, 8 # set 8, L
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.byte 0xfe, REG_IND, HL, BIT_8, NULL, DELAY|RD_WR, 1, 16 # set 8, (HL)
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set_mem

.byte 0xff, REG, A, BIT_8, NULL, NULL, 1, 8 # set 8, A
.ascii "set 8, B\0\0\0\0\0\0\0\0"
.long BYTE
.long op_set

.p2align 3
# values according to Game Boy type
# the defaults for the following are for DMG; they change if type is set to CGB
gb_clk_rate:
.quad 4194304
gb_line_clks:
.quad 459
gb_vbln_clks:
.quad 4590, 4590
gb_oam_clks:
.quad 80, 80
gb_hblank_clks:
.quad 200, 200
gb_vram_clks:
.quad 176, 176
# these values are commong to Game Boy types
ints_offs:
.quad 0x40, 0x48, 0x50, 0x58, 0x60
addr_sp_ptrs:
.quad	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
ime_flag:
.quad 1
# depending on the number of sprites for the line, mode 3 takes extra cycles
spr_extr_cycles:
.quad 0, 8, 20, 32, 44, 52, 64, 76, 88, 96, 108

# the following must be initialized to 0 before returning from exec_next
set_zero_addr:
# variables for doing HBlank-Driven DMA
hbln_dma_src:
.quad 0
hbln_dma_dst:
.quad 0
hbln_dma:
.quad 0
hdma_on:
.quad 0
cpu_cur_mode:
.quad 0
lcd_vbln_hbln_ctrl:
.quad 0x00000000
div_ctrl:
.quad 0
cpu_halt:
.quad 0
ret_int:
.quad 0
tac_on:
.quad 0
tac_counter:
.quad 0
tac_reload:
.quad 0
skip_next_frame:
.quad 0
nb_spr:
.quad 0
spr_cur_extr:
.quad 0
just_enabled:
.quad 0
write_is_delayed:
.quad 0
inst_is_cb:
.quad 0
regs_sets:
.fill 384, 1, 0
temp_var:
.quad 0
set_zero_addr_end:
print_str:
.asciz "%x "
