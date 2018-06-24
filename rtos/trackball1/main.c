#include "freeRTOS.h"
#include "task.h"
#include "queue.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

static QueueHandle_t uart_txq;		// TX queue for UART
TaskHandle_t redHandle, greenHandle, blueHandle, whiteHandle;

static void
uart_puts(const char *s) {
	
	for ( ; *s; ++s ) {
		// blocks when queue is full
		xQueueSend(uart_txq,s,portMAX_DELAY); 
	}
}

static void redLED(void* args __attribute((unused))) { 
  uart_puts("Red LED start\r\n");
  gpio_set(GPIOA, GPIO1);
  vTaskDelay(pdMS_TO_TICKS(2000));
  gpio_clear(GPIOA, GPIO1);
  uart_puts("Red LED end\r\n");
  vTaskResume(greenHandle);
  vTaskSuspend(NULL);
}

static void greenLED(void* args __attribute((unused))) {
  vTaskDelay(250);
  uart_puts("Green LED start\r\n");
  gpio_set(GPIOA, GPIO2);
  vTaskDelay(pdMS_TO_TICKS(2000));
  gpio_clear(GPIOA, GPIO2);
  uart_puts("Green LED end\r\n");
  vTaskResume(blueHandle);
  vTaskSuspend(NULL);
}

static void blueLED(void* args __attribute((unused))) {
  vTaskDelay(250);
  uart_puts("Blue LED start\r\n");
  gpio_set(GPIOA, GPIO3);
  vTaskDelay(pdMS_TO_TICKS(2000));
  gpio_clear(GPIOA, GPIO3);
  uart_puts("Blue LED end\r\n");
  vTaskResume(whiteHandle);
  vTaskSuspend(NULL);
}

static void whiteLED(void* args __attribute((unused))) {
  vTaskDelay(250);
  uart_puts("White LED start\r\n");
  gpio_set(GPIOA, GPIO6);
  vTaskDelay(pdMS_TO_TICKS(2000));
  gpio_clear(GPIOA, GPIO6);
  uart_puts("White LED end\r\n");
  vTaskResume(redHandle);
  vTaskSuspend(NULL);
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

int main(void) {

  rcc_clock_setup_in_hse_8mhz_out_72mhz();

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO3);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO6);

  uart_setup();

  xTaskCreate(redLED, "redLED", 100, NULL, configMAX_PRIORITIES-1, redHandle);
  xTaskCreate(greenLED, "greenLED", 100, NULL, configMAX_PRIORITIES-2, greenHandle);
  xTaskCreate(blueLED, "blueLED", 100, NULL, configMAX_PRIORITIES-3, blueHandle);
  xTaskCreate(whiteLED, "whiteLED", 100, NULL, configMAX_PRIORITIES-4, whiteHandle);
	xTaskCreate(uart_task,"UART",100,NULL,configMAX_PRIORITIES-1,NULL);
  
  uart_puts("Program start\r\n");
  
  vTaskStartScheduler();
  
  for(;;);
  return 0;
}