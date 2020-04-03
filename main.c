#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#define CPU_DIAG
#define CPU_DIAG_OFFSET 0x100
FILE *file;
extern uint8_t memory[65536];
int main(int argc, char **argv) { 
	file = fopen(argv[1], "rb");
	if (file == NULL){
		perror("Failed: ");
		return -1;
	}
	fseek(file, 0, SEEK_END);
	long filelen = ftell(file);
	rewind(file);
	#ifdef CPU_DIAG
	fread(&memory[CPU_DIAG_OFFSET], 1, filelen, file);
	#else
	fread(&memory, 1, filelen, file);
	#endif
	fclose(file);
	tick();
	return 1;
}