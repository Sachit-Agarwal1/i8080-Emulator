#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#define CPU_DIAG
/* CPU Core Emulator for i8080 */
/* Register Pairs:
	B : B and C
	D : D and E
	H : H and L
*/
/* FLAGS:
	C : Carry
	AC : Auxillary Carry
	S : Sign Bit
	Z : Zero Bit
	P : Parity Bit
*/
int cycleCount;

uint8_t B;
uint8_t C;
uint8_t D;
uint8_t E;
uint8_t H;
uint8_t L;
uint8_t A;
#ifdef CPU_DIAG
uint16_t programCounter = 0x100;
#else
uint16_t programCounter = 0x000;
#endif
uint16_t stackPointer;
bool isCPURunning;
bool interruptsEnabled;
bool carryFlag;
bool auxCarryFlag;
bool signFlag;
bool zeroFlag;
bool parityFlag;

uint32_t temp32;
uint16_t temp16;
uint8_t temp8;


uint8_t memory[65536];

int parity(uint8_t byte)
{
	int y = byte ^ (byte >> 1);
	y = y ^ (y >> 2);
	y = y ^ (y >> 4);
	y = y ^ (y >> 8);
	return !(y & 1);
}

static inline void MOV(uint8_t *dest, uint8_t *src) {
	*dest = *src;
	programCounter++;
	cycleCount += 7;
}
static inline void MVI(uint8_t *dest) {
	*dest = memory[programCounter + 1];
	programCounter += 2;
	cycleCount += 7;
}
static inline void LXI(uint8_t *hiReg, uint8_t *lowReg) {
	*lowReg = memory[programCounter + 1];
	*hiReg = memory[programCounter + 2];
	programCounter += 3;
	cycleCount += 10;
}
static inline void ADD(uint8_t *src){
	carryFlag = (A + *src) > 255;
	A = A + *src;
	zeroFlag = (A == 0);
	parityFlag = parity(A);
	signFlag = (A >> 7);
	programCounter += 1;
	cycleCount += 4;
}
static inline void ADC(uint8_t *src){
	temp16 = A + *src + carryFlag;
	carryFlag = (temp16 > 0xFF);
	A = temp16 & 0xFF;
	zeroFlag = (A==0);
	parityFlag = parity(A);
	signFlag = (A >> 7);
	programCounter += 1;
	cycleCount += 4;
}
static inline void SUB(uint8_t *src){
	carryFlag = *src > A;
	A = A - *src;
	zeroFlag = (A==0);
	parityFlag = parity(A);
	signFlag = (A >> 7);
	programCounter += 1;
	cycleCount += 4;
}
static inline void SBB(uint8_t *src){
	temp8 = A - (*src + carryFlag);
	carryFlag = (*src + carryFlag) > A;
	A = temp8;
	zeroFlag = (A==0);
	parityFlag = parity(A);
	signFlag = (A >> 7);
	programCounter += 1;
	cycleCount += 4;
}
static inline void INR(uint8_t *src){
	*src += 1;
	zeroFlag = (*src == 0);
	signFlag = (*src >> 7);
	parityFlag = parity(*src);
	cycleCount += 5;
	programCounter += 1;
}
static inline void DCR(uint8_t *src){
	*src -= 1;
	zeroFlag = (*src == 0);
	signFlag = (*src >> 7);
	parityFlag = parity(*src);
	cycleCount += 5;
	programCounter += 1;
}
static void inline CMP(uint8_t *src){
	carryFlag = (*src > A);
	temp8 = *src - A;
	zeroFlag = (temp8 == 0);
	parityFlag = parity(temp8);
	signFlag = temp8 >> 7;
	cycleCount += 4;
	programCounter += 1;
}
static inline uint16_t make16(uint8_t hiReg, uint8_t lowReg)
{
	return (hiReg << 8) | lowReg;
}
void tick()
{
	uint8_t opcode;
	isCPURunning = true;
	while (isCPURunning)
	{
		opcode = memory[programCounter];
		printf("OPCODE:%x\n", opcode);
		printf("Program Counter:%x\n", programCounter);
		switch (opcode)
		{
		case 0x00:
			/*NOP*/
			cycleCount+=4;
			programCounter++;
			break;
		case 0x01:
			/*LXI B, D16*/
			B = memory[programCounter + 2];
			C = memory[programCounter + 1];
			cycleCount += 10;
			programCounter += 3;
			break;
		case 0x02:
			/*STAX B*/
			memory[make16(B, C)] = A;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x03:
			/*INX B*/
			temp16 = (make16(B, C)) + 1;
			B = temp16 >> 8;
			C = temp16 & 0x00FF;
			programCounter++;
			cycleCount += 5;
			break;
		case 0x04:
			/*INR B*/
			INR(&B);
			break;
		case 0x05:
			/*DCR B*/
			DCR(&B);
			break;
		case 0x06:
			/*MVI B,D8*/
			MVI(&B);
			break;
		case 0x07:
			/*RLC*/
			/*FLAGS: C*/
			carryFlag = A >> 7;
			A = (A << 1) | carryFlag;
			programCounter++;
			cycleCount += 4;
			break;
		case 0x09:
			/*DAD B*/
			/*FLAGS: C*/
			/*TODO*/
			temp32 = (make16(B, C)) + (make16(H, L));
			carryFlag = (temp32 > 65535);
			temp32 = temp32 & 0xFFFF;
			H = temp32 >> 8;
			L = temp32 & 0x00FF;
			programCounter++;
			cycleCount += 10;
			break;
		case 0x0A:
			/*LDAX B*/
			A = memory[make16(B, C)];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x0B:
			/*DCX B*/
			temp16 = (make16(B, C)) - 1;
			B = (temp16 >> 8);
			C = temp16 & 0xFF;
			programCounter++;
			cycleCount += 5;
			break;
		case 0x0C:
			/*INR C*/
			INR(&C);
			break;
		case 0x0D:
			/*DCR C*/
			DCR(&C);
			break;
		case 0x0E:
			/*MVI C, D8*/
			MVI(&C);
			break;
		case 0x0F:
			/*RRC CY*/
			/*FLAGS: C*/
			carryFlag = A & 0x01;
			A = (A >> 1) | (carryFlag << 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0x10:
			/*NOP*/
			cycleCount += 4;
			break;
		case 0x11:
			/*LXI D, D16 - double check*/
			E = memory[programCounter+1];
			D = memory[programCounter+2];
			programCounter += 3;
			cycleCount += 10;
			break;
		case 0x12:
			/*STAX D*/
			memory[make16(D, E)] = A;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x13:
			/*INX D*/
			temp16 = (make16(D, E)) + 1;
			D = temp16 >> 8;
			E = temp16 & 0x00FF;
			programCounter++;
			cycleCount += 5;
			break;
		case 0x14:
			/*INR D*/
			INR(&D);
			break;
		case 0x15:
			/*DCR D*/
			DCR(&D);
			break;
		case 0x16:
			/*MVI D, D8*/
			MVI(&D);
			break;
		case 0x17:
			/*RAL*/
			temp8 = A << 1;
			temp8 = temp8 | carryFlag;
			carryFlag = A >> 7;
			A = temp8;
			cycleCount += 4;
			programCounter++;
			break;
		case 0x19:
			/*DAD D*/
			/*FLAGS: C*/
			temp32 = (make16(D, E)) + (make16(H, L));
			carryFlag = (temp32 > 0xFFFF);
			temp32 = temp32 & 0xFFFF;
			H = temp32 >> 8;
			L = temp32 & 0x00FF;
			cycleCount += 10;
			programCounter++;
			break;
		case 0x1A:
			/*LDAX D*/
			A = memory[make16(D, E)];
			cycleCount += 10;
			programCounter += 1;
			break;
		case 0x1B:
			/*DCX D*/
			temp16 = make16(D, E) - 1;
			D = (temp16 >> 8);
			E = temp16 & 0xFF;
			programCounter++;
			cycleCount += 5;
			break;
		case 0x1C:
			/*INR E*/
			/*FLAGS: S Z AC P*/
			INR(&E);
			break;
		case 0x1D:
			/*DCR E*/
			/*FLAGS: S Z AC P*/
			DCR(&E);
			break;
		case 0x1E:
			/*MVI, E, d8*/
			MVI(&E);
			break;
		case 0x1F:
			/*RAR*/
			temp8 = A >> 1;
			temp8 = temp8 | (carryFlag << 7);
			carryFlag = A & 1;
			A = temp8;
			cycleCount += 4;
			programCounter++;
			break;
		case 0x20:
			/*NOP*/
		case 0x21:
			/*LXI H, d16*/
			L = memory[programCounter+1];
			H = memory[programCounter+2];
			programCounter += 3;
			cycleCount += 10;
			break;
		case 0x22:
			/*SHLD a16*/
			temp16 = (memory[programCounter+2] << 8) | memory[programCounter+1];
			memory[temp16] = L;
			memory[temp16 + 1] = H;
			programCounter = programCounter + 3;
			cycleCount += 16;
			break;
		case 0x23:
			/*INX H*/
			temp16 = (make16(H, L)) + 1;
			H = temp16 >> 8;
			L = temp16 &0x00FF;
			programCounter++;
			break;
		case 0x24:
			/*INR H*/
			/*FLAGS: S Z AC P*/
			INR(&H);
			break;
		case 0x25:
			/*DCR H*/
			/*FLAGS: S Z AC P*/
			DCR(&H);
			break;
		case 0x26:
			/*MVI, H, d8*/
			MVI(&H);
			break;
		case 0x27:
			/*DAA*/
			/*FLAGS: S Z AC P C*/
			//unfinished
			break;
			;
		case 0x28:
			/*NOP*/
			break;
			;
		case 0x29:
			/*DAD H*/
			/*FLAGS: C*/
			temp32 = (make16(H, L)) << 1;
			carryFlag = (temp32 > 0xFFFF);
			temp32 = temp32 & 0xFFFF;
			H = temp32 >> 8;
			L = temp32 & 0x00FF;
			cycleCount += 10;
			programCounter++;
			break;
		case 0x2A:
			/*LHLD a16*/
			temp16 = (memory[programCounter+2] << 8) | memory[programCounter+1];
			H = memory[temp16+1];
			L = memory[temp16];
			programCounter = programCounter + 3;
			cycleCount += 16;
			break;
		case 0x2B:
			/*DCX H*/
			temp16 = (make16(H, L)) - 1;
			H = temp16 >> 8;
			L = temp16 & 0x00FF;
			programCounter++;
			break;
		case 0x2C:
			/*INR L*/
			/*FLAGS: S Z AC P*/
			INR(&L);
			break;
		case 0x2D:
			/*DCR L*/
			/*FLAGS: S Z AC P*/
			DCR(&L);
			break;
		case 0x2E:
			/*MVI  L, d8*/
			MVI(&L);
			break;
		case 0x2F:
			/*CMA*/
			A = ~A;
			programCounter++;
			cycleCount += 4;
			break;
		case 0x30:
			/*dummy OP*/
			/*dump processor state*/
			printf("Accumulator Value: %x\n", A);
			printf("B and C Values: %x %x\n", B, C);
			printf("D and E Values: %x %x\n", D, E);
			printf("H and L Values: %x %x\n", H, L);
			printf("Carry:%d\nSign:%d\nZero:%d\nParity:%d\n", carryFlag, signFlag, zeroFlag, parityFlag);
			cycleCount += 4;
			programCounter += 1;
			break;
		case 0x31:
			/*LXI SP, d16*/
			stackPointer = (memory[programCounter + 2] << 8) | memory[programCounter+1];
			programCounter = programCounter + 3;
			cycleCount += 10;
			break;
		case 0x32:
			/*STA a16*/
			temp16 = (memory[programCounter+2] << 8) | memory[programCounter+1];
			memory[temp16] = A;
			programCounter = programCounter + 3;
			cycleCount += 13;
			break;
		case 0x33:
			/*INX SP*/
			stackPointer++;
			cycleCount += 5;
			programCounter++;
			break;
		case 0x34:
			/*INR M*/
			/*FLAGS: S Z AC P*/
			INR(&memory[make16(H,L)]);
			cycleCount += 5;
			break;
		case 0x35:
			/*DCR M*/
			/*FLAGS: S Z AC P*/
			DCR(&memory[make16(H,L)]);
			cycleCount += 5;
			break;
		case 0x36:
			/*MVI M, d8*/
			temp16 = make16(H, L);
			memory[temp16] = memory[programCounter + 1];
			programCounter += 2;
			cycleCount += 10;
			break;
		case 0x37:
			/*STC*/
			/*FLAGS: C*/
			carryFlag = 1;
			programCounter++;
			cycleCount += 4;
			break;
		case 0x38:
			/*NOP*/
			cycleCount += 4;
			programCounter += 1;
			break;
		case 0x39:
			/*DAD SP*/
			/*FLAGS: C*/
			temp32 = make16(H,L) + stackPointer;
			carryFlag = (temp32 > 0xFFFF);
			temp32 = temp32 % 65536;
			H = temp32 >> 8;
			L = temp32 & 0x00FF;
			cycleCount += 10;
			programCounter++;
			break;
		case 0x3A:
			/*LDA a16*/
			temp16 = (memory[programCounter + 2] << 8) | memory[programCounter+1];
			A = memory[temp16];
			programCounter = programCounter + 3;
			cycleCount += 13;
			break;
		case 0x3B:
			/*DCX SP*/
			stackPointer--;
			cycleCount += 5;
			programCounter++;
			break;
		case 0x3C:
			/*INR A*/
			/*FLAGS: S Z AC P*/
			INR(&A);
			break;
		case 0x3D:
			/*DCR A*/
			/*FLAGS: S Z AC P*/
			DCR(&A);
			break;
		case 0x3E:
			/*MVI A, d8*/
			MVI(&A);
			break;
		case 0x3F:
			/*CMC*/
			/*FLAGS: C*/
			carryFlag = !carryFlag;
			cycleCount += 4;
			programCounter += 1;
			break;
		/*MOVE OPCODES*/
		case 0x40:
			MOV(&B, &B);
			break;
		case 0x41:
			MOV(&B, &C);
			break;
		case 0x42:
			MOV(&B, &D);
			break;
		case 0x43:
			MOV(&B, &E);
			break;
		case 0x44:
			MOV(&B, &H);
			break;
		case 0x45:
			MOV(&B, &L);
			break;
		case 0x46:
			/*MOV B, M*/
			temp16 = make16(H, L);
			B = memory[temp16];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x47:
			MOV(&B, &A);
			break;
		case 0x48:
			MOV(&C, &B);
			break;
		case 0x49:
			MOV(&C, &C);
			break;
		case 0x4A:
			MOV(&C, &D);
			break;
		case 0x4B:
			MOV(&C, &E);
			break;
		case 0x4C:
			MOV(&C, &H);
			break;
		case 0x4D:
			MOV(&C, &L);
			break;
		case 0x4E:
			/*MOV C, M*/
			temp16 = make16(H, L);
			C = memory[temp16];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x4F:
			MOV(&C, &A);
			break;
		case 0x50:
			MOV(&D, &B);
			break;
		case 0x51:
			MOV(&D, &C);
			break;
		case 0x52:
			MOV(&D, &D);
			break;
		case 0x53:
			MOV(&D, &E);
			break;
		case 0x54:
			MOV(&D, &H);
			break;
		case 0x55:
			MOV(&D, &L);
			break;
		case 0x56:
			/*MOV D, M*/
			temp16 = make16(H, L);
			D = memory[temp16];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x57:
			MOV(&D, &A);
			break;
		case 0x58:
			MOV(&E, &B);
			break;
		case 0x59:
			MOV(&E, &C);
			break;
		case 0x5A:
			MOV(&E, &D);
			break;
		case 0x5B:
			MOV(&E, &E);
			break;
		case 0x5C:
			MOV(&E, &H);
			break;
		case 0x5D:
			MOV(&E, &L);
			break;
		case 0x5E:
			/*MOV E, M*/
			temp16 = make16(H, L);
			E = memory[temp16];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x5F:
			MOV(&E, &A);
			break;
		case 0x60:
			MOV(&H, &B);
			break;
		case 0x61:
			MOV(&H, &C);
			break;
		case 0x62:
			MOV(&H, &D);
			break;
		case 0x63:
			MOV(&H, &E);
			break;
		case 0x64:
			MOV(&H, &H);
			break;
		case 0x65:
			MOV(&H, &L);
			break;
		case 0x66:
			/*MOV H, M*/
			temp16 = make16(H, L);
			H = memory[temp16];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x67:
			MOV(&H, &A);
			break;
		case 0x68:
			MOV(&L, &B);
			break;
		case 0x69:
			MOV(&L, &C);
			break;
		case 0x6A:
			MOV(&L, &D);
			break;
		case 0x6B:
			MOV(&L, &E);
			break;
		case 0x6C:
			MOV(&L, &H);
			break;
		case 0x6D:
			MOV(&L, &L);
			break;
		case 0x6E:
			/*MOV L, M*/
			temp16 = make16(H, L);
			L = memory[temp16];
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x6F:
			MOV(&L, &A);
			break;
		/*MEMORY MOVE OPCODES*/
		case 0x70:
			/*MOV M, B*/
			temp16 = make16(H, L);
			memory[temp16] = B;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x71:
			/*MOV M, C*/
			temp16 = make16(H, L);
			memory[temp16] = C;
			programCounter++;
			cycleCount += 7;
			break;
		case 0x72:
			/*MOV M, D*/
			temp16 = make16(H, L);
			memory[temp16] = D;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x73:
			/*MOV M, E*/
			temp16 = make16(H, L);
			memory[temp16] = E;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x74:
			/*MOV M, H*/
			temp16 = make16(H, L);
			memory[temp16] = H;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x75:
			/*MOV M, L*/
			temp16 = make16(H, L);
			memory[temp16] = L;
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0x76:
			/*HLT*/
			isCPURunning = false;
			cycleCount += 7;
			programCounter++;
			break;
		case 0x77:
			/*MOV M, A*/
			temp16 = make16(H, L);
			memory[temp16] = A;
			programCounter++;
			cycleCount += 7;
			break;
		case 0x78:
			MOV(&A, &B);
			break;
		case 0x79:
			MOV(&A, &C);
			break;
		case 0x7A:
			MOV(&A, &D);
			break;
		case 0x7B:
			MOV(&A, &E);
			break;
		case 0x7C:
			MOV(&A, &H);
			break;
		case 0x7D:
			MOV(&A, &L);
			break;
		case 0x7E:
			/*MOV A, M*/
			temp16 = make16(H, L);
			A = memory[temp16];
			programCounter++;
			cycleCount += 7;
			break;
		case 0x7F:
			MOV(&A, &A);
			break;
		case 0x80:
			/*ADD B*/
			/*FLAGS: S Z AC P C*/
			ADD(&B);
			break;
		case 0x81:
			/*ADD C*/
			/*FLAGS: S Z AC P C*/
			ADD(&C);
			break;
		case 0x82:
			/*ADD D*/
			/*FLAGS: S Z AC P C*/
			ADD(&D);
			break;
		case 0x83:
			/*ADD E*/
			/*FLAGS: S Z AC P C*/
			ADD(&E);
			break;
		case 0x84:
			/*ADD H*/
			/*FLAGS: S Z AC P C*/
			ADD(&H);
			break;
		case 0x85:
			/*ADD L*/
			/*FLAGS: S Z AC P C*/
			ADD(&L);
			break;
		case 0x86:
			/*ADD M*/
			/*FLAGS: S Z AC P C*/
			ADD(&memory[make16(H, L)]);
			cycleCount += 3;
			break;
		case 0x87:
			/*ADD A*/
			/*FLAGS: S Z AC P C*/
			ADD(&A);
			break;
		case 0x88:
			/*ADC B*/
			/*FLAGS: S Z AC P C*/
			ADC(&B);
			break;
		case 0x89:
			/*ADC C*/
			/*FLAGS: S Z AC P C*/
			ADC(&C);
			break;
		case 0x8A:
			/*ADC D*/
			/*FLAGS: S Z AC P C*/
			ADC(&D);
			break;
		case 0x8B:
			/*ADC E*/
			/*FLAGS: S Z AC P C*/
			ADC(&E);
			break;
		case 0x8C:
			/*ADC H*/
			/*FLAGS: S Z AC P C*/
			ADC(&H);
			break;
		case 0x8D:
			/*ADC L*/
			/*FLAGS: S Z AC P C*/
			ADC(&L);
			break;
		case 0x8E:
			/*ADC M*/
			/*FLAGS: S Z AC P C*/
			ADC(&memory[make16(H, L)]);
			cycleCount += 3;
			break;
		case 0x8F:
			/*ADC A*/
			/*FLAGS: S Z AC P C*/
			ADC(&A);
			break;
		case 0x90:
			/*SUB B*/
			/*FLAGS: S Z AC P C*/
			SUB(&B);
			break;
		case 0x91:
			/*SUB C*/
			/*FLAGS: S Z AC P C*/
			SUB(&C);
			break;
		case 0x92:
			/*SUB D*/
			/*FLAGS: S Z AC P C*/
			SUB(&D);
			break;
		case 0x93:
			/*SUB E*/
			/*FLAGS: S Z AC P C*/
			SUB(&E);
			break;
		case 0x94:
			/*SUB H*/
			/*FLAGS: S Z AC P C*/
			SUB(&H);
			break;
		case 0x95:
			/*SUB L*/
			/*FLAGS: S Z AC P C*/
			SUB(&L);
			break;
		case 0x96:
			/*SUB M*/
			/*FLAGS: S Z AC P C*/
			SUB(&memory[make16(H, L)]);
			cycleCount += 3;
			break;
		case 0x97:
			/*SUB A*/
			/*FLAGS: S Z AC P C*/
			SUB(&A);
			break;
		case 0x98:
			/*SBB B*/
			/*FLAGS: S Z AC P C*/
			SBB(&B);
			break;
		case 0x99:
			/*SBB C*/
			/*FLAGS: S Z AC P C*/
			SBB(&C);
			break;
		case 0x9A:
			/*SBB D*/
			/*FLAGS: S Z AC P C*/
			SBB(&D);
			break;
		case 0x9B:
			/*SBB E*/
			/*FLAGS: S Z AC P C*/
			SBB(&E);
			break;
		case 0x9C:
			/*SBB H*/
			/*FLAGS: S Z AC P C*/
			SBB(&H);
			break;
		case 0x9D:
			/*SBB L*/
			/*FLAGS: S Z AC P C*/
			SBB(&L);
			break;
		case 0x9E:
			/*SBB M*/
			/*FLAGS: S Z AC P C*/
			SBB(&memory[make16(H,L)]);
			cycleCount += 3;
			break;
		case 0x9F:
			/*SBB A*/
			/*FLAGS: S Z AC P C*/
			SBB(&A);
			break;
		case 0xA0:
			/*ANA B*/
			/*FLAGS: S Z AC P C*/
			A = A & B;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA1:
			/*ANA C*/
			/*FLAGS: S Z AC P C*/
			A = A & C;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA2:
			/*ANA D*/
			/*FLAGS: S Z AC P C*/
			A = A & D;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA3:
			/*ANA E*/
			/*FLAGS: S Z AC P C*/
			A = A & E;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA4:
			/*ANA H*/
			/*FLAGS: S Z AC P C*/
			A = A & H;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA5:
			/*ANA L*/
			/*FLAGS: S Z AC P C*/
			A = A & L;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA6:
			/*ANA M*/
			/*FLAGS: S Z AC P C*/
			A = A & memory[make16(H,L)];
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 7;
			break;
		case 0xA7:
			/*ANA A*/
			/*FLAGS: S Z AC P C*/
			A = A & A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA8:
			/*XRA B*/
			/*FLAGS: S Z AC P C*/
			A = A ^ B;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xA9:
			/*XRA C*/
			/*FLAGS: S Z AC P C*/
			A = A ^ C;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xAA:
			/*XRA D*/
			/*FLAGS: S Z AC P C*/
			A = A ^ D;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xAB:
			/*XRA E*/
			/*FLAGS: S Z AC P C*/
			A = A ^ E;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xAC:
			/*XRA H*/
			/*FLAGS: S Z AC P C*/
			A = A ^ H;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xAD:
			/*XRA l*/
			/*FLAGS: S Z AC P C*/
			A = A ^ L;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 4;
			break;
		case 0xAE:
			/*XRA M*/
			/*FLAGS: S Z AC P C*/
			A = A ^ memory[make16(H,L)];
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter++;
			cycleCount += 7;
			break;
		case 0xAF:
			/*XRA A*/
			/*FLAGS: S Z AC P C*/
			A = A ^ A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB0:
			/*ORA B*/
			/*FLAGS: S Z AC P C*/
			A = B | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB1:
			/*ORA C*/
			/*FLAGS: S Z AC P C*/
			A = C | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB2:
			/*ORA D*/
			/*FLAGS: S Z AC P C*/
			A = D | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB3:
			/*ORA E*/
			/*FLAGS: S Z AC P C*/
			A = E | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB4:
			/*ORA H*/
			/*FLAGS: S Z AC P C*/
			A = H | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB5:
			/*ORA L*/
			/*FLAGS: S Z AC P C*/
			A = L | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB6:
			/*ORA M*/
			/*FLAGS: S Z AC P C*/
			A = A | memory[make16(H, L)];
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 7;
			break;
		case 0xB7:
			/*ORA A*/
			/*FLAGS: S Z AC P C*/
			A = A | A;
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = (A >> 7);
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xB8:
			/*CMP B*/
			/*FLAGS: S Z AC P C*/
			CMP(&B);
			break;
		case 0xB9:
			/*CMP C*/
			CMP(&C);
			break;
		case 0xBA:
			/*CMP D*/
			CMP(&D);
			break;
		case 0xBB:
			/*CMP E*/
			CMP(&E);
			break;
		case 0xBC:
			/*CMP H*/
			CMP(&H);
			break;
		case 0xBD:
			/*CMP L*/
			CMP(&L);
			break;
		case 0xBE:
			/*CMP M*/
			CMP(&memory[make16(H, L)]);
			cycleCount += 3;
			break;
		case 0xBF:
			/*CMP A*/
			zeroFlag = 0;
			carryFlag = 0;
			signFlag = 0;
			parityFlag = 0;
			cycleCount = 4;
			break;
		case 0xC0:
			/*RNZ*/
			if (!zeroFlag){
				programCounter = memory[stackPointer];
				programCounter = programCounter | (memory[stackPointer+1] << 8);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xC1:
			/*POP B*/
			C = memory[stackPointer];
			B = memory[stackPointer+1];
			stackPointer += 2;
			cycleCount += 10;
			programCounter += 1;
			break;
		case 0xC2:
			/*JNZ a16*/
			if (!zeroFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xC3:
			/*JMP Unconditional*/
			programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			cycleCount += 10;
			break;
		case 0xC4:
			/*CNZ a16*/
			if (!zeroFlag){
				memory[stackPointer - 1] = ((programCounter+3) >> 8);
				memory[stackPointer - 2] = ((programCounter+3) & 255);
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter + 2], memory[programCounter + 1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xC5:
			/*PUSH B*/
			memory[stackPointer - 1] = B;
			memory[stackPointer - 2] = C;
			stackPointer = stackPointer - 2;
			programCounter += 1;
			cycleCount += 11;
			break;
		case 0xC6:
			/*ADI*/
			/*FLAGS: S Z AC P C*/
			temp16 = A + memory[programCounter + 1];
			A = temp16 & 0xFF;
			signFlag = A >> 7;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			carryFlag = temp16 > 255;
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xC7:
			/*RST 0*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 0;
			cycleCount += 11;
			break;
		case 0xC8:
			/*RZ*/
			if (zeroFlag){
				programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xC9:
			/*RET*/
			programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
			stackPointer = stackPointer + 2;
			cycleCount += 10;
			break;
		case 0xCA:
			/*JZ a16*/
			if (zeroFlag){
				programCounter = make16(memory[programCounter + 2], memory[programCounter + 1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xCB:
			/* *JMP a16*/
			programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			cycleCount += 10;
			break;
		case 0xCC:
			/*CZ a16*/
			if (zeroFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xCD:
			/*CALL a16*/
			memory[stackPointer - 1] = (programCounter+3) >> 8;
			memory[stackPointer - 2] = (programCounter+3) & 255;
			stackPointer = stackPointer - 2;
			programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			cycleCount += 17;
			break;
		case 0xCE:
			/*ACI d8*/
			temp16 = A + memory[programCounter+1] + carryFlag;
			A = temp16 & 0xFF;
			zeroFlag = (A == 0);
			signFlag = A >> 7;
			parityFlag = parity(A);
			carryFlag = temp16 > 255;
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xCF:
			/*RST 1*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 8;
			cycleCount += 11;
			break;
		case 0xD0:
			/*RNC*/
			if (!carryFlag){
				programCounter = memory[stackPointer];
				programCounter = programCounter | (memory[stackPointer+1] << 8);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xD1:
			/*POP D*/
			E = memory[stackPointer];
			D = memory[stackPointer + 1];
			stackPointer += 2;
			cycleCount += 10;
			programCounter += 1;
			break;
		case 0xD2:
			/*JNC a16*/
			if (!carryFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xD3:
			/*OUT d8* TODO*/
			programCounter += 2;
			cycleCount += 10;
			break;
		case 0xD4:
			/*CNC a16*/
			if (!carryFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else {
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xD5:
			/*PUSH D*/
			memory[stackPointer - 1] = D;
			memory[stackPointer - 2] = E;
			stackPointer = stackPointer - 2;
			cycleCount += 11;
			programCounter += 1;
			break;
		case 0xD6:
			/*SUI d8*/
			/*FLAGS: S Z AC P C*/
			temp8 = memory[programCounter + 1];
			carryFlag = (temp8 > A);
			A = A - temp8;
			signFlag = (A >> 7);
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xD7:
			/*RST 2*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 16;
			cycleCount += 11;
			break;
		case 0xD8:
			/*RC*/
			if (carryFlag){
				programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xD9:
			/* *RET */
			programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
			stackPointer = stackPointer + 2;
			cycleCount += 10;
			break;
		case 0xDA:
			/*JC a16*/
			if (carryFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xDB:
			/*IN d8 TODO*/
			cycleCount += 10;
			programCounter += 2;
			break;
		case 0xDC:
			/*CC a16*/
			if (carryFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xDD:
			/* *CALL*/
			memory[stackPointer - 1] = programCounter >> 8;
			memory[stackPointer - 2] = programCounter & 255;
			stackPointer = stackPointer - 2;
			programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			cycleCount += 17;
			break;
		case 0xDE:
			/*SBI d8 TODO*/
			SBB(&memory[programCounter + 1]);
			cycleCount += 3;
			programCounter += 1;
			break;
		case 0xDF:
			/*RST 3*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 24;
			cycleCount += 11;
			break;
		case 0xE0:
			/*RPO*/
			if (!parityFlag){
				programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xE1:
			/*POP H*/
			L = memory[stackPointer];
			H = memory[stackPointer + 1];
			stackPointer += 2;
			cycleCount += 10;
			programCounter += 1;
			break;
		case 0xE2:
			/*JPO*/
			if(!parityFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xE3:
			/*XTHL*/
			temp8 = memory[stackPointer];
			memory[stackPointer] = L;
			L = temp8;
			temp8 = memory[stackPointer+1];
			memory[stackPointer+1] = H;
			H = temp8;
			cycleCount += 18;
			programCounter += 1;
			break;
		case 0xE4:
			/*CPO a16*/
			if (!parityFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xE5:
			/*PUSH H*/
			memory[stackPointer - 1] = H;
			memory[stackPointer - 2] = L;
			stackPointer = stackPointer - 2;
			cycleCount += 11;
			programCounter += 1;
			break;
		case 0xE6:
			/*ANI d8*/
			A = A & memory[programCounter + 1];
			zeroFlag = (A == 0);
			signFlag = (A >> 7);
			parityFlag = parity(A);
			carryFlag = 0;
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xE7:
			/*RST 4*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 32;
			cycleCount += 11;
			break;
		case 0xE8:
			/*RPE*/
			if (parityFlag){
				programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xE9:
			/*PCHL*/
			programCounter = (H << 8) | L;
			cycleCount += 5;
			break;
		case 0xEA:
			/*Its in the game*/
			/*JPE a16*/
			if (parityFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xEB:
			/*XCHG*/
			temp8 = H;
			H = D;
			D = temp8;
			temp8 = L;
			L = E;
			E = temp8;
			cycleCount += 5;
			programCounter += 1;
			break;
		case 0xEC:
			/*CPE a16*/
			if (parityFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xED:
			/* *CALL */
			memory[stackPointer - 1] = programCounter >> 8;
			memory[stackPointer - 2] = programCounter & 255;
			stackPointer = stackPointer - 2;
			programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			cycleCount += 17;
			break;
		case 0xEE:
			/*XRI d8*/
			A = A ^ memory[programCounter + 1];
			carryFlag = 0;
			zeroFlag = (A == 0);
			signFlag = (A >> 7);
			parityFlag = parity(A);
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xEF:
			/*RST 5*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 40;
			cycleCount += 11;
			break;
		case 0xF0:
			/*RP*/
			if (!signFlag){
				programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xF1:
			/*POP PSW TEST*/
			A = memory[stackPointer + 1];
			temp8 = memory[stackPointer];
			carryFlag = temp8 & 1;
			parityFlag = (temp8 & 0b100) >> 2;
			auxCarryFlag = (temp8 & 0b10000) >> 4;
			zeroFlag = (temp8 & 0b1000000) >> 6;
			signFlag = (temp8 & 0b10000000) >> 7;
			cycleCount += 10;
			programCounter += 1;
			stackPointer += 2;
			break;
		case 0xF2:
			/*JP a16*/
			if (!signFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else{
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xF3:
			/*DI*/
			interruptsEnabled = false;
			cycleCount += 4;
			programCounter += 1;
			break;
		case 0xF4:
			/*CP a16*/
			if (!signFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xF5:
			/*PUSH PSW TEST*/
			memory[stackPointer - 1] = A;
			temp8 = 0b00000010 | (carryFlag) | (parityFlag << 2) | (auxCarryFlag << 4) | (zeroFlag << 6) | (signFlag << 7);
			memory[stackPointer - 2] = temp8;
			stackPointer = stackPointer - 2;
			cycleCount += 11;
			programCounter += 1;
			break;
		case 0xF6:
			/*ORI d8*/
			A = A | memory[programCounter + 1];
			carryFlag = 0;
			zeroFlag = (A == 0);
			parityFlag = parity(A);
			signFlag = ( A >> 7);
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xF7:
			/*RST 6*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 48;
			cycleCount += 11;
			break;
		case 0xF8:
			/*RM*/
			if (signFlag){
				programCounter = make16(memory[stackPointer + 1], memory[stackPointer]);
				stackPointer = stackPointer + 2;
				cycleCount += 11;
			}
			else {
				programCounter += 1;
				cycleCount += 5;
			}
			break;
		case 0xF9:
			/*SPHL*/
			stackPointer = (H << 8) | L;
			cycleCount += 5;
			programCounter += 1;
			break;
		case 0xFA:
			/*JM a16*/
			if (signFlag){
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			}
			else {
				programCounter += 3;
			}
			cycleCount += 10;
			break;
		case 0xFB:
			/*EI*/
			interruptsEnabled = true;
			programCounter += 1;
			cycleCount += 4;
			break;
		case 0xFC:
			/*CM a16*/
			if (signFlag){
				memory[stackPointer - 1] = (programCounter+3) >> 8;
				memory[stackPointer - 2] = (programCounter+3) & 255;
				stackPointer = stackPointer - 2;
				programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
				cycleCount += 17;
			}
			else{
				programCounter += 3;
				cycleCount += 11;
			}
			break;
		case 0xFD:
			/*CALL*/
			memory[stackPointer - 1] = programCounter >> 8;
			memory[stackPointer - 2] = programCounter & 255;
			stackPointer = stackPointer - 2;
			programCounter = make16(memory[programCounter+2], memory[programCounter+1]);
			break;
		case 0xFE:
			/*CPI d8*/
			temp8 = A - memory[programCounter + 1];
			zeroFlag = (temp8 == 0);
			signFlag = (temp8) >> 7;
			carryFlag = (A < temp8);
			parityFlag = parity(temp8);
			programCounter += 2;
			cycleCount += 7;
			break;
		case 0xFF:
			/*RST 7*/
			memory[stackPointer - 1] = (programCounter+1) >> 8;
			memory[stackPointer - 2] = (programCounter+1) & 255;
			stackPointer = stackPointer - 2;
			programCounter = 56;
			cycleCount += 11;
			break;
		default: printf("Unimplemented OPCODE"); return;
	}
}	
}