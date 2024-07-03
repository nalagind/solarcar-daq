#pragma once
#include <SPI.h>
#include <SdFat.h>
#include "..\lib\SdFat\src\SpiDriver\SdSpiBaseClass.h"
#include "..\lib\SdFat\src\BufferedPrint.h"
#include "logger.h"

#define SD_CS PC4
#define LED_G PB5
#define LED_B PB6
#define LED_R PB7

#define SPI_CLOCK SD_SCK_MHZ(50)

SPI_HandleTypeDef* hspi1_ptr;  // Define the SPI handle
DMA_HandleTypeDef hdma_spi_tx;
DMA_HandleTypeDef hdma_spi_rx;

void DMA_Init(void) {
  __HAL_RCC_DMA2_CLK_ENABLE();  // Enable DMA clock

  hdma_spi_tx.Instance = DMA2_Stream3;
  hdma_spi_tx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi_tx.Init.Mode = DMA_NORMAL;
  hdma_spi_tx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  
  hdma_spi_rx.Instance = DMA2_Stream0;
  hdma_spi_rx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_spi_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi_rx.Init.Mode = DMA_NORMAL;
  hdma_spi_rx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_spi_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma_spi_tx) != HAL_OK) {
      Serial.println("DMA TX initialization failed");
      while (1);
  }
  if (HAL_DMA_Init(&hdma_spi_rx) != HAL_OK) {
      Serial.println("DMA RX initialization failed");
      while (1);
  }
  
  __HAL_LINKDMA(hspi1_ptr, hdmarx, hdma_spi_rx);  // Link DMA handle to SPI handle
  __HAL_LINKDMA(hspi1_ptr, hdmatx, hdma_spi_tx);  // Link DMA handle to SPI handle

  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  
  digitalWrite(LED_G, LOW);
}

extern "C" void DMA2_Stream3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi_tx);
}

extern "C" void DMA2_Stream0_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi_rx);
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  
}

extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  
}

class DmaSpi : public SdSpiBaseClass {
public:
  void begin(SdSpiConfig config) override {
    SPI.setMISO(PA6);
    SPI.setMOSI(PA7);
    SPI.setSCLK(PA5);
    pinMode(SD_CS, OUTPUT);
    SPI.begin();
    Serial.print("spi begin ok...");

    hspi1_ptr = SPI.getHandle();
    Serial.print("spi get handle ok...");

    DMA_Init();
    Serial.print("dma init ok...");
  }

  void activate() override {
    // Activate the SPI hardware
    SPI.beginTransaction(m_spiSettings);
  }

  void deactivate() override {
    // Deactivate the SPI hardware
    SPI.endTransaction();
  }

  void end() override {
    SPI.end();
  }

  uint8_t receive() override {
    uint8_t r = SPI.transfer(0xFF);
    return r;
  }

  uint8_t receive(uint8_t* buf, size_t count) override {
    memset(buf, 0xFF, count);
    if (HAL_SPI_Receive_DMA(hspi1_ptr, buf, count) == HAL_OK) {
      while (HAL_DMA_GetState(&hdma_spi_rx) != HAL_DMA_STATE_READY) {}
      return 0;
    }
    return 1;
  }

  void send(uint8_t data) override {
    SPI.transfer(data);
  }

  void send(const uint8_t* buf, size_t count) override {
    if (count > 512) {return;}

    uint8_t buf_cpy[512];
    memcpy(buf_cpy, buf, count);
    uint8_t rxbuf[512];

    if (HAL_SPI_TransmitReceive_DMA(hspi1_ptr, buf_cpy, rxbuf, count) == HAL_OK) {
      digitalWrite(LED_R, LOW);
      while (HAL_DMA_GetState(&hdma_spi_tx) != HAL_DMA_STATE_READY) {}
      digitalWrite(LED_R, HIGH);
    }
  }

  void setSckSpeed(uint32_t maxSck) override {
    m_spiSettings = SPISettings(maxSck, MSBFIRST, SPI_MODE0);
  }

private:
  SPISettings m_spiSettings;
};

DmaSpi dmaSpi;

extern SdFs SD;
FsFile file;

extern BufferedPrintPlus<FsFile, 255, true, 256> sdbp;

bool appendFile(const char *filename, const char *message) {
  file = SD.open(filename, FILE_WRITE); //FILEWRITE includes flags for creating file, appending, and R&W
  if (!file) {
    Serial.println("Failed to open file for appending");
    return false;
  }

  if (!file.println(message)) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool writeFile(const char *filename, const char *message) {
  if (!file.open(filename, FILE_WRITE)) {
    Serial.println("Failed to open file for writing");
    return false;
  }

  if (!file.print(message)) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool sd_init(uint32_t chipSelectPin = PC4, uint32_t miso = PA6, uint32_t mosi = PA7, uint32_t sclk = PA5, const char *filename = "daq.csv") {
  //need to change SPI pins for timeconsumer board
  SPI.setMISO(miso); //PA6
  SPI.setMOSI(mosi); //PA7
  SPI.setSCLK(sclk); //PA5

  if (!SD.begin(SdSpiConfig(chipSelectPin, DEDICATED_SPI, SPI_CLOCK, &dmaSpi))) { //PC4
    Serial.println("SD initialization failed!");
    return false;
  } else {
    Serial.print("SD found...");
  }
  if (!file.open(filename, O_RDWR | O_CREAT | O_TRUNC)) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  sdbp.begin(&file);

  if (!file.println("DAQDAQDAQDAQDAQDAQDAQDAQ")) {
    Serial.println("Writing to file failed!");
    return false;
  }
  file.sync();

  Serial.println("...and mounted!");
  return true;
}