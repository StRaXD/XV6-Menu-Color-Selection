// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;


static struct {
	struct spinlock lock;
	int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
	static char digits[] = "0123456789abcdef";
	char buf[16];
	int i;
	uint x;

	if(sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do{
		buf[i++] = digits[x % base];
	}while((x /= base) != 0);

	if(sign)
		buf[i++] = '-';

	while(--i >= 0)
		consputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
	int i, c, locking;
	uint *argp;
	char *s;

	locking = cons.locking;
	if(locking)
		acquire(&cons.lock);

	if (fmt == 0)
		panic("null fmt");

	argp = (uint*)(void*)(&fmt + 1);
	for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
		if(c != '%'){
			consputc(c);
			continue;
		}
		c = fmt[++i] & 0xff;
		if(c == 0)
			break;
		switch(c){
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if((s = (char*)*argp++) == 0)
				s = "(null)";
			for(; *s; s++)
				consputc(*s);
			break;
		case '%':
			consputc('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
			consputc(c);
			break;
		}
	}

	if(locking)
		release(&cons.lock);
}

void
panic(char *s)
{
	int i;
	uint pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for(i=0; i<10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for(;;)
		;
}



#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
char kopija[2000];
int boje[2000];
int boja=0x0700;
int brojac=0;
char sep[10] = {'-', '-' , '-' , '-' , '-', '-', '-', '-' , '-'};
char whtYel[10] = {'|', 'W' , 'H' , 'T' , ' ', 'Y', 'E', 'L' , '|'};
char redAqu[10] = {'|', 'R' , 'E' , 'D' , ' ', 'A', 'Q', 'U' , '|'};
char purWht[10] = {'|', 'P' , 'U' , 'R' , ' ', 'W', 'H', 'T' , '|'};
char whtBlk[10] = {'|', 'W' , 'H' , 'T' , ' ', 'B', 'L', 'K' , '|'};
int whtYelEnabled=0;
int redAquEnabled=0;
int purWhtEnabled=0;
int whtBlkEnabled=1;
int konzola = 0;

int getPosition(int c){
	int pos;
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);
	if(c == '\n')
		pos += 80 - pos%80;
	else if(c == BACKSPACE){
		if(pos > 0) --pos;
	}
	if(80-(pos%80)<=10){
		pos=pos-10;
	}
	return pos;
}

void desnaKonzola(int c){
	int i=0;
	int pos = getPosition(c);
	for(i=0;i<=2000;i++){
			kopija[i] =  crt[i];
		}
		for(i=0;i<9;i++){
			crt[pos-80+i] = (sep[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-160+i] = (whtYel[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-240+i] = (sep[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-320+i] = (redAqu[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-400+i] = (sep[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-480+i] = (purWht[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-560+i] = (sep[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-640+i] = (whtBlk[i]&0xff) | 0x7000;
		}
		for(i=0;i<9;i++){
			crt[pos-720+i] = (sep[i]&0xff) | 0x7000;
		}
}


static void
cgaputc(int c)
{
	int pos;
	// Cursor position: col + 80*row.
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);

	if(c == '\n')
		pos += 80 - pos%80;
	else if(c == BACKSPACE){
		if(pos > 0) --pos;
	}else if(whtYelEnabled==1){
		boja=0x6700;
		boje[pos] = boja;
		crt[pos++] = (c&0xff) | 0x6700; 
	}else if(redAquEnabled==1){
		boja=0x3400;
		boje[pos] = boja;
		crt[pos++] = (c&0xff) | 0x3400; 
	}else if(purWhtEnabled==1){
		boja=0x7500;
		boje[pos] = boja;
		crt[pos++] = (c&0xff) | 0x7500; 
	}
	else if(whtBlkEnabled==1){
		boja=0x0700;
		boje[pos] = boja;
		crt[pos++] = (c&0xff) | 0x0700;  // black on white
	}

	if(pos < 0 || pos > 25*80)
		panic("pos under/overflow");

	if((pos/80) >= 24){  // Scroll up.
		memmove(crt, crt+80, sizeof(crt[0])*23*80);
		memmove(boje, boje+80, sizeof(boje[0])*23*80);
		pos -= 80;
		memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
		memset(boje+pos, 0, sizeof(boje[0])*(24*80 - pos));
	}

	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
	crt[pos] = ' ' | 0x0700;
}

void
consputc(int c)
{
	if(panicked){
		cli();
		for(;;)
			;
	}

	if(c == BACKSPACE){
		uartputc('\b'); uartputc(' '); uartputc('\b');
	} else
		uartputc(c);
	cgaputc(c);
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint r;  // Read index
	uint w;  // Write index
	uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x


void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;
	int i=0;
	int pos=getPosition(c);

	acquire(&cons.lock);
	while((c = getc()) >= 0){
		if(c==1){
			if(konzola==0){
				if(whtBlkEnabled==1){
					desnaKonzola(c);
					for(i=0;i<9;i++){
					crt[pos-640+i] = (whtBlk[i]&0xff) | 0x2000;
					}
				}
				if(purWhtEnabled==1){
					desnaKonzola(c);
					for(i=0;i<9;i++){
					crt[pos-480+i] = (purWht[i]&0xff) | 0x2000;
					}
				}
				if(redAquEnabled==1){
					desnaKonzola(c);
					for(i=0;i<9;i++){
					crt[pos-320+i] = (redAqu[i]&0xff) | 0x2000;
					}
				}
				if(whtYelEnabled==1){
					desnaKonzola(c);
					for(i=0;i<9;i++){
					crt[pos-160+i] = (whtYel[i]&0xff) | 0x2000;
					}
				}
			
			konzola=1;
		}else if(konzola==1){
			for(i=0;i<=2000;i++){
				crt[i] = kopija[i] | boje[i];
			}
			konzola=0;
		}
		c=0;
		}
		if(konzola==1){
		if(c=='s' || c=='S'){
			if(whtBlkEnabled==1){
					for(i=0;i<9;i++){
						crt[pos-480+i] = (purWht[i]&0xff) | 0x2000;
						}
					for(i=0;i<9;i++){
						crt[pos-640+i] = (whtBlk[i]&0xff) | 0x7000;
						}
						whtBlkEnabled=0;
						purWhtEnabled=1;
						break;
				}
						if(purWhtEnabled==1){
					for(i=0;i<9;i++){
						crt[pos-320+i] = (redAqu[i]&0xff) | 0x2000;
						}

					for(i=0;i<9;i++){
						crt[pos-480+i] = (purWht[i]&0xff) | 0x7000;
						}
						purWhtEnabled=0;
						redAquEnabled=1;
						break;
				}
			if(redAquEnabled==1){
					for(i=0;i<9;i++){
						crt[pos-160+i] = (whtYel[i]&0xff) | 0x2000;
						}
					for(i=0;i<9;i++){
						crt[pos-320+i] = (redAqu[i]&0xff) | 0x7000;
						}
						redAquEnabled=0;
						whtYelEnabled=1;
						break;
				}
		}
		if(c=='w' || c=='W'){
			if(whtYelEnabled==1){
				for(i=0;i<9;i++){
						crt[pos-160+i] = (whtYel[i]&0xff) | 0x7000;
						}
				for(i=0;i<9;i++){
						crt[pos-320+i] = (redAqu[i]&0xff) | 0x2000;
						}
					whtYelEnabled=0;
					redAquEnabled=1;
					break;
			}
			if(redAquEnabled==1){
				for(i=0;i<9;i++){
						crt[pos-480+i] = (purWht[i]&0xff) | 0x2000;
						}
				for(i=0;i<9;i++){
						crt[pos-320+i] = (redAqu[i]&0xff) | 0x7000;
						}
					purWhtEnabled=1;
					redAquEnabled=0;
					break;
			}
			if(purWhtEnabled==1){
				for(i=0;i<9;i++){
						crt[pos-640+i] = (whtBlk[i]&0xff) | 0x2000;
						}
				for(i=0;i<9;i++){
						crt[pos-480+i] = (purWht[i]&0xff) | 0x7000;
						}
					whtBlkEnabled=1;
					purWhtEnabled=0;
					break;
			}
			
		}
		}
		
		if(konzola!=1){
		switch(c){
		case C('P'):  // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			doprocdump = 1;
			break;
		case C('U'):  // Kill line.
			while(input.e != input.w &&
			      input.buf[(input.e-1) % INPUT_BUF] != '\n'){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('H'): case '\x7f':  // Backspace
			if(input.e != input.w){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		default:
			if(c != 0 && input.e-input.r < INPUT_BUF){
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				consputc(c);
				if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
					input.w = input.e;
					wakeup(&input.r);
				}
			}
			break;
		}
		}
	}
	release(&cons.lock);
	if(doprocdump) {
		procdump();  // now call procdump() wo. cons.lock held
	}
}

int
consoleread(struct inode *ip, char *dst, int n)
{
	uint target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while(n > 0){
		while(input.r == input.w){
			if(myproc()->killed){
				release(&cons.lock);
				ilock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}
		c = input.buf[input.r++ % INPUT_BUF];
		if(c == C('D')){  // EOF
			if(n < target){
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if(c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);

	return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
	int i;

	iunlock(ip);
	acquire(&cons.lock);
	for(i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);
	ilock(ip);

	return n;
}

void
consoleinit(void)
{
	int i=0;
	for(i=0;i<=2000;i++){
		boje[i] = 0x0700;
	}
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}

