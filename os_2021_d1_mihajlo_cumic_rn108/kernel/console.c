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
//moje globalne promenljive
static altFLag = 0;
static int enterMenu = 0;
static int menuOpen = 0;
static ushort hiddenItems[345];
static int cursorPrevPos;
static int menuMode = 0;
static int cursorLookPrev;
//cubanje izabrane boje
static ushort colorFG = 0x0700;
static ushort colorBG = 0x0000;


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

static void
cgaputc(int c)
{
	int pos;

	// Cursor position: col + 80*row.
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);

	if(menuMode)
	{
		menuMovement(c, pos);
		if(c == 'e' || c == 'r') colorSelector(c, pos);
		
	}else
	{

		if(c == '\n')
		{	
			if(enterMenu)
			{
				pos+= 57;
			}else pos += 80 - pos%80;
		}else if(c == BACKSPACE){
			if(pos > 0) --pos;
		} else
		{
			 crt[pos++] = (c&0xff) | colorFG | colorBG;
			  // black on white
		}

		if(pos < 0 || pos > 25*80)
			panic("pos under/overflow");

		if((pos/80) >= 24){  // Scroll up.
			memmove(crt, crt+80, sizeof(crt[0])*23*80);
			pos -= 80;
			memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
			changeColorFG();
		}

		
		outb(CRTPORT, 14);
		outb(CRTPORT+1, pos>>8);
		outb(CRTPORT, 15);
		outb(CRTPORT+1, pos);
		crt[pos] = ' ' | colorFG | colorBG;
		crt[0] = crt[0] & 0x00FF | colorFG | colorBG;
	}
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

#define C(x)  ((x)-'@')


void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;

	acquire(&cons.lock);
	while((c = getc()) >= 0){
		switch(c){
		case C('P'):  // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			if(!menuOpen) doprocdump = 1;
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
		case C('C'):
			if(altFLag == 0) ++altFLag;
			else altFLag = 0; 
			break;
		case C('O'):
			if(altFLag == 1) ++altFLag;
			else altFLag  = 0;
			break;
		case C('L'):
			if(altFLag == 2)
			{
				if(!menuOpen)
				{
					prevCursorPos();
					saveHidden();
					cursorPos(56);
					menuPrint();
					menuOpen = 1;
					menuMode = 1;
					cursorPos(146);
					cursorLook(146);
				}else
				{
					menuClose();
					menuOpen = 0;
					cursorPos(cursorPrevPos);
					menuMode = 0;
				}
			}	
			altFLag = 0;
			break;		 	
		default:
			if(c != 0 && input.e-input.r < INPUT_BUF){
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				consputc(c);
				if((c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF) && !menuOpen){
					input.w = input.e;
					wakeup(&input.r);
				}
			}
			break;
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
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}

//za menjanje pozicije cursora
void
cursorPos(int pos)
{
	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
	
}

//f-ja koja sacuva prekrivene karaktere
void
saveHidden(void)
{
	int i, j;
	int t = 0;
	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 23; j++)
		{
			hiddenItems[t++] = crt[56 + j + 80*i];
		}
	}
}

//ispisuje izgled menija
void
menuPrint(void)
{
	enterMenu = 1;
	cons.locking = 0;
	cprintf("/---<FG>--- ---<BG>---\\\n|Black     |Black     |\n|Blue      |Blue      |\n|Green     |Green     |\n|Aqua      |Aqua      |\n|Red       |Red       |\n|Purple    |Purple    |\n|Yellow    |Yellow    |\n|White     |White     |\n\\---------------------/");
	stopColorChange();
}

//zatvara meni i ispisuje prekrivene karaktere
void
menuClose(void)
{
	int i, j;
	int t = 0;
	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 23; j++)
		{
			crt[56 + j + 80*i] = hiddenItems[t++] & 0x00FF | colorFG | colorBG;
		}

	}

	enterMenu = 0;
}

