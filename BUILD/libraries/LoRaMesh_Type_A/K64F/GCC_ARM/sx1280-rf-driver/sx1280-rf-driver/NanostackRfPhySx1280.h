/*
 * Copyright (c) 2014-2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NANOSTACK_RF_PHY_ATMEL_H_
#define NANOSTACK_RF_PHY_ATMEL_H_

#include "NanostackRfPhy.h"
//#include "at24mac.h"
#include "PinNames.h"

// Arduino pin defaults for convenience
#if !defined(SX1280_SPI_MOSI)
#define SX1280_SPI_MOSI   D11
#endif
#if !defined(SX1280_SPI_MISO)
#define SX1280_SPI_MISO   D12
#endif
#if !defined(SX1280_SPI_SCLK)
#define SX1280_SPI_SCLK   D13
#endif
#if !defined(SX1280_SPI_CS)
#define SX1280_SPI_CS     D7
#endif
#if !defined(SX1280_SPI_RST)
#define SX1280_SPI_RST    A0
#endif
#if !defined(SX1280_SPI_IRQ)
//#define SX1280_SPI_IRQ    D5
#define SX1280_SPI_IRQ    PTC6  // for the NXP new board testing
#endif
#if !defined(ATMEL_I2C_SDA)
#define ATMEL_I2C_SDA    D14
#endif
#if !defined(ATMEL_I2C_SCL)
#define ATMEL_I2C_SCL    D15
#endif

class RFBits;

class NanostackRfPhyAtmel : public NanostackRfPhy {
public:
    NanostackRfPhyAtmel(PinName spi_mosi, PinName spi_miso,
            PinName spi_sclk, PinName spi_cs,  PinName spi_rst, PinName spi_irq,
            PinName i2c_sda, PinName i2c_scl);
    virtual ~NanostackRfPhyAtmel();
    virtual int8_t rf_register();
    virtual void rf_unregister();
    virtual void get_mac_address(uint8_t *mac);
    virtual void set_mac_address(uint8_t *mac);

private:
//  AT24Mac _mac;
    uint8_t _mac_addr[8];
    RFBits *_rf;
    bool _mac_set;

    const PinName _spi_mosi;
    const PinName _spi_miso;
    const PinName _spi_sclk;
    const PinName _spi_cs;
    const PinName _spi_rst;
    //const PinName _spi_slp;
    const PinName _spi_irq;
};

#endif /* NANOSTACK_RF_PHY_ATMEL_H_ */
