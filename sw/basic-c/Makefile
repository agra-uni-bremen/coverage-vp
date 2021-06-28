all : main.c symbolic.c bootstrap.S
	riscv32-unknown-elf-gcc main.c symbolic.c bootstrap.S -o main -g3 -march=rv32i -mabi=ilp32 -nostartfiles -Wl,--no-relax
	
sim: all
	riscv-vp --intercept-syscalls main
	
dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
dump-comment: all
	objdump -s --section .comment main
	
clean:
	rm -f main
