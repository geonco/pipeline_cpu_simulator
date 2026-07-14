/* **************************************
 * Module: top design of rv32i pipelined processor
 *
 * Author:
 *
 * **************************************
 */

// headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


// defines
#define REG_WIDTH 32
#define IMEM_DEPTH 1024
#define DMEM_DEPTH 1024

// configs
#define CLK_NUM 45

// structures
struct imem_input_t {
	uint32_t addr;
};

struct imem_output_t {
	uint32_t dout;
};

// structures for pipeline registers
struct pipe_if_id_t {
	uint32_t pc;
	uint32_t inst;
};

struct pipe_id_ex_t {
	uint32_t pc;
	uint32_t rs1_dout;
	uint32_t rs2_dout;
	uint32_t imm32;
	uint32_t funct3;
	uint32_t funct7;
	uint32_t branch;
	uint32_t alu_src;
	uint32_t alu_op;
	uint32_t mem_read;
	uint32_t mem_write;
	uint32_t rs1;
	uint32_t rs2;
	uint32_t rd;
	uint32_t reg_write;
	uint32_t mem_to_reg;
	uint32_t lui;
	uint32_t auipc;
	uint32_t jal;
	uint32_t jalr;
};

struct pipe_ex_mem_t {
	uint32_t alu_result;
	uint32_t rs2_dout;
	uint32_t funct3;
	uint32_t mem_read;
	uint32_t mem_write;
	uint32_t rd;
	uint32_t reg_write;
	uint32_t mem_to_reg;
};

struct pipe_mem_wb_t {
	uint32_t alu_result;
	uint32_t dmem_dout;
	uint32_t rd;
	uint32_t reg_write;
	uint32_t mem_to_reg;
};