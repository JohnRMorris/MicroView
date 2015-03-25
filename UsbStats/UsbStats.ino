#include <MicroView.h>
#include <stdint.h>

#define X 64
#define Y 48
#define SPEED 19200
#define RMT_SCREENS 10
#define MAX_LINES 5

static char rmt_screens[RMT_SCREENS][MAX_LINES][11];
static char lines[MAX_LINES][11];
static unsigned long t_prev = 0;


#define PRR    *((volatile uint8_t *)0x64)
#define ADCL   *((volatile uint8_t *)0x78)
#define ADCH   *((volatile uint8_t *)0x79)
#define ADCSRA *((volatile uint8_t *)0x7A)
#define ADCSRB *((volatile uint8_t *)0x7B)
#define ADCMUX *((volatile uint8_t *)0x7C)

void setup() 
{
    int i, j;
    for (i = 0; i < RMT_SCREENS; i++)
    {
        for (j = 0; j < MAX_LINES; j++)
        {
            rmt_screens[i][j][0] = 0;
        }
    }
    Serial.begin(SPEED);        // start serial communication
    uView.begin();              // start microview
    ADCSRB = 0x00;
    ADCMUX = 0xC8;
    ADCSRA = 0xC7;
    // analogReference (INTERNAL);
}

static const int rows[] = {0, 13, 22, 31, 40};

static int get_remote_screen (int index)
{
    int i;
    for (i = 0; i < MAX_LINES; i++)
    {
        if (rmt_screens[index][i][0])
            break;
    }
    if (i >= MAX_LINES)
        return 0;
    for (i = 0; i < MAX_LINES; i++)
        strcpy (lines[i], rmt_screens[index][i]);
    return 1;
}

static void get_uptime_screen (void)
{
    static unsigned rollovers = 0;
    static uint32_t prev_uptime = 0;
    uint16_t days, hours, minutes, seconds;
    uint64_t t;
    
    uint32_t uptime = millis();
    if (uptime < prev_uptime)
        rollovers += 1;
    prev_uptime = uptime;
    t = rollovers;
    t <<= 32ULL;
    t += uptime;
    t /= 1000;
    days = t / 86400ULL;
    t %= 86400ULL;
    hours = t / 3600;
    t %= 3600;
    minutes = t / 60;
    t %= 60;
    seconds = t;
    strcpy (lines[0], "MV runtime");
    sprintf (lines[1], " %4d day", days);
    sprintf (lines[2], " %4d hrs", hours);
    sprintf (lines[3], " %4d min", minutes);
    sprintf (lines[4], " %4d sec", seconds);
}

static void get_temperature_screen (void)
{
    uint8_t adcl, adch;
    int adc;
    ADCSRA = 0xC7;
    strcpy (lines[0], " Temp. ADC");
    while (ADCSRA & 0x40)
        ;
    adcl = ADCL;
    adch = ADCH;
    adc = adch;
    adc = (adc << 8) + adcl;
    //sprintf (lines[1], "%02X  %02X  %02X", ADCMUX, ADCSRA, ADCSRB);
    //sprintf (lines[2], "%02X  %02X  %02X", PRR, adcl, adch);
    sprintf (lines[1], "ADC = %4d", adc);
    int millivolts = adc * 1.1 / 1024 * 1000 + 0.5;
    sprintf (lines[2], "%6d mV", millivolts);
    int degc = 21 + (millivolts - 367);
    int degf = degc * 9 / 5.0 + 32.0 + 0.5;
    sprintf (lines[3], "%6d C", degc);
    sprintf (lines[4], "%6d F", degf);
}

#define PREDEF_SCREENS 2

static void get_next_screen (void)
{
    static unsigned char screen_no = 0;
	static unsigned char update = 0;
    for (;;)
    {
        if (update)
            screen_no = (screen_no + 1) % (PREDEF_SCREENS + RMT_SCREENS);
        switch (screen_no)
        {
        case 0:
            get_uptime_screen ();
            update = 1 - update;
            return;

        case 1:
            get_temperature_screen ();
            update = 1 - update;
            return;

        default:
            if (get_remote_screen (screen_no - PREDEF_SCREENS))
            {
                update = 1 - update;
                return;
            }
        }
    }
}

static void display_next (void)
{
    get_next_screen ();
    uView.clear(PAGE);
    for (int i = 0; i < MAX_LINES; i++)
    {
        uView.setCursor(0, rows[i]);
    	uView.print(lines[i]);
    }
    uView.lineH(0, 9, 64);  
    uView.flipHorizontal (true);          
    uView.flipVertical (true);          
    uView.display ();
}

static void read_remote (void)
{
    char buff[64];
    int len = Serial.readBytesUntil ('\r', buff, 63);
    buff[len] = 0;
    switch (buff[0])
    {
    default:
        int scr = buff[0] - '0';
        if (scr < 0 || scr >= RMT_SCREENS)
            return;
        char *l;
        int i;
        l = strtok (buff + 1, ",");
        for (i = 0; i < MAX_LINES && l; i++, l = strtok (NULL, ","))
        {
            strncpy (rmt_screens[scr][i], l, 10);
            rmt_screens[scr][i][10] = 0;
        }
    }        
}

void loop() {
    display_next ();
    for (;;)
    {
        if (millis() - t_prev >= 1000)
        {
            t_prev = t_prev + 1000;
            display_next ();
        }
        if (Serial.available())
        {
            read_remote ();
        }
    }
}
