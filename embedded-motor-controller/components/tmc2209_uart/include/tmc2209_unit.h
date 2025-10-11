//
// Created by Anders Sund-Jensen on 10-10-2025.
//

#ifndef TMC2209_UNIT_H
#define TMC2209_UNIT_H

#include "tmc2209_reg.h"
#include "driver/uart.h"

typedef struct
{
    //TODO add serial abstraction
    // UART abstraction is only the port in esp idf
    uart_port_t uart_num;
    uint8_t address;

    // WRITE registers
    Register_GCONF GCONF;
    Register_IHOLD_IRUN IHOLD_IRUN;
    Register_CHOPCONF CHOPCONF;
    Register_PWMCONF PWMCONF;
    Register_COOLCONF COOLCONF;
    Register_THRS TCOOLTHRS;
    Register_THRS TPWMTHRS;

    // READ registers
    Register_IOIN IOIN;
    Register_DRV_STATUS DRV_STATUS;

    uint16_t SG_RESULT; //READ
    uint8_t SGTHRS; //WRITE

    uint8_t TPOWERDOWN; //WRITE

    uint8_t IFCNT; //READ
} TMC2209_UNIT;

#endif //TMC2209_UNIT_H
