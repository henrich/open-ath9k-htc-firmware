#include "sys_cfg.h"
#include "athos_api.h"

#if defined(PROJECT_K2)
#if SYSTEM_MODULE_SFLASH
#include "sflash_api.h"
#endif
#endif /* #if defined(PROJECT_K2) */

#if defined(SYSTEM_MODULE_DBG)

/* Function prototypes */
int db_help_cmd(char *, char*, char*, char*);
int db_ldr_cmd(char*, char*, char*, char*);
int db_str_cmd(char*, char*, char*, char*);
int db_dump_memory(char* cmd, char* param1, char* param2, char* param3);
int db_info_cmd(char*, char*, char*, char*);
int db_cmd_dbg(char*, char*, char*, char*);
int db_usb_cmd(char*, char*, char*, char*);
int db_intr_cmd(char*, char*, char*, char*);
int db_patch_cmd(char*, char*, char*, char*);

int db_cmd_memtest(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_dmips(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_starthtc(char* cmd, char* param1, char* param2, char* param3);

int db_eeprom_cmd(char* cmd, char* param1, char* param2, char* param3);
int db_wdt_cmd(char* cmd, char* param1, char* param2, char* param3);

#if defined(PROJECT_K2)
#if SYSTEM_MODULE_SFLASH
int db_cmd_sferase(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_sfpg(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_sfru(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_sfrm(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_sfrdsr(char* cmd, char* param1, char* param2, char* param3);
#endif
#endif /* #if defined(PROJECT_K2) */
int db_cmd_memcmp(char* cmd, char* param1, char* param2, char* param3);
int db_cmd_memdump(char* cmd, char* param1, char* param2, char* param3);

int db_clock_cmd(char* cmd, char* param1, char* param2, char* param3);

uint16_t db_get_cmd_line(uint8_t ch, char *cmd_line, uint16_t* i);
int db_formalize_command(char*, char*);
int db_ascii_to_hex(char*, unsigned long*);
int db_hex_to_ascii(unsigned long, char*);
void zfDebugTask(void);

int db_info_intr(char* cmd, char* param1, char* param2, char* param3);

extern u32_t this_is_global_variables;

/* Console debug command table */
const struct DB_COMMAND_STRUCT command_table[] =
{
	{"HELP",   ", List all debug commands", db_help_cmd},
	{"?",      ", Equal to HELP comamnd", db_help_cmd},

	/* Basic load/store/dump command */
	{"LDR",    "<Hex addr>, Load word", db_ldr_cmd},
	{"LDRH",   "<Hex addr>, Load half word", db_ldr_cmd},
	{"LDRB",   "<Hex addr>, Load byte", db_ldr_cmd},
	{"STR",    "<Hex addr> <Hex value>, Store word", db_str_cmd},
	{"STRH",   "<Hex addr> <Hex value>, Store half word", db_str_cmd},
	{"STRB",   "<Hex addr> <Hex value>, Store byte", db_str_cmd},
	{"DUMP",   "<Hex addr>, Dump memory", db_dump_memory},
	{"INFO",   ", Print debug information", db_info_cmd},
	{"USB",   ", usb releated command", db_usb_cmd},
	{"INTR",   ", intr releated command", db_intr_cmd},
	{"PATCH",   ", patch function releated command", db_patch_cmd},
	{"DBG",    ", mute all print msg", db_cmd_dbg},
	{"CLOCK",    ", change the clock...", db_clock_cmd},
	{"MEMTEST",    "<Hex addr> <Number of bytes> test memory", db_cmd_memtest},
	{"HTCR", "Issue HTC ready to host", db_cmd_starthtc},
	{"EEP",   ", eeprom r/w debug command", db_eeprom_cmd},
	{"WDT",   ", wdt debug command", db_wdt_cmd},
#if defined(PROJECT_K2)
#if SYSTEM_MODULE_SFLASH
	{"SFE", ", S<Hex>/B<Hex>/C, SPI Flash chip erase", db_cmd_sferase},
	{"SFPG", "<Hex addr> <Hex len> <Hex buf>, SPI Flash program", db_cmd_sfpg},
	{"SFRU", "f/r <Hex addr> <Hex addr>, SPI Flash fast read/read to UART", db_cmd_sfru},
	{"SFRM", "f/r <Hex addr> <Hex addr>, SPI Flash fast read/read to Memory 0x520000", db_cmd_sfrm},
	{"SFRDSR", ", SPI Flash status register read", db_cmd_sfrdsr},
#endif
#endif /* #if defined(PROJECT_K2) */
	{"MEMCMP", "<Hex addr> <Hex addr> <Hex len>, memory comparison", db_cmd_memcmp},
	{"MEMDMP", "<Hex addr> <Hex addr>, memory dump", db_cmd_memdump},
	{"", "", 0}
	/* {Command, Help description, function} */
};

char cmd_buffer[COMMAND_BUFFER_SIZE][DB_MAX_COMMAND_LENGTH]; /* Backup previous command */
int cmd_buf_ptr;
int cmd_buf_full;
char raw_cmd[DB_MAX_COMMAND_LENGTH];
char cmd_str[DB_MAX_COMMAND_LENGTH*4];
int cmd_not_found;
uint16_t gvLen;
int pressed_time;

//////////////////////////////////////////////////
#define MAX_REG_NUM 16

typedef struct reg_elem {
	unsigned char valid;
	unsigned char mode;     // byte, half-word word
	unsigned long reg_addr;
} t_reg_elem;

t_reg_elem reg_buffer[MAX_REG_NUM];

//////////////////////////////////////////////////

void zfDebugInit(void)
{
	uint8_t ch;

	/* Purge Rx FIFO */
	while ((zm_get_char(&ch)) != 0)
	{
	}

	cmd_buf_ptr = 0;
	cmd_buf_full = FALSE;
	gvLen = 0;
	pressed_time = 0;
}

void zfDebugTask(void)
{
	int i;
	uint8_t ch;

	if ((zm_get_char(&ch)) == 0)
	{
		return;
	}

	if (db_get_cmd_line(ch, raw_cmd, &gvLen) == 0)
	{
		return;
	}

	if (db_formalize_command(raw_cmd, cmd_str))
	{
		gvLen = 0;
		i = 0;

		cmd_not_found = TRUE;
		while(command_table[i].cmd_func)
		{
			if (!strcmp(command_table[i].cmd_str, cmd_str))
			{
				cmd_not_found = FALSE;
				command_table[i].cmd_func(cmd_str,
							  cmd_str+DB_MAX_COMMAND_LENGTH,
							  cmd_str+DB_MAX_COMMAND_LENGTH*2,
							  cmd_str+DB_MAX_COMMAND_LENGTH*3);
				break;
			}
			i++;
		}
		if (cmd_not_found)
		{
			zm_uart_send("Error, HELP for command list.\n\r", 31);
		}

	}

	zm_uart_send(">", 1);
	return;
}

uint16_t db_get_cmd_line(uint8_t ch, char *cmd_line, uint16_t* i)
{
	int cmd_buf_loc;

	switch (ch)
	{
	case '\\' : /* Last command */
		pressed_time++;
		if (pressed_time >= COMMAND_BUFFER_SIZE)
		{
			pressed_time--;
		}
		cmd_buf_loc = cmd_buf_ptr - pressed_time;
		if (cmd_buf_loc < 0)
		{
			if (cmd_buf_full == TRUE)
			{
				cmd_buf_loc += COMMAND_BUFFER_SIZE;
			}
			else
			{
				cmd_buf_loc = 0;
			}
		}

		if(strlen(cmd_buffer[cmd_buf_loc]) != 0)
		{
			strcpy(cmd_line, cmd_buffer[cmd_buf_loc]);
			*i = strlen(cmd_buffer[cmd_buf_loc]);
			zm_uart_send("\r>", 2);
			zm_uart_send(cmd_line, *i);
		}
		break;
	case 13 : /* Return */
		pressed_time = 0;
		cmd_line[*i] = 0;
		zm_uart_send("\n\r", 2);
		if (*i != 0)
		{
			//Filter duplicated string in command history
			if (strcmp(cmd_buffer[(cmd_buf_ptr==0)?(COMMAND_BUFFER_SIZE-1):(cmd_buf_ptr-1)], cmd_line) != 0)
			{
				strcpy(cmd_buffer[cmd_buf_ptr++], cmd_line);
			}
		}
		if (cmd_buf_ptr >= COMMAND_BUFFER_SIZE)
		{
			cmd_buf_ptr = 0;
			cmd_buf_full = TRUE;
		}
		return 1;
	case '\b' : /* Backspace */
		pressed_time = 0;
		if (*i > 0)
		{
			*i = *i-1;
			zm_uart_send("\b \b", 3);
		}
		break;
	case 0 : //None
		break;
	default :
		if ((ch >= ' ') && (ch <= '~'))
		{
			pressed_time = 0;
			if (*i < DB_MAX_COMMAND_LENGTH-2)
			{
				if ((ch >= 0x11) && (ch <= 0x7e))
				{
					//if ((buf <= 'z') && (buf >= 'a'))
					//{
					//    buf -= 'a' - 'A';
					//}
					cmd_line[*i] = ch;
					*i = *i + 1;
					zm_uart_send(&ch, 1);
				}
			}
		}
		else
		{
			ch = 7; /* Beep */
			zm_uart_send(&ch, 1);
		}
		break;
	} /* end of switch */

	return 0;

}

int db_formalize_command(char* raw_str,  char* cmd_str)
{
	int i = 0;
	int j;
	int k;


	for (k=0; k<4; k++)
	{
		/* Remove preceeding spaces */
		while (raw_str[i++] == ' '){}
		i--;

		/* Copy command string */
		j = 0;
		while(raw_str[i] && (raw_str[i] != ' '))
		{
			if (k == 0)
			{
				if ((raw_str[i] <= 'z') && (raw_str[i] >= 'a'))
				{
					raw_str[i] -= 'a' - 'A';
				}
				cmd_str[k*DB_MAX_COMMAND_LENGTH + j++] = raw_str[i++];
			}
			else
			{
				cmd_str[k*DB_MAX_COMMAND_LENGTH + j++] = raw_str[i++];
			}
		}
		cmd_str[k*DB_MAX_COMMAND_LENGTH + j] = 0;
	}
	return (int)strlen(cmd_str);
}

int db_ascii_to_hex(char* num_str, unsigned long* hex_num)
{
	int i = 0;

	*hex_num = 0;
	while (num_str[i])
	{
		if ((num_str[i] >= '0') && (num_str[i] <= '9'))
		{
			*hex_num <<= 4;
			*hex_num += (num_str[i] - '0');
		}
		else if ((num_str[i] >= 'A') && (num_str[i] <= 'F'))
		{
			*hex_num <<= 4;
			*hex_num += (num_str[i] - 'A' + 10);
		}
		else if ((num_str[i] >= 'a') && (num_str[i] <= 'f'))
		{
			*hex_num <<= 4;
			*hex_num += (num_str[i] - 'a' + 10);
		}
		else
		{
			return -1;
		}
		i++;
	}
	return 0;
}

int db_ascii_to_int(char* num_str, unsigned long* int_num)
{
	int i = 0;

	*int_num = 0;
	while (num_str[i])
	{
		if ((num_str[i] >= '0') && (num_str[i] <= '9'))
		{
			*int_num *= 10;
			*int_num += (num_str[i] - '0');
		}
		else
		{
			return -1;
		}
		i++;
	}
	return 0;
}

int db_hex_to_ascii(unsigned long hex_num, char* num_str)
{
	int i;
	unsigned long four_bits;

	for (i=7; i>=0; i--)
	{
		four_bits = (hex_num >> i*4) & 0xf;
		if (four_bits < 10)
		{
			num_str[7-i] = four_bits + '0';
		}
		else
		{
			num_str[7-i] = four_bits - 10 + 'A';
		}
	}
	num_str[8] = 0;
	return 0;
}

int db_help_cmd(char* cmd, char* param1, char* param2, char* param3)
{
	int i;

	i = 0;

	zm_uart_send(ATH_DEBUGGER_VERSION_STR, strlen(ATH_DEBUGGER_VERSION_STR));
	zm_uart_send(ATH_COMMAND_LIST_STR, strlen(ATH_COMMAND_LIST_STR));

	while (command_table[i].cmd_func)
	{
		zm_uart_send(command_table[i].cmd_str, strlen(command_table[i].cmd_str));
		zm_uart_send("\t", 1);
		zm_uart_send(command_table[i].help_str, strlen(command_table[i].help_str));
		zm_uart_send("\n\r", 2);
		i++;
	}
	return i;
}

int db_ldr_cmd(char* cmd, char* param1, char* param2, char* param3)
{
	unsigned long val;
	unsigned long addr;
	char val_str[20];
	char addr_str[20];

	if (db_ascii_to_hex(param1, &addr) != -1)
	{
		if( addr == 0 )
		{
			zm_uart_send("Error! bad address 0x%08x.\n\r", (unsigned long)addr);
			return -1;
		}
		if (strcmp(cmd, "LDR") == 0)
		{
			addr &= 0xfffffffc;
			//val = *(unsigned long *)addr;

			val = HAL_WORD_REG_READ(addr);
		}
		else if (strcmp(cmd, "LDRH") == 0)
		{
			addr &= 0xfffffffe;
			val = HAL_HALF_WORD_REG_READ(addr);
		}
		else if (strcmp(cmd, "LDRB") == 0)
		{
		}

		db_hex_to_ascii(val, val_str);
		db_hex_to_ascii(addr, addr_str);

		zm_uart_send(addr_str, strlen(addr_str));
		zm_uart_send(" : ", 3);
		zm_uart_send(val_str, strlen(val_str));
		zm_uart_send("\n\r", 2);

		return 0;
	}
	else
	{
		zm_uart_send("Error! Incorrect format.\n\r", 26);

		return -1;
	}
}

int db_str_cmd(char* cmd, char* param1, char* param2, char* param3)
{
	unsigned long val;
	unsigned long addr;
	char val_str[20];
	char addr_str[20];

	if ((strlen(param2) > 0) &&
	    (db_ascii_to_hex(param1, &addr) != -1) &&
	    (db_ascii_to_hex(param2, &val) != -1))
	{
		if (strcmp(cmd, "STR") == 0)
		{
			addr &= 0xfffffffc;
			//HAL_WORD_REG_WRITE(addr, val);
			HAL_WORD_REG_WRITE(addr, val);
			//*(volatile unsigned long *)(addr & 0xfffffffc) = (unsigned long)val;
		}

		else if (strcmp(cmd, "STRH") == 0)
		{
			addr &= 0xfffffffe;
			//*(volatile unsigned short *)(addr & 0xfffffffe) = (unsigned short)val;
			HAL_HALF_WORD_REG_WRITE(addr, val);
		}
		else if (strcmp(cmd, "STRB") == 0)
		{
			if( addr & 0x00f00000 )
				HAL_BYTE_REG_WRITE(addr, val);
			else
				HAL_BYTE_REG_WRITE(addr^3, val);
			//*(volatile unsigned char *)addr = (unsigned char)val;
		}

		db_hex_to_ascii(val, val_str);
		db_hex_to_ascii(addr, addr_str);

		zm_uart_send(addr_str, strlen(addr_str));
		zm_uart_send(" : ", 3);
		zm_uart_send(val_str, strlen(val_str));
		zm_uart_send("\n\r", 2);

		return 0;
	}
	else
	{
		zm_uart_send("Error! Incorrect format.\n\r", 26);

		return -1;
	}
}

// macro extension the address to dump the memory
#define FOUR_BYTE_HEX_DUMP(addr) (" %02x %02x %02x %02x",		\
				  *(uint8_t*)((addr)+3), *(uint8_t*)((addr)+2), \
				  *(uint8_t*)((addr)+1), *(uint8_t*)((addr)))


int db_dump_memory(char* cmd, char* param1, char* param2, char* param3)
{
	unsigned long addr;
	unsigned long length;
	unsigned long ptrAddr;
	int i;

	if (db_ascii_to_hex(param1, &addr) != -1 &&
	    (db_ascii_to_int(param2, &length) != -1))
	{
		// if no length, default is 128 bytes to dump
		if( length == 0 )
			length = 128;
		addr &= 0xfffffffc;

		A_PRINTF("length: %d\n\r", length);

		//zm_uart_send("                     7 6 5 4  3 2 1 0\n\r", 28);
		A_PRINTF("           15 14 13 12 11 10 09 08   07 06 05 04 03 02 01 00\n\r");
		A_PRINTF("------------------------------------------------------------\n\r");
		for (i=0; i<length/16; i++)
		{
			//zfUartSendHex((unsigned long)addr);
			A_PRINTF("%08x: ", (unsigned long)addr);

			ptrAddr = (unsigned long *)addr;

			// dump from MSB to LSB
			A_PRINTF FOUR_BYTE_HEX_DUMP(ptrAddr+12);
			A_PRINTF FOUR_BYTE_HEX_DUMP(ptrAddr+8);
			A_PRINTF(" -");
			A_PRINTF FOUR_BYTE_HEX_DUMP(ptrAddr+4);
			A_PRINTF FOUR_BYTE_HEX_DUMP(ptrAddr);
			A_PRINTF("\n\r");
			addr+=16;
		}

		// the rest of the byte to dump
		if( (length %16)!=0 )
		{
			A_PRINTF("%08x: ", (unsigned long)addr);

			// make the space, since we dump MSB first
			for(i=0; i<(16-(length %16)); i++)
				A_PRINTF("   ");

			// if less than 8 bytes, add 2 more space for " -"
			if( (length%16) < 8 )
				A_PRINTF("  ");

			for(i=0; i<length%16; i++)
			{
				// MSB first,
				A_PRINTF(" %02x", *(uint8_t*)((addr+(length%16)-1)-i));

				if((16-(length%16))+i==7)
					A_PRINTF(" -");
			}
		}
		A_PRINTF("\n\r");
		return 0;
	}
	return -1;
}

LOCAL void dbg_timer_func(A_HANDLE alarm, void *data)
{
	A_PRINTF("this is a timer alarm function 0x%08x\n\r", xthal_get_ccount());
}

int db_patch_cmd(char* cmd, char* param1, char* param2, char* param3)
{

	return 0;
}

uint32_t delay = 0;

LOCAL void cb_tick()
{
	;    
}


int db_intr_cmd(char* cmd, char* param1, char* param2, char* param3)
{
#if SYSTEM_MODULE_INTR
	uint32_t pending_intrs;

	if(strcmp(param1, "read") == 0 )
	{
		{
			/* Update snapshot of pending interrupts */

			pending_intrs = A_INTR_GET_INTRPENDING();

			A_PRINTF("intr mask [0x%08x]\n\r", xthal_get_intenable());
			A_PRINTF("intr on [0x%08x]\n\r", pending_intrs);
		}
	}
	else if (strcmp(param1, "timer") == 0 )
	{
		uint32_t data = 0;

		if (strcmp(param2, "on") == 0 )
		{
			A_ATTACH_ISR(A_INUM_XTTIMER, cb_tick, NULL);

			pending_intrs = A_INTR_GET_INTRENABLE()|CMNOS_IMASK_XTTIMER;
			A_INTR_SET_INTRENABLE(pending_intrs);
			A_PRINTF("- intr [0x%08x]\n\r", pending_intrs);
		}
		else if ( strcmp(param2, "off") == 0 )
		{
			pending_intrs = A_INTR_GET_INTRENABLE()&(~CMNOS_IMASK_XTTIMER);
			A_INTR_SET_INTRENABLE(pending_intrs);
			A_PRINTF("- intr [0x%08x]\n\r", pending_intrs);
            
		}
		else if( db_ascii_to_hex(param2, &data)==0 )
		{
			if( data>=0 && data <=10 )
				delay = data;
			else
				delay = 3;
            
			A_PRINTF("==>set cb to %d seconds \n\r", delay);
		}

	}
	else
	{
		A_PRINTF("\tintr read - read the interrenable status\n\r");
		A_PRINTF("\tintr timer on/off/tick - timer attach on/off/ticks\n\r");

	}

#endif //#if SYSTEM_MODULE_INTR
	return 0;
}

uint32_t usb_swap_flag = 0; //default
uint32_t usb_swap_flag_changed = 0;
int db_usb_cmd(char* cmd, char* param1, char* param2, char* param3)
{
	A_PRINTF("THIS IS USB COMMAND\n\r");

	if( strcmp(param1, "que") == 0 )
	{
		HIFusb_DescTraceDump();
	}
	else
	{
		A_PRINTF("\tusb que - dump descriptor queue\n\r");
		A_PRINTF("\tusb fw on/off - enable/disable write fw download to ram\n\r");

	}
	return 0;
}

static void clk_change(uint32_t clk, uint32_t ratio, uint32_t baud)
{
	uint32_t clk_sel = 0;

	switch(clk){
        case 22:
		clk_sel = 0;
		break;
        case 88:
		clk_sel = 1;
		break;
        case 44:
		clk_sel = 2;
		break;
        case 117:
		clk_sel = 4;
		break;
        case 40:
		clk_sel = 6;            
		break;
        default:
		clk_sel = 6;
		break;
	}

	HAL_WORD_REG_WRITE(0x50040, (0x300|clk_sel|(ratio>>1)<<12));
	A_UART_HWINIT((clk*1000*1000)/ratio, baud);

}

int db_clock_cmd(char* cmd, char* param1, char* param2, char* param3)
{
	uint32_t ratio = 1;
	uint32_t baud = 19200;
	uint32_t clk = 0;
    
	if( db_ascii_to_int(param1, &clk) != -1 )
	{
		A_PRINTF("changing clock to %d\n", clk);
		clk_change(clk, ratio, baud);
	}
}

int db_info_cmd(char* cmd, char* param1, char* param2, char* param3)
{
#if 1

	if(strcmp(param1, "ram") == 0 )
	{
		A_ALLOCRAM_DEBUG();
	}
#if SYSTEM_MODULE_SYS_MONITOR
	else if(strcmp(param1, "cpu") == 0)
		zfPrintCpuUtilization();
#endif
	else   // defalut dump
		HIFusb_DescTraceDump();

	return 1;

#else
    
	{
		uint32_t ccount1;
		uint32_t ccount2;

		uint32_t data;
		register uint32_t data1;
		if( db_ascii_to_hex(param1, &data1)==0 )
		{
			__asm__ __volatile__ (
				"rsr     %0, ccount"
				: "=a" (ccount1) : : "memory"
				);
			data = *(volatile uint32_t *)(data1);
			__asm__ __volatile__ (
				"rsr     %0, ccount"
				: "=a" (ccount2) : : "memory"
				);
			A_PRINTF("\n\rread 0x%08x (0x%08x) use %d clocks\n\r", data1, data, ccount2-ccount1);
		}

		__asm__ __volatile__ (
			"rsr     %0, ccount"
			: "=a" (ccount1) : : "memory"
			);
		data = *(volatile uint32_t *)(data1);
		__asm__ __volatile__ (
			"rsr     %0, ccount"
			: "=a" (ccount2) : : "memory"
			);
		A_PRINTF("\n\rread 0x%08x (0x%08x) use %d clocks\n\r", data1, data, ccount2-ccount1);


		__asm__ __volatile__ (
			"rsr     %0, ccount"
			: "=a" (ccount1) : : "memory"
			);
		data = *(volatile uint32_t *)(data2);
		__asm__ __volatile__ (
			"rsr     %0, ccount"
			: "=a" (ccount2) : : "memory"
			);
		A_PRINTF("read 0x%08x (0x%08x) use %d clocks\n\r", data2, data, ccount2-ccount1);


		__asm__ __volatile__ (
			"rsr     %0, ccount"
			: "=a" (ccount1) : : "memory"
			);
		data = *(volatile uint32_t *)(data3);
		__asm__ __volatile__ (
			"rsr     %0, ccount"
			: "=a" (ccount2) : : "memory"
			);
		A_PRINTF("read 0x%08x (0x%08x) use %d clocks\n\r", data3, data, ccount2-ccount1);

	}
#endif
	return 1;
}

int db_cmd_dbg(char* cmd, char* param1, char* param2, char* param3)
{
}

int db_cmd_dmips(char* cmd, char* param1, char* param2, char* param3)
{

}

int db_cmd_starthtc(char* cmd, char* param1, char* param2, char* param3)
{
    extern htc_handle_t htc_handle;
    HTC_Ready(htc_handle);
}

int db_cmd_memtest(char* cmd, char* param1, char* param2, char* param3)
{
}


void eep_test()
{
}

#define WRITE_USB_DESC(pDesc, Offset)					\
	{								\
		uint16_t *pSrc = 0;					\
		uint16_t mSize = 0;					\
		pSrc = (uint16_t *)(pDesc);				\
		mSize = (*pSrc&0xff)/2;					\
		A_PRINTF("0x%04x, 0x%04x, 0x%08x\n", Offset, mSize, pSrc); \
		A_EEP_WRITE(Offset, mSize, pSrc);			\
		A_DELAY_USECS(500);					\
	}

#define READ_USB_DESC(pDesc, Offset, Size)				\
	{								\
		uint16_t *pDst;						\
		uint16_t mSize;						\
		pDst = (uint16_t *)pDesc;				\
		A_EEP_READ(Offset, 1, &mSize);				\
		mSize = mSize &0xff;					\
		mSize = mSize/2;					\
		if( mSize > Size)					\
			mSize = Size;					\
		A_PRINTF("0x%04x, 0x%04x, 0x%08x\n", Offset, mSize, pDst); \
		A_EEP_READ(Offset, mSize, pDst);			\
		A_DELAY_USECS(500);					\
	}


////////////////////////////////////////////////////////////////////////////////////////////////

extern uint16_t UsbDeviceDescriptor[];
extern uint16_t String00Descriptor[];
extern uint16_t String10Descriptor[];
extern uint16_t String20Descriptor[];
extern uint16_t String30Descriptor[];

int db_eeprom_cmd(char* cmd, char* param1, char* param2, char* param3)
{
	
}

int db_wdt_cmd(char* cmd, char* param1, char* param2, char* param3)
{
        if ( strcmp(param1, "rst") == 0 )
        {
		A_PRINTF(" reseting.....................\n\n\r");
		A_WDT_RESET();
        }
        else if( strcmp(param1, "on") == 0 )
        {
		A_WDT_ENABLE();
        }
        else if (strcmp(param1, "off") == 0 )
        {
		A_WDT_DISABLE();
        }
        else if ( strcmp(param1, "boot") == 0 )
        {
		if (ENUM_WDT_BOOT == A_WDT_LASTBOOT() )
			A_PRINTF("LAST BOOT IS %s", "wdt");
		else
			A_PRINTF("LAST BOOT IS %s", "normal boot");
        }
        else if (strcmp(param1, "loop") == 0 )
        {
		T_WDT_CMD wdt_cmd;
		uint32_t time_offset;
		A_PRINTF(" doing the wdt reseting................\n\n\r");

		if( db_ascii_to_hex(param2, &time_offset)!=0 )
		{
			if( time_offset < 0 || time_offset >0xffffffff )
				time_offset = 0xffffff;
		}
		A_PRINTF(" doing the wdt reseting (wdt tick: 0x%08x................\n\n\r", time_offset);
		wdt_cmd.cmd = WDT_TIMEOUT;
		wdt_cmd.timeout = time_offset;

		A_WDT_SET(wdt_cmd);
		while(1) ;
        }
        else if (strcmp(param1, "noloop") == 0 )
        {
		T_WDT_CMD wdt_cmd;
		uint32_t time_offset;
		A_PRINTF(" doing the wdt reseting................\n\n\r");

		if( db_ascii_to_hex(param3, &time_offset)!=0 )
		{
			if( time_offset < 0 || time_offset >0xffffffff )
				time_offset = 0xffffff;
		}
		A_PRINTF(" doing the wdt reseting (wdt tick: 0x%08x................\n\n\r", time_offset);

		wdt_cmd.cmd = WDT_TIMEOUT;
		wdt_cmd.timeout = time_offset;

		A_WDT_SET(wdt_cmd);
        }
        else if( strcmp(param1, "event") == 0 )
        {
		uint32_t event= 0x00123400;
#define USB_BYTE_REG_WRITE(addr, val)       	HAL_BYTE_REG_WRITE(USB_CTRL_BASE_ADDRESS|(uint8_t)(addr^3), (val))
#define USB_BYTE_REG_READ(addr)             	HAL_BYTE_REG_READ(USB_CTRL_BASE_ADDRESS|(uint8_t)(addr^3))

#define USB_WORD_REG_WRITE(addr, val)       	HAL_WORD_REG_WRITE(USB_CTRL_BASE_ADDRESS|(uint32_t)(addr), (val))
#define USB_WORD_REG_READ(addr)             	HAL_WORD_REG_READ(USB_CTRL_BASE_ADDRESS|(uint32_t)(addr))

		// disable ep3 intr
		USB_BYTE_REG_WRITE(0x17, USB_BYTE_REG_READ(0x17)|0xc0);

		//ZM_CBUS_FIFO_SIZE_REG = 0xf;
		USB_WORD_REG_WRITE(0x100, 0x0f);

		//ZM_EP3_DATA_REG = event;
		USB_WORD_REG_WRITE(0xF8, event);

		// tx done
		USB_BYTE_REG_WRITE(0xAE, USB_BYTE_REG_READ(0xAE)|0x08);

		// enable ep3 intr
		USB_BYTE_REG_WRITE(0x17, USB_BYTE_REG_READ(0x17)&0xbf);
        }
}

#if defined(PROJECT_K2)
#if SYSTEM_MODULE_SFLASH
/* Serial Flash -> Chip Erase, Sector Erase, Block Erase */
int db_cmd_sferase(char* cmd, char* param1, char* param2, char* param3)
{
	unsigned long       addr;

	if (strcmp(param1, "s") == 0)
	{
		if (db_ascii_to_hex(param2, &addr) != -1 && addr < SPI_FLASH_MAX_SIZE)
		{
			/* Sector size is 4K (0x1000) */
			A_PRINTF("Sector addr : 0x%08X\n\r", addr - addr%0x1000);
			A_SFLASH_ERASE(ZM_SFLASH_SECTOR_ERASE, addr);

			return 0;
		}
		else
		{
			A_PRINTF("Error! Incorrect format.\n\r");
			return -1;
		}
	}
	else if (strcmp(param2, "b") == 0)
	{
		if (db_ascii_to_hex(param2, &addr) != -1 && addr < SPI_FLASH_MAX_SIZE)
		{
			/* Sector size is 64K (0x10000) */
			A_PRINTF("Block addr : 0x%08X\n\r", addr - addr%0x10000);
			A_SFLASH_ERASE(ZM_SFLASH_BLOCK_ERASE, addr);

			return 0;
		}
		else
		{
			A_PRINTF("Error! Incorrect format.\n\r");
			return -1;
		}
	}
	else if (strcmp(param1, "c") == 0)
	{
		A_SFLASH_ERASE(ZM_SFLASH_CHIP_ERASE, addr);

		A_PRINTF("\n\r");
		return 0;
	}
	else
	{
		A_PRINTF("Error! Unknown command.\n\r");
		return -1;
	}
}

/* Serial Flash -> Program */
int db_cmd_sfpg(char* cmd, char* param1, char* param2, char* param3)
{
	unsigned long       addr, len, buf;

	if (db_ascii_to_hex(param1, &addr) != -1 &&
	    db_ascii_to_hex(param2, &len) != -1 &&
	    db_ascii_to_hex(param3, &buf) != -1 &&
	    ((addr+len) <= SPI_FLASH_MAX_SIZE) &&
	    addr%4 == 0 && len%4 == 0 && buf%4 == 0 &&
	    ((buf >=0x500000 && buf < 0x528000) || (buf >=0x4e0000 && buf < 0x4e6000)) )
	{
		A_SFLASH_PROG(addr, len, (A_UINT8 *)buf);

		A_PRINTF("\n\r");
		return 0;
	}
	else
	{
		A_PRINTF("Error! Incorrect format.\n\r");
		return -1;
	}
}

/* Serial Flash -> Read, Fast Read to UART */
int db_cmd_sfru(char* cmd, char* param1, char* param2, char* param3)
{
	A_UINT32            i;
	unsigned long       addr1, addr2, t_addr;
	A_UINT32            fast, val;

	if (strcmp(param1, "r") == 0)
		fast = 0;
	else if (strcmp(param1, "f") == 0)
		fast = 1;
	else
	{
		A_PRINTF("Error! Unknown command.\n\r");
		return -1;
	}

	if (db_ascii_to_hex(param2, &addr1) != -1 &&
	    db_ascii_to_hex(param3, &addr2) != -1 &&
	    addr1 < addr2 && addr1 < SPI_FLASH_MAX_SIZE &&
	    addr2 < SPI_FLASH_MAX_SIZE && addr1%4 == 0)
	{
		A_PRINTF("addr    data     data     data     data     data     data     data     data\n\r");
		A_PRINTF("======  ======== ======== ======== ======== ======== ======== ======== ========");

		for (i = 0, t_addr = addr1; t_addr < addr2; i++, t_addr += 4)
		{
			if ((i%8) == 0)
				A_PRINTF("\n\r%06X  ", t_addr);

			A_SFLASH_READ(fast, t_addr, 4, (A_UINT8 *)&val);
			A_PRINTF("%08X ", val);
		}

		A_PRINTF("\n\r");
		return 0;
	}
	else
	{
		A_PRINTF("Error! Incorrect format.\n\r");
		return -1;
	}
}

/* Serial Flash -> Read, Fast Read to Memory */
int db_cmd_sfrm(char* cmd, char* param1, char* param2, char* param3)
{
	A_UINT32            i;
	unsigned long       addr1, addr2, t_addr;
	A_UINT32            fast;
	A_UINT8             *buf = (A_UINT8 *)0x520000;

	if (strcmp(param1, "r") == 0)
		fast = 0;
	else if (strcmp(param1, "f") == 0)
		fast = 1;
	else
	{
		A_PRINTF("Error! Unknown command.\n\r");
		return -1;
	}

	if (db_ascii_to_hex(param2, &addr1) != -1 &&
	    db_ascii_to_hex(param3, &addr2) != -1 &&
	    addr1 < addr2 && addr1 < SPI_FLASH_MAX_SIZE &&
	    addr2 < SPI_FLASH_MAX_SIZE && addr1%4 == 0)
	{
		for (i = 0, t_addr = addr1; t_addr < addr2; i++, t_addr += 4)
		{
			A_SFLASH_READ(fast, t_addr, 4, buf + i*4);
		}

		A_PRINTF("\n\r");
		return 0;
	}
	else
	{
		A_PRINTF("Error! Incorrect format.\n\r");
		return -1;
	}
}

/* Serial Flash -> Read Status Register */
int db_cmd_sfrdsr(char* cmd, char* param1, char* param2, char* param3)
{
	A_PRINTF("0x%02X\n\r", A_SFLASH_RDSR());
	return 0;
}
#endif
#endif /* #if defined(PROJECT_K2) */

/* Memory Comparison */
int db_cmd_memcmp(char* cmd, char* param1, char* param2, char* param3)
{
	unsigned long       addr1, addr2, len;
	A_UINT8             *buf1, *buf2;

	if (db_ascii_to_hex(param1, &addr1) != -1 &&
	    db_ascii_to_hex(param2, &addr2) != -1 &&
	    db_ascii_to_hex(param3, &len) != -1 &&
	    addr1 != addr2 && addr1%4 == 0 && addr2%4 == 0 && len%4 == 0)
	{
		buf1 = (A_UINT8 *)addr1;
		buf2 = (A_UINT8 *)addr2;        ;

		A_PRINTF("memcmp(buf1, buf2, len) = %d\n\r", A_MEMCMP(buf1, buf2, len));
		return 0;
	}
	else
	{
		A_PRINTF("Error! Incorrect format.\n\r");
		return -1;
	}
}

/* Memory Dump */
int db_cmd_memdump(char* cmd, char* param1, char* param2, char* param3)
{
	A_UINT32            i;
	unsigned long       addr1, addr2, t_addr;
	A_UINT32            *val;

	if (db_ascii_to_hex(param1, &addr1) != -1 && db_ascii_to_hex(param2, &addr2) != -1 && addr1 < addr2 && addr1%4 == 0)
	{
		A_PRINTF("addr    data     data     data     data     data     data     data     data\n\r");
		A_PRINTF("======  ======== ======== ======== ======== ======== ======== ======== ========");

		for (i = 0, t_addr = addr1; t_addr < addr2; i++, t_addr += 4)
		{
			if ((i%8) == 0)
				A_PRINTF("\n\r%06X  ", t_addr);

			val = (A_UINT32 *)t_addr;
			A_PRINTF("%08X ", *val);
		}

		A_PRINTF("\n\r");
		return 0;
	}
	else
	{
		A_PRINTF("Error! Incorrect format.\n\r");
		return -1;
	}
}
void cmnos_dbg_module_install(struct dbg_api *apis)
{
	apis->_dbg_init = zfDebugInit;
	apis->_dbg_task = zfDebugTask;
}

#endif 	/* SYSTEM_MODULE_DBG */

