#include <can2040.h>
#include <hardware/regs/intctrl.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

static struct can2040 cbus;
static QueueHandle_t queue;

void send_task(void *params){
    printf("CAN Send Task Started\n");
    struct can2040_msg msg = {0};
    msg.id = 0;
    msg.dlc = 8;
    msg.data32[0] = 0x01234567;
    msg.data32[1] = 0x89abcdef;

    while(1){
        int ok = can2040_transmit(&cbus, &msg);
        printf("TX enqueue: %d\n", ok);
        busy_wait_ms(10);   // Even very low delay allows throughput on low-priority
    }
}

void receive_task(void *params){
    printf("CAN Receive Task Started\n");
    struct can2040_msg msg;
    while(1){
        // printf("Waiting for message...\n");
        BaseType_t result = xQueueReceive(queue, &msg, pdMS_TO_TICKS(100));
        // printf("xQueueReceive returned: %d\n", result);
        if(result == pdTRUE){
            printf("Message received: ID=0x%X DLC=%d DATA=0x%08X%08X\n", msg.id, msg.dlc, msg.data32[0], msg.data32[1]);
        }
    }
}

static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendToBackFromISR(queue, msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void PIOx_IRQHandler(void)
{
    can2040_pio_irq_handler(&cbus);
}

void canbus_setup(void)
{
    queue = xQueueCreate(12, sizeof(struct can2040_msg));

    uint32_t pio_num = 0;
    uint32_t sys_clock = 125000000, bitrate = 500000;
    uint32_t gpio_rx = 4, gpio_tx = 5;

    // Setup canbus
    can2040_setup(&cbus, pio_num);
    can2040_callback_config(&cbus, can2040_cb);

    // Enable irqs
    irq_set_exclusive_handler(PIO0_IRQ_0, PIOx_IRQHandler);
    irq_set_priority(PIO0_IRQ_0, PICO_DEFAULT_IRQ_PRIORITY - 1);
    irq_set_enabled(PIO0_IRQ_0, 1);

    // Start canbus
    can2040_start(&cbus, sys_clock, bitrate, gpio_rx, gpio_tx);
}

int main(void){
    stdio_init_all();
    canbus_setup();
    TaskHandle_t can_send_handle;
    TaskHandle_t can_receive_handle;
    xTaskCreate(send_task, "CAN Send Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &can_send_handle);
    xTaskCreate(receive_task, "CAN Receive Task", configMINIMAL_STACK_SIZE + 128, NULL, tskIDLE_PRIORITY+1, &can_receive_handle);
    vTaskStartScheduler();
    return 0;
}