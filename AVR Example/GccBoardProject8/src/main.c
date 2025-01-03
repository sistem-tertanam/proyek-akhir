#include <asf.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#define TIPE1 1
#define TIPE2 2

// ------------------- KONFIGURASI UMUM -------------------
#define BAUDRATE     9600
// BSEL=12, BSCALE=0 => ~9600 bps @2MHz (approx)
#define BSEL_VALUE   12
#define BSCALE_VALUE 0

// Maks. panjang baris pesan yang kita tampung
#define MAX_LINE_SIZE 4096

// Timeout default (ms) untuk baca 1 karakter atau 1 baris
#define CHAR_TIMEOUT_MS  50    // misal 500 ms per karakter
#define LINE_TIMEOUT_MS  500   // misal 2000 ms total per baris

// ------------------- INISIALISASI USART (Tanpa Interrupt) -------------------
static void usart_init_polling(void)
{
    // Asumsikan pakai USARTC0, pin C2=RX, C3=TX
    PORTC_DIRCLR = PIN2_bm; // RX sebagai input
    PORTC_DIRSET = PIN3_bm; // TX sebagai output

    // Atur Baud
    USARTC0_BAUDCTRLA = (uint8_t)(BSEL_VALUE & 0xFF);
    USARTC0_BAUDCTRLB = (BSCALE_VALUE << USART_BSCALE_gp) 
                        | ((BSEL_VALUE >> 8) & 0x0F);

    // Format: 8 data bits, no parity, 1 stop bit
    USARTC0_CTRLC = USART_CHSIZE_8BIT_gc;

    // Enable RX & TX (tanpa interrupt)
    USARTC0_CTRLB = USART_RXEN_bm | USART_TXEN_bm;
}

#define USART_SERIAL_EXAMPLE             &USARTC0
#define USART_SERIAL_EXAMPLE_BAUDRATE    9600
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false
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

// ------------------- FUNGSI GETCHAR DENGAN TIMEOUT -------------------
/*
 * Menunggu sampai data masuk atau hingga 'timeout_ms'.
 * Return: 
 *   >= 0  => karakter yang diterima
 *   -1    => timeout (tidak ada data hingga waktu habis)
 */
static int16_t usart_getchar_timeout(int timeout_ms)
{
    // Kita akan menunggu sampai RXCIF set,
    // atau sampai 'timeout_ms' tercapai.
    for (int i = 0; i < timeout_ms; i++) {
        if (USARTC0_STATUS & USART_RXCIF_bm) {
            // Data tersedia
            return USARTC0_DATA;
        }
    }
    return -1; // menandakan timeout
}

// ------------------- FUNGSI PUTCHAR (BLOCKING) -------------------
static void usart_putchar_polling(char c)
{
    while (!(USARTC0_STATUS & USART_DREIF_bm)) {
        // tunggu data register empty
    }
    USARTC0_DATA = c;
}

// ------------------- FUNGSI KIRIM STRING (BLOCKING) -------------------
static void usart_send_string(const char* str)
{
    while (*str) {
        usart_putchar_polling(*str++);
    }
}

// ------------------- BACA 1 BARIS (HINGGA '\n') DENGAN TIMEOUT -------------------
/*
 * Mengumpulkan karakter hingga '\n' (newline) atau sampai total 'line_timeout_ms'.
 * Setiap karakter juga tidak boleh melebihi 'char_timeout_ms'.
 * 
 * Return:
 *  1 => Berhasil dapat 1 baris
 *  0 => Timeout (gagal)
 */
static uint8_t usart_read_line_timeout(char* dest, int maxlen,
int char_timeout_ms,
int line_timeout_ms)
{
	int idx = 0;
	int elapsed_ms = 0;
	int baca = 0;

	while (1) {
		// Attempt to read one character with a CHAR timeout
		int c = usart_getchar_timeout(char_timeout_ms);

		if (c > 0 ) {  // Valid character received
			baca++;
			elapsed_ms = 0;
			
			// If '`\n', line is complete
			if ((char)c == '\n') {
				dest[idx] = '\0';  // Null-terminate the string
				char buf[32];
				snprintf(buf, sizeof(buf), "break \\n after %d chars", baca);
				gfx_mono_draw_string(buf, 0, 8, &sysfont);
				return 1;
			}

			// Save character to buffer if there's space
			if (idx < (maxlen - 1)) {
				dest[idx++] = (char)c;
			}
		} else {
			// Increment elapsed time if no character is received
			elapsed_ms += char_timeout_ms;
		}

		// Check if the total line timeout is exceeded
		if (elapsed_ms >= line_timeout_ms) {
			return 0;  // Timeout reached
		}
		
		
		// Debug output for each character
		char buf[32];
		snprintf(buf, sizeof(buf), "char: %c %d %d", (char)c, c, elapsed_ms);
		gfx_mono_draw_string(buf, 0, 0, &sysfont);
	}
}

// ------------------- MAIN -------------------
int main(void)
{
	int tipe = 2;
    // 1. Inisialisasi dasar
    sysclk_init();
    board_init();
    gfx_mono_init();       // LCD
    cpu_irq_disable();     // pastikan interrupt global off (kita pakai polling)
    
    // Jika LCD butuh backlight
    gpio_set_pin_high(NHD_C12832A1Z_BACKLIGHT);

    // 2. Inisialisasi USART
    usart_init_polling();

    // 3. Kirim pesan awal
    usart_send_string("XMEGA Started.\n");
	gfx_mono_draw_string("done sending!", 0, 8, &sysfont);
	
	set_up_arduino_config();

    // 4. Variabel & buffer
    uint16_t xmega_counter = 0;
    char line_buf[MAX_LINE_SIZE];

    while (1) {
        // Baca satu baris dengan mekanisme timeout
        uint8_t ok = usart_read_line_timeout(line_buf, MAX_LINE_SIZE,CHAR_TIMEOUT_MS, LINE_TIMEOUT_MS);\
        if (!ok) {
            // Timeout terjadi => kita tidak menerima newline tepat waktu
            // Di sini kita bisa melakukan hal lain, misal kirim notifikasi
            
			if (tipe==1){
				gfx_mono_draw_string("Ngirim DATA!", 0, 16, &sysfont);
				usart_send_string("LED:0 PIR:1 L:0\n");
			}else if (tipe==2){
				gfx_mono_draw_string("Ngirim DATA!", 0, 16, &sysfont);
				usart_send_string("T:25 H:20 Q:1 W:1 F:0 B:0\n");
			}
			//usart_send_string("XMEGA: (timeout/no data)\n");
            
            // (Opsional) Tampilkan "no data" di LCD
            gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
            gfx_mono_draw_string("No data from Arduino!", 0, 0, &sysfont);

            delay_ms(1000); // jeda 1 detik, lalu coba lagi
            continue;
        }

        // Jika ok == 1 => kita sudah dapat satu baris
        /*usart_send_string("XMEGA received: ");
        usart_send_string(line_buf);
        usart_send_string("\n");*/

        // Bersihkan area LCD (opsional)
        gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
        gfx_mono_draw_string("From Arduino:", 0, 0, &sysfont);
        gfx_mono_draw_string(line_buf, 0, 8, &sysfont);

        // Balas => "XMEGA: {counter}"
        char buf[32];
        snprintf(buf, sizeof(buf), "..XMEGA: %u\n", xmega_counter++);
        usart_send_string(buf);

        // Tampilkan juga di LCD baris berikutnya
        gfx_mono_draw_string("Reply:", 0, 16, &sysfont);
        gfx_mono_draw_string(buf, 0, 24, &sysfont);

        // Jeda kecil
        delay_ms(10);
    }
}