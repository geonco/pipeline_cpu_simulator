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

};

struct pipe_id_ex_t {
	
};

struct pipe_ex_mem_t {

};

struct pipe_mem_wb_t {

};