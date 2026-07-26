/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * This is the STM32 HAL port for W25Q128 on SPI2.
 */

#include "sfud.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* External SPI2 handle from CubeMX */
extern SPI_HandleTypeDef hspi2;

/* User data structure for CS pin info */
typedef struct {
    GPIO_TypeDef *cs_gpiox;
    uint16_t cs_gpio_pin;
} spi_user_data, *spi_user_data_t;

static char log_buf[256];

/* Debug log functions */
void sfud_log_debug(const char *file, const long line, const char *format, ...);
void sfud_log_info(const char *format, ...);

/* Lock/unlock for SPI access (critical section) */
static void spi_lock(const sfud_spi *spi) {
    __disable_irq();
}

static void spi_unlock(const sfud_spi *spi) {
    __enable_irq();
}

/**
 * SPI write/read - combined operation
 * For write: sends write_buf, ignores read_buf
 * For read:  sends 0xFF dummy bytes, receives into read_buf
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf,
                               size_t write_size, uint8_t *read_buf, size_t read_size) {
    sfud_err result = SFUD_SUCCESS;
    spi_user_data_t spi_dev = (spi_user_data_t) spi->user_data;
    HAL_StatusTypeDef hal_status;

    if ((write_size && write_buf == NULL) || (read_size && read_buf == NULL)) {
        return SFUD_ERR_NOT_FOUND;
    }

    /* Assert CS low */
    HAL_GPIO_WritePin(spi_dev->cs_gpiox, spi_dev->cs_gpio_pin, GPIO_PIN_RESET);

    if (write_size > 0) {
        /* Send command/data */
        hal_status = HAL_SPI_Transmit(&hspi2, (uint8_t *)write_buf, write_size, 1000);
        if (hal_status != HAL_OK) {
            result = SFUD_ERR_TIMEOUT;
            goto exit;
        }
    }

    if (read_size > 0) {
        /* Read data - send 0xFF dummy bytes while receiving */
        memset(read_buf, SFUD_DUMMY_DATA, read_size);
        /* Use TransmitReceive: send dummy (already in read_buf) and receive */
        hal_status = HAL_SPI_TransmitReceive(&hspi2, read_buf, read_buf, read_size, 1000);
        if (hal_status != HAL_OK) {
            result = SFUD_ERR_TIMEOUT;
            goto exit;
        }
    }

exit:
    /* De-assert CS high */
    HAL_GPIO_WritePin(spi_dev->cs_gpiox, spi_dev->cs_gpio_pin, GPIO_PIN_SET);

    return result;
}

/* Delay function */
static void retry_delay_100us(void) {
    uint32_t delay = (SystemCoreClock / 10000000) * 100 / 5; /* ~100us */
    while (delay--);
}

/* SFUD port initialization */
sfud_err sfud_spi_port_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    /* Static user data for CS pin (GPIOB, PIN_12 = W25QXX CS) */
    static spi_user_data spi2_dev = {
        .cs_gpiox = GPIOB,
        .cs_gpio_pin = GPIO_PIN_12,
    };

    switch (flash->index) {
        case 0: {
            /* SPI2 is initialized by CubeMX MX_SPI2_Init() */

            /* Set SPI interface callbacks */
            flash->spi.wr = spi_write_read;
            flash->spi.lock = spi_lock;
            flash->spi.unlock = spi_unlock;
            flash->spi.user_data = &spi2_dev;

            /* Retry parameters */
            flash->retry.delay = retry_delay_100us;
            flash->retry.times = 60 * 10000; /* ~60s timeout */

            break;
        }
    }

    return result;
}

/* Debug log functions */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\r\n", log_buf);
    va_end(args);
}

void sfud_log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[SFUD]");
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\r\n", log_buf);
    va_end(args);
}
