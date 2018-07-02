#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>

#include "i2c.h"

// the i2c address
#define VCNL4000_ADDRESS 0x13

// commands and constants
#define VCNL4000_COMMAND 0x80
#define VCNL4000_PRODUCTID 0x81
#define VCNL4000_IRLED 0x83
#define VCNL4000_AMBIENTPARAMETER 0x84
#define VCNL4000_AMBIENTDATA 0x85
#define VCNL4000_PROXIMITYDATA 0x87
#define VCNL4000_SIGNALFREQ 0x89
#define VCNL4000_PROXINITYADJUST 0x8A

#define VCNL4000_3M125 0
#define VCNL4000_1M5625 1
#define VCNL4000_781K25 2
#define VCNL4000_390K625 3

#define VCNL4000_MEASUREAMBIENT 0x10
#define VCNL4000_MEASUREPROXIMITY 0x08
#define VCNL4000_AMBIENTREADY 0x40
#define VCNL4000_PROXIMITYREADY 0x20

static QueueHandle_t uart_txq;		// TX queue for UART
static I2C_Control i2c;			// I2C Control struct

static void
uart_puts(const char *s) {
	
	for ( ; *s; ++s ) {
		// blocks when queue is full
		xQueueSend(uart_txq,s,portMAX_DELAY); 
	}
}

static void
uart_task(void *args __attribute__((unused))) {
	char ch;

	for (;;) {
		// Receive char to be TX
		if ( xQueueReceive(uart_txq,&ch,500) == pdPASS ) {
			while ( !usart_get_flag(USART1,USART_SR_TXE) )
				taskYIELD();	// Yield until ready
			usart_send(USART1,ch);
		}
		// Toggle LED to show signs of life
		gpio_toggle(GPIOC,GPIO13);
	}
}

static void uart_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);

	// UART TX on PA9 (GPIO_USART1_TX)
	gpio_set_mode(GPIOA,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		GPIO_USART1_TX);

	usart_set_baudrate(USART1,38400);
	usart_set_databits(USART1,8);
	usart_set_stopbits(USART1,USART_STOPBITS_1);
	usart_set_mode(USART1,USART_MODE_TX);
	usart_set_parity(USART1,USART_PARITY_NONE);
	usart_set_flow_control(USART1,USART_FLOWCONTROL_NONE);
	usart_enable(USART1);
  // Create a queue for data to transmit from UART
	uart_txq = xQueueCreate(256,sizeof(char));
}

/*
			i2c_start_addr(&i2c,addr,Write);
			i2c_write(&i2c,value&0x0FF);
			i2c_stop(&i2c);

			i2c_start_addr(&i2c,addr,Read);
			byte = i2c_read(&i2c,true);
			i2c_stop(&i2c);
*/
uint8_t currentReadByte;
bool waitForProx = true;
char printByte[24];
static void readProx() {
  for(;;) {
    uart_puts("Starting sensor read!\r\n");
    // Configure I2C1
    i2c_configure(&i2c,I2C1,1000);
    i2c_start_addr(&i2c,VCNL4000_ADDRESS,Write);
    i2c_write_restart(&i2c,VCNL4000_PRODUCTID,VCNL4000_ADDRESS);
    currentReadByte = i2c_read(&i2c,true);
    i2c_stop(&i2c);

    if ((currentReadByte & 0xF0) != 0x10) {
      uart_puts("Could not find sensor!\r\n");
    } else {
      uart_puts("Sensor connected!\r\n");
    }

    i2c_start_addr(&i2c,VCNL4000_ADDRESS,Write);
    i2c_write_restart(&i2c,VCNL4000_IRLED,VCNL4000_ADDRESS);
    i2c_write(&i2c, 0x00);
    i2c_write(&i2c, 0x14);
    i2c_stop(&i2c);    
    uart_puts("Set IR LED current to 200mA\r\n");

    i2c_start_addr(&i2c,VCNL4000_ADDRESS,Write);
    i2c_write_restart(&i2c,VCNL4000_PROXINITYADJUST,VCNL4000_ADDRESS);
    i2c_write(&i2c, 0x81);
    i2c_stop(&i2c);    
    uart_puts("Set Proximity adjustment register to 0x81\r\n");

    i2c_start_addr(&i2c,VCNL4000_ADDRESS,Write);
    i2c_write_restart(&i2c,VCNL4000_COMMAND,VCNL4000_ADDRESS);
    i2c_write(&i2c, VCNL4000_MEASUREPROXIMITY);
    i2c_stop(&i2c);    
    uart_puts("Set Command to measure proximity\r\n");

    while(waitForProx) {
      uart_puts("Waiting for prox measurement...\r\n");
      i2c_start_addr(&i2c,VCNL4000_ADDRESS,Write);
      i2c_write_restart(&i2c,VCNL4000_COMMAND,VCNL4000_ADDRESS);
      currentReadByte = i2c_read(&i2c,true);
      i2c_stop(&i2c);
      uart_puts(currentReadByte);

      if((currentReadByte & VCNL4000_PROXIMITYREADY) == 0x20) {
        waitForProx = false;
      } else {
        vTaskDelay(pdMS_TO_TICKS(20));
      }
    }

    uart_puts("Prox data ready!");


  //     return read16(VCNL4000_PROXIMITYDATA);


    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int main(void) {
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
  
  uart_setup();

  rcc_periph_clock_enable(RCC_GPIOB);	// I2C
	rcc_periph_clock_enable(RCC_I2C1);	// I2C
	gpio_set_mode(GPIOB,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		GPIO6|GPIO7);			// I2C
	gpio_set(GPIOB,GPIO6|GPIO7);		// Idle high
  // AFIO_MAPR_I2C1_REMAP=0, PB6+PB7
	gpio_primary_remap(0,0); 


  xTaskCreate(uart_task,"UART",100,NULL,configMAX_PRIORITIES-1,NULL);
  xTaskCreate(readProx, "READ_PROX", 100, NULL, configMAX_PRIORITIES-2, NULL);
  
  uart_puts("Program start\r\n");
  
  vTaskStartScheduler();
  
  for(;;);
  return 0;
}

