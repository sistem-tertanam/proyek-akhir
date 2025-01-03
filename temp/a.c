#include <asf.h>
#include <stdio.h>
#include <string.h>

#include <util/delay.h>
#include <stdlib.h>

#define CLOSE_RANGE 3               // Jarak dekat maksimal (cm)
#define MID_RANGE 5                 // Jarak menengah maksimal (cm)
#define FAR_RANGE 8                 // Jarak jauh maksimal (cm)
#define MAX_RANGE 11              // Jarak untuk menghapus penguncian

#define BUZZER_PIN PIN0_bm          // Pin untuk buzzer pada PC0
#define CLOSE_BUZZER_DURATION 2500  // Durasi bunyi kontinu untuk jarak dekat (ms)

void setup_main_timer(void);
void setup_buzzer_timer(void);
long calculate_distance_cm(uint16_t pulse_duration);
void control_buzzer(long distance_cm);
void toggle_buzzer(void);                       // Fungsi untuk mengaktifkan buzzer dengan interrupt
uint16_t get_dynamic_delay(long distance_cm);   // Fungsi untuk menentukan delay dinamis
void delay_ms_runtime(uint16_t ms);             // Fungsi delay berbasis runtime

// Variabel status buzzer dan penguncian

#define USART_SERIAL_EXAMPLE             &USARTC0
#define USART_SERIAL_EXAMPLE_BAUDRATE    9600
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

static char strbuf[201];
static char reads[100];

void setUpSerial()
{
 // Baud rate selection
 // BSEL = (2000000 / (2^0 * 16*9600) -1 = 12.0208... ~ 12 -> BSCALE = 0
 // FBAUD = ( (2000000)/(2^0*16(12+1)) = 9615.384 -> mendekati lah ya
 
 USARTC0_BAUDCTRLB = 0; //memastikan BSCALE = 0
 USARTC0_BAUDCTRLA = 0x0C; // 12
 
 //USARTC0_BAUDCTRLB = 0; //Just to be sure that BSCALE is 0
 //USARTC0_BAUDCTRLA = 0xCF; // 207
 
 //Disable interrupts, just for safety
 USARTC0_CTRLA = 0;
 //8 data bits, no parity and 1 stop bit
 USARTC0_CTRLC = USART_CHSIZE_8BIT_gc;
 
 //Enable receive and transmit
 USARTC0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;
}

void sendChar(char c)
{
 
 while( !(USARTC0_STATUS & USART_DREIF_bm) ); //Wait until DATA buffer is empty
 
 USARTC0_DATA = c;
 
}

char receiveChar()
{
 int cnt = 0;
 while( !(USARTC0_STATUS & USART_RXCIF_bm)&& cnt<2046) cnt++; //Wait until receive finish
 return USARTC0_DATA;
}

void set_up_arduino_config(){
 board_init();
 sysclk_init();
 gfx_mono_init();
 cpu_irq_enable();
 
 gpio_set_pin_high(LCD_BACKLIGHT_ENABLE_PIN);
 
 PORTC_OUTSET = PIN3_bm; // PC3 as TX
 PORTC_DIRSET = PIN3_bm; //TX pin as output
 
 PORTC_OUTCLR = PIN2_bm; //PC2 as RX
 PORTC_DIRCLR = PIN2_bm; //RX pin as input
 
 setUpSerial();
 
 static usart_rs232_options_t USART_SERIAL_OPTIONS = {
  .baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
  .charlength = USART_SERIAL_CHAR_LENGTH,
  .paritytype = USART_SERIAL_PARITY,
  .stopbits = USART_SERIAL_STOP_BIT
 };
 
 usart_init_rs232(USART_SERIAL_EXAMPLE, &USART_SERIAL_OPTIONS);
 
 ioport_set_pin_dir(J2_PIN0, IOPORT_DIR_OUTPUT);
}

void clearStaleData() {
 while (USARTC0_STATUS & USART_RXCIF_bm) {
  volatile char discard = USARTC0_DATA; // Read and discard
 }
}

void sendString(char *text)
{
 if (USARTC0_STATUS & USART_RXCIF_bm){
  char buffarray[256];
  snprintf(buffarray, sizeof(buffarray), "bisa baca ");
  gfx_mono_draw_string(buffarray, 0, 8, &sysfont);
  return;
 }
 while(*text)
 {
  sendChar(*text++);
  //usart_putchar(USART_SERIAL_EXAMPLE, *text++);
 }
 
 // Wait for transmission to complete
 while (!(USARTC0_STATUS & USART_TXCIF_bm));

 // Clear TXCIF flag
 USARTC0_STATUS |= USART_TXCIF_bm;
}

/*static char buf[256];*/
void receiveString(char* buf)
{
 clearStaleData();
 int i = 0;
 int cnt=0;
 // Wait for the first character
 while (!(USARTC0_STATUS & USART_RXCIF_bm)&& cnt<4096) {
  cnt++;
 }
 if (cnt>=4096){
  char buffarray[256];
  snprintf(buffarray, sizeof(buffarray), "gk bisa baca");
  gfx_mono_draw_string(buffarray, 0, 8, &sysfont);
  return;
 }
 while(i<128){
  char inp = receiveChar();
  if(inp=='\n') break;
  else buf[i++] = inp;
 }
 buf[i] = '\0'; // Null-terminate the string
}