//postavlja na globalnu promenljivu poziciju kursora na kojoj je bio pre otvaranja menija
void
prevCursorPos(void)
{
	outb(CRTPORT, 14);
	cursorPrevPos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	cursorPrevPos |= inb(CRTPORT+1);
}

void
menuMovement(int c, int pos)
{
	cursorLookPrev = pos;
	int newPos = 0;
	if(c == 'w')
	{
		if(pos == 146 || pos == 157)
		{
			newPos = pos + 560;
			cursorPos(newPos);
		}else
		{
			newPos = pos - 80;
			cursorPos(newPos);
		}
	}else if(c == 's')
	{
		if(pos == 706 || pos ==717)
		{
			newPos = pos - 560;
			cursorPos(newPos);
		}else
		{
			newPos = pos + 80;
			cursorPos(newPos);
		}
	}else if(c == 'a')
	{ 
		if(pos % 10 == 6)
		{
			newPos = pos + 11;
			cursorPos(newPos);
		}else if(pos % 10 == 7)
		{
			newPos = pos - 11;
			cursorPos(newPos);
		}
	}else if(c == 'd')
	{
		if(pos % 10 == 6)
		{
			newPos = pos + 11;
			cursorPos(newPos);
		}else if(pos % 10 == 7)
		{
			newPos = pos - 11;
			cursorPos(newPos);
		}
	}

	if(newPos != 0) cursorLook(newPos);
}

void
cursorLook(int pos)
{
	int i = 10;
	while(i > 0)
	{
		crt[cursorLookPrev - i] = crt[cursorLookPrev - i] & 0x00FF | 0x0F00;
		crt[pos - i] = crt[pos - i] & 0x00FF | 0xF000;
		i--;
	}
	crt[cursorLookPrev] = crt[cursorLookPrev] & 0x00FF | 0x0000;
	crt[pos] = crt[pos] & 0x00FF | 0xFF00;
	crt[0] = crt[0] & 0x00FF | colorFG | colorBG;//ova linija regulisa da ne nestaje crt[0] pri prvom otvaranju menija
}

void
colorSelector(int c, int pos)
{
	cursorLook(pos);
	if(pos == 146)
	{
		colorFG = 0x0000;
	}else if(pos == 157)
	{
		colorBG = 0x0000;
	}else if(pos == 226)
	{
		colorFG = 0x0100;
	}else if(pos == 237)
	{
		colorBG = 0x1000;
	}else if(pos == 306)
	{
		colorFG = 0x0200;
	}else if(pos == 317)
	{
		colorBG = 0x2000;
	}else if(pos == 386)
	{
		colorFG = 0x0300;
	}else if(pos == 397)
	{
		colorBG = 0x3000;
	}else if(pos == 466)
	{
		colorFG = 0x0400;
	}else if(pos == 477)
	{
		colorBG = 0x4000;
	}else if(pos == 546)
	{
		colorFG = 0x0500;
	}else if(pos == 557)
	{
		colorBG = 0x5000;
	}else if(pos == 626)
	{
		colorFG = 0x0600;
	}else if(pos ==637)
	{
		colorBG = 0x6000;
	}else if(pos == 706)
	{
		colorFG = 0x0700;
	}else if(pos == 717)
	{
		colorBG = 0x7000;
	}

	if(c == 'r')
	{
		if(pos % 10 == 6)
		{
			colorFG += 0x0800;
		}
		if(pos % 10 == 7)
		{
			colorBG += 0x8000;
		}
	} 
	changeColorFG();
	cursorLook(pos);


		
}

void
changeColorFG(void)
{
	
	int i =0;
	ushort tmp;
			while(i < 25*80)
			{
				tmp = crt[i] & 0x00FF;
				tmp = tmp | colorFG | colorBG; 
				crt[i] = tmp;
				
				
				i++; 
			}
			if(menuOpen) stopColorChange();
	
			
}

void
stopColorChange(void)
{
	int i, j;
	int t = 0;
	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 23; j++)
		{
			crt[56 + j + 80*i] = crt[56 + j + 80*i]& 0x00FF | 0x0F00;
		}

	}

	
}

