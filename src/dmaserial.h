#include <Arduino.h>
#include <HardwareSerial.h>

#define DMA_SERIAL_RX_BUFFER_SIZE 512

extern "C" 

// UART_HandleTypeDef* huart3;
DMA_HandleTypeDef hdma_usart2_rx = {};
// DMA_HandleTypeDef hdma_usart2_tx;

class DmaSerial: public HardwareSerial {
    public:

    DmaSerial(): HardwareSerial(PA3, PA2, NC, NC) {}

    bool begin(unsigned long baud) {
        byte config = SERIAL_8N1;
        
        uint32_t databits = 0;
        uint32_t stopbits = 0;
        uint32_t parity = 0;

        // Manage databits
        switch (config & 0x07) {
            case 0x02:
            databits = 6;
            break;
            case 0x04:
            databits = 7;
            break;
            case 0x06:
            databits = 8;
            break;
            default:
            databits = 0;
            break;
        }

        if ((config & 0x30) == 0x30) {
            parity = UART_PARITY_ODD;
            databits++;
        } else if ((config & 0x20) == 0x20) {
            parity = UART_PARITY_EVEN;
            databits++;
        } else {
            parity = UART_PARITY_NONE;
        }

        if ((config & 0x08) == 0x08) {
            stopbits = UART_STOPBITS_2;
        } else {
            stopbits = UART_STOPBITS_1;
        }

        switch (databits) {
        #ifdef UART_WORDLENGTH_7B
            case 7:
            databits = UART_WORDLENGTH_7B;
            break;
        #endif
            case 8:
            databits = UART_WORDLENGTH_8B;
            break;
            case 9:
            databits = UART_WORDLENGTH_9B;
            break;
            default:
            case 0:
            Error_Handler();
            break;
        }

        uart_init(&_serial, (uint32_t)115200, databits, parity, stopbits);

        __HAL_RCC_DMA1_CLK_ENABLE();

        hdma_usart2_rx.Instance = DMA1_Stream5;
        hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK) return false;

        // hdma_usart2_tx.Instance = DMA1_Stream6;
        // hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
        // hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        // hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        // hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
        // hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        // hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        // hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        // hdma_usart2_tx.Init.Priority = DMA_PRIORITY_HIGH;
        // if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK) return false;

        __HAL_LINKDMA(&(_serial.handle), hdmarx, hdma_usart2_rx);
        // __HAL_LINKDMA(&(_serial.handle), hdmatx, hdma_usart2_tx);

        HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
        // HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
        // HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

        __HAL_UART_ENABLE_IT(&(_serial.handle), UART_IT_IDLE);

        _serial.rx_callback = _circdma_rx_event_irq;
        // _serial.rx_callback = _rx_complete_irq;
        HAL_NVIC_DisableIRQ(_serial.irq);
        if (HAL_UARTEx_ReceiveToIdle_DMA(&(_serial.handle), _serial.rx_buff, DMA_SERIAL_RX_BUFFER_SIZE) != HAL_OK) return false;
        // HAL_UART_Receive_IT(&(_serial.handle), _serial.rx_buff, DMA_SERIAL_RX_BUFFER_SIZE);
        // HAL_UART_Receive_DMA(&(_serial.handle), _serial.rx_buff, DMA_SERIAL_RX_BUFFER_SIZE);
        HAL_NVIC_EnableIRQ(_serial.irq);

        return true;
    }

    int available() override {
        return ((unsigned int)(DMA_SERIAL_RX_BUFFER_SIZE + _serial.rx_head - _serial.rx_tail)) % DMA_SERIAL_RX_BUFFER_SIZE;
    }

    int read() override {
        // if the head isn't ahead of the tail, we don't have any characters
        if (_serial.rx_head == _serial.rx_tail) {
            return -1;
        }
        
        unsigned char c = _serial.rx_buff[_serial.rx_tail];
        _serial.rx_tail = (rx_buffer_index_t)(_serial.rx_tail + 1) % DMA_SERIAL_RX_BUFFER_SIZE;
        return c;
    }
    
    template <size_t N>
    int read(char (&buf)[N], uint16_t bytes_out = 1) {
        if (_serial.rx_head == _serial.rx_tail) {
            return -1;
        }

        if (bytes_out > available()) bytes_out = available();
        if (bytes_out > N) bytes_out = N;
        
        uint16_t till_end = DMA_SERIAL_RX_BUFFER_SIZE - _serial.rx_tail;
        if (bytes_out > till_end) {
            memcpy(buf, _serial.rx_buff + _serial.rx_tail, till_end);
            memcpy(buf + till_end, _serial.rx_buff, bytes_out - till_end);
        } else {
            memcpy(buf, _serial.rx_buff + _serial.rx_tail, bytes_out);
        }
        if (bytes_out < N) buf[bytes_out] = '\0';
        _serial.rx_tail = (rx_buffer_index_t)(_serial.rx_tail + bytes_out) % DMA_SERIAL_RX_BUFFER_SIZE;

        return bytes_out;
    }

    static void _circdma_rx_event_irq(serial_t *obj) {
        if (HAL_UARTEx_GetRxEventType(&(obj->handle)) == HAL_UART_RXEVENT_IDLE
         || HAL_UARTEx_GetRxEventType(&(obj->handle)) == HAL_UART_RXEVENT_TC) {
            obj->rx_head = DMA_SERIAL_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
        }
    }

    static void _dma_tx_complete_irq(serial_t *obj) {
        
    }
};

extern "C" serial_t *get_serial_obj(UART_HandleTypeDef *huart);

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    serial_t *obj = get_serial_obj(huart);
    if (obj) {
        obj->rx_callback(obj);
    }
}

extern "C" void DMA1_Stream5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

// extern "C" void DMA1_Stream6_IRQHandler(void) {
//     HAL_DMA_IRQHandler(&hdma_usart2_tx);
// }
