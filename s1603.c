#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdbool.h>
#include<SDL.h>

//reg and memory I/O port
#define SIZE 0xFFFF
#define REG 6
#define EXH 7
#define EXL 8
#define VRAM_S		0xF000
#define VRAM_F		0xFFFC
#define O_PORT_FLAG 0xFFFD
#define O_PORT		0xFFFE
#define I_PORT		0xFFFF

//GPU
#define WINDOW_WIDTH	640
#define WINDOW_HEIGHT	480

//opecode
#define HLT		0b11111
#define IMOV	0b00000
#define MOV		0b00001
#define LOAD	0b00010
#define STORE	0b00011
#define ADD		0b00100
#define SUB		0b00101
#define MUL		0b00110
#define DIV		0b00111
#define AND		0b01000
#define OR		0b01001
#define XOR		0b01010
#define NOT		0b01011
#define JMP		0b01100
#define JZ		0b01101
#define JNZ		0b01110
#define PUSH	0b01111
#define POP		0b10000
#define CMP		0b10001
#define CALL	0b10010
#define RET		0b10011
#define NOP		0b10100

uint16_t memory[SIZE];
uint16_t reg[REG+2];
uint16_t sp=0xFFFC;
uint16_t pc;
bool z_flag=false;

void load_program(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        exit(1);
    }
    size_t readItems = fread(memory, sizeof(uint16_t), SIZE, file);
    if(readItems == 0) {
        printf("Failed to read from file: %s\n", filename);
        fclose(file);
        exit(1);
    }
    fclose(file);
}

int emulate(void)
{
  uint16_t instruction=memory[pc];
  uint16_t mnemonic   =(instruction>>11)&0x1F;
  uint16_t ope1_reg   =(instruction>>8 )&0x07;
  uint16_t ope2_reg   =(instruction>>5 )&0x07;
  uint16_t ope3_reg   =(instruction>>2 )&0x07;
  uint16_t immediate  =(instruction>>0 )&0xFF;
  uint16_t addr_date  =memory[pc+1];
  /*
  origin  AAAAA BBB CCCcccCC
  sift 11 00000 000 000AAAAA
  sift 8  00000 000 AAAAABBB
  sift 5  00000 AAA AABBBCCC
  sift 2  00AAA AAB BBCCCccc
  sift 0  00000 000 CCCcccCC
  
  MOV   AAAAA BBB CCC 00000
  IMOV  AAAAA BBB CCCCCCCC
  LOAD  AAAAA BBB CCC DDD 00
  STORE AAAAA BBB CCC DDD 00
  ADD   AAAAA 
  */
  switch(mnemonic)
  {
    case HLT:
      return 1;
    case MOV:
      reg[ope1_reg]=reg[ope2_reg];
      break;
    case IMOV:
      reg[ope1_reg]=immediate;
      break;
    case LOAD:
      reg[ope1_reg]=memory[(reg[ope2_reg]<<8)|reg[ope3_reg]];
      break;
    case STORE:
      memory[(reg[ope1_reg]<<8)|reg[ope2_reg]]=reg[ope3_reg];
      break;
    case ADD:
      reg[ope1_reg]=reg[ope1_reg]+reg[ope2_reg];
      break;
    case SUB:
      reg[ope1_reg]=reg[ope1_reg]-reg[ope2_reg];
      break;
    case MUL:
      reg[ope1_reg]=reg[ope1_reg]*reg[ope2_reg];
      break;
    case DIV:
      reg[ope1_reg]=reg[ope1_reg]/reg[ope2_reg];
      break;
    case AND:
      reg[ope1_reg]&=reg[ope2_reg];
      break;
    case OR:
      reg[ope1_reg]|=reg[ope2_reg];
      break;
    case XOR:
      reg[ope1_reg]^=reg[ope2_reg];
      break;
    case NOT:
      reg[ope1_reg]=~reg[ope2_reg];
      break;
    case JMP:
      pc=(reg[ope1_reg]<<8)|reg[ope2_reg];
      pc--;
      break;
    case JZ:
      if(z_flag==1)
      {
        pc=(reg[ope1_reg]<<8)|reg[ope2_reg];
        pc--;
      }
      break;
    case JNZ:
      if(z_flag==0)
      {
        pc=(reg[ope1_reg]<<8)|reg[ope2_reg];
        pc--;
      }
      break;
    case PUSH:
      memory[sp]=reg[ope1_reg];
      sp--;
      break;
    case POP:
      reg[ope1_reg]=memory[sp];
      sp++;
      break;
    case CMP:
      if(reg[ope1_reg]==reg[ope2_reg])
      {
        z_flag=1;
      }else{
        z_flag=0;
      }
      break;
    case NOP:
      break;


  }

  return 0;
}

void gpu(SDL_Renderer *renderer)
{
	for(int y=0; y < WINDOW_HEIGHT; y++)
	{
		for(int x=0; x < WINDOW_WIDTH; x++)
		{
			int index = (y * WINDOW_WIDTH + x) - 0xF000;
			if(index >= 0 && index < SIZE) {
				uint16_t pixel = memory[index];
				
				uint16_t r = (pixel & 0xF800) >> 8;
				uint16_t g = (pixel & 0x07E0) >> 3;
				uint16_t b = (pixel & 0x001F);
				
				SDL_SetRenderDrawColor(renderer, r, g, b, 255);
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}
}

void setPixelData(uint16_t address,uint16_t value)
{
	if(address >= 0xF000 && address <= 0xFFFC)
	{
		memory[address-0xF000]=value;
	}
}

int main(int argc, char *argv[])
{
	bool isRunning=true;


	if (argc != 2) {
        printf("Usage: %s <input.bin>\n", argv[0]);
        return 1;
    }

    load_program(argv[1]);
    
    
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Window *window=SDL_CreateWindow("SDL Memory Display",
	SDL_WINDOWPOS_UNDEFINED,
	SDL_WINDOWPOS_UNDEFINED,
	WINDOW_WIDTH,WINDOW_HEIGHT,0);
	
	SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
	
	while(isRunning)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type==SDL_QUIT)
			{
				isRunning=false;
			}
		}
		SDL_SetRenderDrawColor(renderer,0,0,0,255);
		SDL_RenderClear(renderer);
		
		gpu(renderer);
		
		
			pc=0;
			while (emulate()==0)
    		{
        		pc++;
    		}
		
		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
    return 0;
}
