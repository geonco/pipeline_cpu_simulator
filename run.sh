set -e

gcc -Wall -o rv32i_pipe rv32i_pipe.c

# instruction coverage test
./rv32i_pipe imem_test.mem dmem_test.mem report_test.txt

# find-max program
./rv32i_pipe imem_findmax.mem dmem_findmax.mem report_findmax.txt
