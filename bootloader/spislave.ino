#include <MicroView.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#define _GNU_SOURCE
#include <stdio.h>

#define SPEED 19200

#define RESET A0
#define MOSI A1
#define MISO A2
#define SCLK A3

static int sgetc (void)
{
    int c;
    do
    {
        c = Serial.read ();
    } while (c < 0);
    return c;
}

static int sprtf (const char *format, ...)
{
    char message[81];
    int n;
    va_list ap;

    va_start(ap, format);
    n = vsprintf (message, format, ap);
    va_end(ap);
    Serial.print (message);
    return n;
}

static int prtf (int row, const char *format, ...)
{
    char message[81];
    int n;
    va_list ap;

    va_start(ap, format);
    n = vsprintf (message, format, ap);
    va_end(ap);
    uView.setCursor(0, row * 16);
	uView.print("       ");
    uView.setCursor(0, row * 16);
	uView.print(message);
    uView.display();
    return n;
}

static unsigned toggle_bit (unsigned bit)
{
    if (bit)
        digitalWrite (MOSI, HIGH);
    else
        digitalWrite (MOSI, LOW);
    digitalWrite (SCLK, HIGH);
    bit = digitalRead (MISO);
    digitalWrite (SCLK, LOW);
    return bit == HIGH ? 1 : 0;
}

static uint8_t iobyte (uint8_t byte)
{
    uint8_t res = 0;
    int i;
    uint8_t mask = 0x80;
    // prtf (2, "%02X", byte);
    for (i = 0; i < 8; i++, mask >>= 1) {
        res <<= 1;
        res |= toggle_bit (byte & mask);
    }
    // prtf (2, "%02X / %02X", byte, res);
    return res;
}

static int prog_mode (void) 
{
    int ret;
    uint8_t byte;
    pinMode (RESET, OUTPUT);  
    pinMode (SCLK, OUTPUT);
    pinMode (MOSI, OUTPUT);
    pinMode (MISO, INPUT);
    delay (1);

    digitalWrite (SCLK, LOW);
    digitalWrite (MOSI, LOW);
    digitalWrite (RESET, HIGH);
    delay (1);
    digitalWrite (RESET, LOW);
    delay (20);
    iobyte (0xAC);
    iobyte (0x53);
    byte = iobyte (0x00);
    if (byte != 0x53) 
    {
        prtf (0, "PRG ERR");
        ret = 1;
    } 
    else 
    {
        prtf (0, "PRG %2X", byte);
        ret = 0;
    }
    iobyte (0x00);
    return ret;
}

static int run_mode (void) 
{
    pinMode (RESET, OUTPUT);  
    pinMode (SCLK, INPUT);
    pinMode (MOSI, INPUT);
    pinMode (MISO, INPUT);
    delay (1);
    digitalWrite (RESET, LOW);
    delay (10);
    digitalWrite (RESET, HIGH);
    prtf (0, "Running");
    return 0;
}

static unsigned hexval (char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static void io4 (void)
{
    int i;
    uint8_t byte;
    for (i = 0; i < 4; i++)
    {
        byte = hexval (sgetc ());
        byte = byte << 4 | hexval (sgetc ());
        byte = iobyte (byte);
    }
    sprtf ("%02X\r\n", byte);
}

void setup() 
{
    Serial.begin(SPEED);        // start serial communication
    uView.begin();              // start microview
    uView.setFontType(1);
    uView.clear(PAGE);
    pinMode (RESET, OUTPUT);  
    pinMode (SCLK, INPUT);
    pinMode (MOSI, INPUT);
    pinMode (MISO, INPUT);

    digitalWrite (RESET, LOW);
    delay (20);
    digitalWrite (RESET, HIGH);
    prtf (0, "Ready");
}

void loop() 
{
    int c;

    c = sgetc ();
    switch (toupper(c)) 
    {
    case 'P':
        sprtf("%d\r\n", prog_mode ());
        break;

    case 'R':
        sprtf("%d\r\n", run_mode ());
        break;

    case 'E':
        sprtf("0\r\n");
        break;

    case 'I':
        io4 ();
        break;

    default:
        return;
    }
}

