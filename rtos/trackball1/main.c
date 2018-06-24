#include "freeRTOS.h"
#include "task.h"
#include "queue.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

static QueueHandle_t uart_txq;		// TX queue for UART

extern void vApplicationStackOverflowHook(
	xTaskHandle *pxTask,
	signed portCHAR *pcTaskName);

void

// Catch for stack overflow from tasks
vApplicationStackOverflowHook(
  xTaskHandle *pxTask __attribute((unused)),
  signed portCHAR *pcTaskName __attribute((unused))
) {
	for(;;);	// Loop forever here..
}

static void
uart_puts(const char *s) {
	
	for ( ; *s; ++s ) {
		// blocks when queue is full
		xQueueSend(uart_txq,s,portMAX_DELAY); 
	}
}

static void redLED(void* args __attribute((unused))) {
  uint32_t notificationValue;

  for(;;) {
    xTaskNotifyWait(
      0x00,
      UINT8_MAX,
      &notificationValue,
      portMAX_DELAY  
    );
    if((notificationValue & 0x01) != 0) {
      uart_puts("Red LED start");
      gpio_set(GPIOA, GPIO1);
      vTaskDelay(pdMS_TO_TICKS(250));
      gpio_clear(GPIOA, GPIO1);
      xTaskNotify("greenLED", 0x01, eSetValueWithOverwrite);
    }
  }
}

static void greenLED(void* args __attribute((unused))) {
  uint32_t notificationValue;
  for(;;) {
    xTaskNotifyWait(
      0x00,
      UINT8_MAX,
      &notificationValue,
      portMAX_DELAY  
    );
    if((notificationValue & 0x01) != 0) {
      uart_puts("Green LED start");
      gpio_set(GPIOA, GPIO2);
      vTaskDelay(pdMS_TO_TICKS(250));
      gpio_clear(GPIOA, GPIO2);
      xTaskNotify("blueLED", 0x01, eSetValueWithOverwrite);
    }
  }
}

static void blueLED(void* args __attribute((unused))) {
  uint32_t notificationValue;
  for(;;) {
    xTaskNotifyWait(
      0x00,
      UINT8_MAX,
      &notificationValue,
      portMAX_DELAY  
    );
    if((notificationValue & 0x01) != 0) {
      uart_puts("Blue LED start");
      gpio_set(GPIOA, GPIO3);
      vTaskDelay(pdMS_TO_TICKS(250));
      gpio_clear(GPIOA, GPIO3);
      xTaskNotify("whiteLED", 0x01, eSetValueWithOverwrite);
    }
  }
}

static void whiteLED(void* args __attribute((unused))) {
  uint32_t notificationValue;
  for(;;) {
    xTaskNotifyWait(
      0x00,
      UINT8_MAX,
      &notificationValue,
      portMAX_DELAY  
    );
    if((notificationValue & 0x01) != 0) {
      uart_puts("White LED start");
      gpio_set(GPIOA, GPIO6);
      vTaskDelay(pdMS_TO_TICKS(250));
      gpio_clear(GPIOA, GPIO6);
      xTaskNotify("redLED", 0x01, eSetValueWithOverwrite);
    }
  }
}

int main(void) {

  rcc_clock_setup_in_hse_8mhz_out_72mhz();
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

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO3);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO6);

  xTaskCreate(redLED, "redLED", 100, NULL, configMAX_PRIORITIES-1, NULL);
  xTaskCreate(greenLED, "greenLED", 100, NULL, configMAX_PRIORITIES-1, NULL);
  xTaskCreate(blueLED, "blueLED", 100, NULL, configMAX_PRIORITIES-1, NULL);
  xTaskCreate(whiteLED, "whiteLED", 100, NULL, configMAX_PRIORITIES-1, NULL);

  vTaskStartScheduler();

  uart_puts("Program start");

  xTaskNotify("redLED", 0x01, eSetValueWithOverwrite);
  
  uart_puts("Notification sent to red LED");

  for(;;);
  return 0;
}