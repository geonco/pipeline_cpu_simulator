/* **************************************
 * Module: top design of rv32i pipelined processor
 *
 * Author:
 *
 * **************************************
 */

#include "rv32i.h"

int main (int argc, char *argv[]) {

	// get input arguments
	FILE *f_imem, *f_dmem;
	if (argc < 3) {
		printf("usage: %s imem_data_file dmem_data_file\n", argv[0]);
		exit(1);
	}

	if ( (f_imem = fopen(argv[1], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[1]);
		exit(1);
	}
	if ( (f_dmem = fopen(argv[2], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[2]);
		exit(1);
	}

	// memory data (global)
	uint32_t *reg_data;
	uint32_t *imem_data;
	uint32_t *dmem_data;

	reg_data = (uint32_t*)malloc(32*sizeof(uint32_t));
	imem_data = (uint32_t*)malloc(IMEM_DEPTH*sizeof(uint32_t));
	dmem_data = (uint32_t*)malloc(DMEM_DEPTH*sizeof(uint32_t));

	// initialize memory data
	int i, j, k;
	for (i = 0; i < 32; i++) reg_data[i] = 0;
	for (i = 0; i < IMEM_DEPTH; i++) imem_data[i] = 0;
	for (i = 0; i < DMEM_DEPTH; i++) dmem_data[i] = 0;

	uint32_t d, buf;
	i = 0;
	printf("\n*** Reading %s ***\n", argv[1]);
	while (fscanf(f_imem, "%1d", &buf) != EOF) {
		d = buf << 31;
		for (k = 30; k >= 0; k--) {
			if (fscanf(f_imem, "%1d", &buf) != EOF) {
				d |= buf << k;
			} else {
				printf("Incorrect format!!\n");
				exit(1);
			}
		}
		imem_data[i] = d;
		printf("imem[%03d]: %08X\n", i, imem_data[i]);
		i++;
	}

	i = 0;
	printf("\n*** Reading %s ***\n", argv[2]);
	while (fscanf(f_dmem, "%8x", &buf) != EOF) {
		dmem_data[i] = buf;
		printf("dmem[%03d]: %08X\n", i, dmem_data[i]);
		i++;
	}

	fclose(f_imem);
	fclose(f_dmem);

	// processor model
	uint32_t pc_curr = 0, pc_next;	// program counter
	struct imem_input_t imem_in;
	struct imem_output_t imem_out;

	// pipeline registers
	struct pipe_if_id_t id = {0};
	struct pipe_id_ex_t ex = {0};
	struct pipe_ex_mem_t mem = {0};
	struct pipe_mem_wb_t wb = {0};

	uint32_t cc = 2;	// clock count
	while (cc < CLK_NUM) {

		// write back stage
		uint32_t rd_din;
		rd_din = wb.mem_to_reg ? wb.dmem_dout : wb.alu_result;
		if (wb.reg_write && wb.rd != 0) reg_data[wb.rd] = rd_din;

		// memory stage
		uint32_t dmem_addr = (mem.alu_result >> 2) & (DMEM_DEPTH - 1);
		uint32_t dmem_dout = dmem_data[dmem_addr];

		if (mem.mem_write) dmem_data[dmem_addr] = mem.rs2_dout;

		uint32_t load_ext;
		switch (mem.funct3)
		{
			case 0x0: load_ext = (int32_t)(int8_t)(dmem_dout & 0xff);   break;  // LB
			case 0x1: load_ext = (int32_t)(int16_t)(dmem_dout & 0xffff); break; // LH
			case 0x4: load_ext = dmem_dout & 0xff;    break;                    // LBU
			case 0x5: load_ext = dmem_dout & 0xffff;  break;                    // LHU
			default:  load_ext = dmem_dout;           break;                    // LW
		}

		wb.alu_result = mem.alu_result;
		wb.dmem_dout = load_ext;
		wb.rd = mem.rd;
		wb.reg_write = mem.reg_write;
		wb.mem_to_reg = mem.mem_to_reg;

		// execution stage
		uint32_t alu_control;
		if ((ex.alu_op >> 1) & 1) {
			switch (ex.funct3) 
			{
				case 0x0: alu_control = (~ex.alu_src & ((ex.funct7 >> 5) & 1)) ? 0x6 : 0x2;	break;
				case 0x1: alu_control = 0x4;	break;
				case 0x2: alu_control = 0x8;	break;
				case 0x3: alu_control = 0x9;	break;
				case 0x4: alu_control = 0x3;	break;
				case 0x5: alu_control = ((ex.funct7 >> 5) & 1) ? 0x7 : 0x5;	break;
				case 0x6: alu_control = 0x1;	break;
				case 0x7: alu_control = 0x0;	break;
				default: alu_control = 0x2;		break;
			}
		}
		else {
			alu_control = (ex.alu_op & 1) ? 0x6 : 0x2;
		}

		uint32_t forward_a, forward_b, alu_fwd_in1, alu_fwd_in2;

		// forward_a (rs1)
        if (mem.reg_write && (mem.rd != 0) && (mem.rd == ex.rs1))
            forward_a = 0x2;
        else if (wb.reg_write && (wb.rd != 0) && (wb.rd == ex.rs1))
            forward_a = 0x1;
        else
            forward_a = 0x0;

        // forward_b (rs2)
        if (mem.reg_write && (mem.rd != 0) && (mem.rd == ex.rs2))
            forward_b = 0x2;
        else if (wb.reg_write && (wb.rd != 0) && (wb.rd == ex.rs2))
            forward_b = 0x1;
        else
            forward_b = 0x0;

		alu_fwd_in1 = (forward_a == 0x2) ? mem.alu_result :
					  (forward_a == 0x1) ? rd_din :
					  ex.rs1_dout;

		alu_fwd_in2 = (forward_b == 0x2) ? mem.alu_result :
					  (forward_b == 0x1) ? rd_din :
					  ex.rs2_dout;

		uint32_t alu_in1, alu_in2, alu_result;
		alu_in1 = (ex.auipc) ? ex.pc :
                  (ex.lui) ? 0 :
                  alu_fwd_in1;
   		alu_in2 = (ex.alu_src) ? ex.imm32 : alu_fwd_in2;

		switch (alu_control) {
			case 0x0: alu_result = alu_in1 & alu_in2; break;
			case 0x1: alu_result = alu_in1 | alu_in2; break;
			case 0x2: alu_result = alu_in1 + alu_in2; break;
			case 0x3: alu_result = alu_in1 ^ alu_in2; break;
			case 0x4: alu_result = alu_in1 << (alu_in2 & 0x1f); break;
			case 0x5: alu_result = alu_in1 >> (alu_in2 & 0x1f); break;
			case 0x6: alu_result = alu_in1 - alu_in2; break;
			case 0x7: alu_result = (uint32_t)((int32_t)alu_in1 >> (alu_in2 & 0x1f)); break;
			case 0x8: alu_result = ((int32_t)alu_in1 < (int32_t)alu_in2) ? 1 : 0; break;
			case 0x9: alu_result = (alu_in1 < alu_in2) ? 1 : 0; break;
			default:  alu_result = 0; break;
		}

		// branch unit
		uint32_t sub_for_branch, bu_zero, bu_sign, bu_sign_u, branch_taken;
		sub_for_branch = alu_fwd_in1 - alu_fwd_in2;
    	bu_zero = (sub_for_branch == 0);
    	bu_sign = (sub_for_branch >> 31) & 1;
    	bu_sign_u = (alu_fwd_in1 < alu_fwd_in2);
    	branch_taken = ((ex.branch >> 0) & 1) & bu_zero              // beq
                        | ((ex.branch >> 1) & 1) & ~bu_zero          // bne
                        | ((ex.branch >> 2) & 1) & bu_sign           // blt
                        | ((ex.branch >> 3) & 1) & ~bu_sign          // bge
                        | ((ex.branch >> 4) & 1) & bu_sign_u         // bltu
                        | ((ex.branch >> 5) & 1) & ~bu_sign_u;       // bgeu

		uint32_t pc_next_sel, pc_target;
		pc_next_sel = branch_taken | ex.jal | ex.jalr;
		pc_target = (ex.jalr) ? ((alu_fwd_in1 + ex.imm32) & ~1u) :
                    (ex.jal) ? (ex.pc + ex.imm32) :
					ex.pc + (ex.imm32 << 1);

        mem.alu_result = (ex.jal | ex.jalr) ? (ex.pc + 4) : alu_result;
        mem.rs2_dout = alu_fwd_in2;
        mem.funct3 = ex.funct3;
        mem.mem_read = ex.mem_read;
        mem.mem_write = ex.mem_write;
        mem.rd = ex.rd;
        mem.reg_write = ex.reg_write;
        mem.mem_to_reg = ex.mem_to_reg;

		// instruction decode stage
		uint32_t opcode, rd, funct3, rs1, rs2, funct7, is_rtype, is_itype, lui, auipc, jal, jalr, mem_read, mem_write, mem_to_reg, is_branch, branch, reg_write, alu_src, alu_op, imm12, imm32, rs1_dout, rs2_dout, stall_by_load_use, flush_by_branch, flush;

		opcode = id.inst & 0x7f;
		rd = (id.inst >> 7)  & 0x1f;
		funct3 = (id.inst >> 12) & 0x7;
		rs1 = (id.inst >> 15) & 0x1f;
		rs2 = (id.inst >> 20) & 0x1f;
		funct7 = (id.inst >> 25) & 0x7f;

		is_rtype = (opcode == 0x33);
		is_itype = (opcode == 0x13);
		lui = (opcode == 0x37);
		auipc = (opcode == 0x17);
		jal = (opcode == 0x6f);
		jalr = (opcode == 0x67);
		mem_read = (opcode == 0x03);
		mem_write = (opcode == 0x23);
		mem_to_reg = mem_read;

		is_branch = (opcode == 0x63);
		branch = 0;
		branch |= (is_branch && funct3 == 0x0) << 0; // beq
		branch |= (is_branch && funct3 == 0x1) << 1; // bne
		branch |= (is_branch && funct3 == 0x4) << 2; // blt
		branch |= (is_branch && funct3 == 0x5) << 3; // bge
		branch |= (is_branch && funct3 == 0x6) << 4; // bltu
		branch |= (is_branch && funct3 == 0x7) << 5; // bgeu

		reg_write = is_rtype | is_itype | mem_read | lui | auipc | jal | jalr;
		alu_src = mem_read | mem_write | is_itype | lui | auipc;
		alu_op = ((is_rtype | is_itype) << 1) | (branch ? 1 : 0);

		// immediate generator
		if (mem_write)                                    	// S-type
			imm12 = ((id.inst >> 25) << 5) | ((id.inst >> 7) & 0x1f);
		else if (is_branch)                                 // B-type
			imm12 = (((id.inst >> 31) & 1) << 11)
				  | (((id.inst >> 7)  & 1) << 10)
				  | (((id.inst >> 25) & 0x3f) << 4)
				  | ((id.inst >> 8) & 0xf);
		else                                                // I-type
			imm12 = (id.inst >> 20) & 0xfff;

		if (lui | auipc)                                    // U-type
			imm32 = id.inst & 0xfffff000;
		else if (jal)                                       // J-type
			imm32 = ((uint32_t)((int32_t)id.inst >> 31) << 20)
				  | (id.inst & 0xff000)
				  | (((id.inst >> 20) & 1) << 11) 
				  | (((id.inst >> 21) & 0x3ff) << 1);
		else                                                // I/S/B
			imm32 = (uint32_t)(((int32_t)(imm12 << 20)) >> 20);


        rs1_dout = reg_data[rs1];
        rs2_dout = reg_data[rs2];

		// hazard detection
		stall_by_load_use = ex.mem_read && ((ex.rd == rs1) || (ex.rd == rs2)) && (ex.rd != 0);
		flush_by_branch = branch_taken | ex.jal | ex.jalr;
		flush = stall_by_load_use | flush_by_branch;

		if (flush) {
			struct pipe_id_ex_t z = {0};
			ex = z;
		}
		else {
			ex.pc = id.pc;
			ex.rs1_dout = rs1_dout;
			ex.rs2_dout = rs2_dout;
			ex.imm32 = imm32;
			ex.funct3 = funct3;
			ex.funct7 = funct7;
			ex.branch = branch;
			ex.alu_src = alu_src;
			ex.alu_op = alu_op;
			ex.mem_read = mem_read;
			ex.mem_write = mem_write;
			ex.rs1 = rs1;
			ex.rs2 = rs2;
			ex.rd = rd;
			ex.reg_write = reg_write;
			ex.mem_to_reg = mem_to_reg;
			ex.lui = lui;
			ex.auipc = auipc;
			ex.jal = jal;
			ex.jalr = jalr;
		}

		// instruction fetch stage
		uint32_t pc_next_plus4, imem_addr, inst, pc_write, stall; 
		
		pc_write = ~stall_by_load_use & 1;
		flush = flush_by_branch;
		stall = stall_by_load_use;

		pc_next_plus4 = pc_curr + 4;
		pc_next = pc_next_sel ? pc_target : pc_next_plus4;

		imem_addr = (pc_curr >> 2) & (IMEM_DEPTH - 1);
		inst = imem_data[imem_addr];

		if (flush) {
			struct pipe_if_id_t z = {0};
			id = z;
		}
		else if (!stall) {
			id.pc = pc_curr;
			id.inst = inst;
		}

		if (pc_write) pc_curr = pc_next;

		cc++;
	}

	free(reg_data);
	free(imem_data);
	free(dmem_data);

	return 1;
}