//////////////////////////////////////////////////////////////////

int main(void)
{
 board_init();
 sysclk_init();
 gfx_mono_init();  // Inisialisasi LCD
 cpu_irq_enable();

 set_up_arduino_config();
 
 gpio_set_pin_high(NHD_C12832A1Z_BACKLIGHT); // Mengaktifkan backlight LCD

 setup_main_timer();

 while (1)
 {
  uint16_t start_time = 0, end_time = 0;
  long distance_cm = 0;

  // Kirim trigger ke sensor dengan delay akurat
  PORTB.DIRSET = PIN0_bm;   // Set PB0 sebagai output untuk trigger
  PORTB.OUTCLR = PIN0_bm;
  _delay_us(2);
  PORTB.OUTSET = PIN0_bm;
  _delay_us(10);
  PORTB.OUTCLR = PIN0_bm;

  // Atur PB0 sebagai input untuk menerima echo
  PORTB.DIRCLR = PIN0_bm;

  // Tunggu sampai echo naik (rising edge)
  while (!(PORTB.IN & PIN0_bm)) {
   if (tc_read_count(&TCC0) > 60000) break;
  }
  start_time = tc_read_count(&TCC0);  // Catat waktu mulai

  // Tunggu sampai echo turun (falling edge)
  while (PORTB.IN & PIN0_bm) {
   if (tc_read_count(&TCC0) > 60000) break;
  }
  end_time = tc_read_count(&TCC0);  // Catat waktu selesai

  // Hitung durasi pulse
  if (end_time >= start_time) {
   distance_cm = calculate_distance_cm(end_time - start_time);
   } else {
   distance_cm = calculate_distance_cm((0xFFFF - start_time) + end_time);
  }

  // Tampilkan jarak di LCD
  char distance_str[10];
  dtostrf(distance_cm, 6, 2, distance_str); // Konversi jarak ke string
  
  char sendarray[256];
  char buffarray[256];
  snprintf(sendarray, sizeof(sendarray), "LALA: %s cm    \n", distance_str);
  sendString(sendarray);
  gfx_mono_draw_string(sendarray, 0, 0, &sysfont);
  
  snprintf(buffarray, sizeof(buffarray), "done sending");
  gfx_mono_draw_string(buffarray, 0, 8, &sysfont);
  char* terima[128];
  receiveString(terima);
  
  snprintf(buffarray, sizeof(buffarray), "done receive");
  gfx_mono_draw_string(buffarray, 0, 8, &sysfont);
  
  snprintf(buffarray, sizeof(buffarray), "m: %s \n", terima);
  gfx_mono_draw_string(buffarray, 0, 16, &sysfont);

  // Dapatkan delay yang dinamis berdasarkan jarak sebelumnya
  //uint16_t dynamic_delay = get_dynamic_delay(distance_cm);
  //delay_ms_runtime(dynamic_delay);  // Delay antar pengukuran berdasarkan jarak
 }
}

long calculate_distance_cm(uint16_t pulse_duration)
{
 return (pulse_duration * 0.01715) / 2;
}

void setup_main_timer(void)
{
 tc_enable(&TCC0);
 tc_set_wgm(&TCC0, TC_WG_NORMAL);
 tc_write_period(&TCC0, 0xFFFF);
 tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1_gc); // Tanpa prescaler, setiap tick 0.5 µs
}

void setup_buzzer_timer(void)
{
 tc_enable(&TCC1);
 tc_set_wgm(&TCC1, TC_WG_NORMAL);
 tc_write_clock_source(&TCC1, TC_CLKSEL_DIV64_gc); // Prescaler untuk kontrol frekuensi buzzer
 tc_set_overflow_interrupt_callback(&TCC1, toggle_buzzer);
 tc_set_overflow_interrupt_level(&TCC1, TC_INT_LVL_LO);
 pmic_enable_level(PMIC_LVL_LOW);
 //PORTC.DIRSET = BUZZER_PIN;  // Set PC0 sebagai output untuk buzzer
}

// Fungsi untuk mendapatkan delay dinamis berdasarkan jarak
uint16_t get_dynamic_delay(long distance_cm)
{
 if (distance_cm <= CLOSE_RANGE) {
  return 50;  // Delay lebih pendek untuk jarak dekat
 }
 else if (distance_cm <= MID_RANGE) {
  return 150; // Delay menengah untuk jarak menengah
 }
 else if (distance_cm <= FAR_RANGE) {
  return 300; // Delay lebih lama untuk jarak jauh
 }
 else {
  return 500; // Delay maksimal untuk jarak di luar rentang
 }
}

// Fungsi delay runtime menggunakan loop
void delay_ms_runtime(uint16_t ms)
{
 while (ms--) {
  _delay_ms(1);  // Delay 1 ms pada setiap iterasi loop
 }
}