/*
*
 * TMC2209 Stepper Driver UART communication.
 *
 * Author: Anders Sund-Jensen
 * Created: 11 October 2025
 *
 */

#include "tmc2209_uart.h"

static const char* TAG = "tmc2209-uart";

//Private methods
uint8_t getStartCRC(uint8_t address);
uint8_t get1ByteCRC(uint8_t byte, uint8_t crc);
uint8_t get4ByteCRC(uint32_t data, uint8_t crc);

// Data byte repack
uint32_t reverseBytes(uint32_t data);

uint64_t createWriteDatagram(uint8_t address, uint8_t reg, uint32_t data);
uint32_t createReadDatagram(uint8_t address, uint8_t reg);
uint32_t createResponseDatagram(uint64_t datagram);

void writeRequestDatagram(uart_port_t uart_num, uint8_t address, uint8_t reg);
uint64_t readResponseDatagram(uart_port_t uart_num);

//Write request
void tmc2209_request(uart_port_t uart_num, uint8_t address, uint8_t reg);

// Implementation
uint8_t getStartCRC(uint8_t address)
{
    if (address == TMC2209_ADR_0)
        return TMC2209_CRC_START_ADR_0;
    if (address == TMC2209_ADR_1)
        return TMC2209_CRC_START_ADR_1;
    if (address == TMC2209_ADR_2)
        return TMC2209_CRC_START_ADR_2;
    if (address == TMC2209_ADR_3)
        return TMC2209_CRC_START_ADR_3;
    return 0;
}

uint8_t get1ByteCRC(uint8_t byte, uint8_t crc)
{
    for (uint8_t i = 0; i < 0x8; ++i)
    {
        if ((crc >> 7) ^ (byte & 0x01))
            crc = (crc << 1) ^ 0x07;
        else
            crc = crc << 1;
        byte >>= 1;
    }
    return crc;
}

uint8_t get4ByteCRC(uint32_t data, uint8_t crc)
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        crc = get1ByteCRC(data & 0xFFU, crc);
        data >>= 8; //Get the next byte
    }
    return crc;
}

uint32_t reverseBytes(uint32_t data)
{
    uint32_t reversed_data = 0;
    uint8_t right_shift;
    uint8_t left_shift;
    for (uint8_t i = 0; i < 4; ++i)
    {
        right_shift = (3 - i) * 8;
        left_shift = i * 8;
        reversed_data |= ((data >> right_shift) & 0xFF ) << left_shift;
    }
    return reversed_data;
}

uint64_t createWriteDatagram(uint8_t address, uint8_t reg, uint32_t data)
{
    Datagram64 datagram;
    datagram.REG.sync = TMC2209_SYNC;
    datagram.REG.serial_address = address;
    datagram.REG.register_address = reg;
    datagram.REG.data = reverseBytes(data);
    datagram.REG.crc = get4ByteCRC(datagram.REG.data, get1ByteCRC(reg, getStartCRC(address)));
    return datagram.UINT64;
}

uint32_t createReadDatagram(uint8_t address, uint8_t reg)
{
    Datagram32 datagram;
    datagram.REG.sync = TMC2209_SYNC;
    datagram.REG.serial_address = address;
    datagram.REG.register_address = reg;
    datagram.REG.crc = get1ByteCRC(reg, getStartCRC(address));
    return datagram.UINT32;
}

uint32_t createResponseDatagram(uint64_t datagram)
{
    if (datagram == UINT64_MAX)
        return datagram; //Error happened
    uint8_t reg = datagram >>= 16;
    uint32_t data = datagram >>= 8;
    uint8_t crc_recieved = datagram >>= 32;
    uint8_t crc_calculated = get4ByteCRC(data, get1ByteCRC(reg, TMC2209_CRC_START_ADR_RESPOND));
    if (crc_recieved != crc_calculated)
        return UINT32_MAX;
    return reverseBytes(data);
}

void tmc2209_write(uart_port_t uart_num, uint8_t address, uint8_t reg, uint32_t data)
{
    //Maybe should check if the uart_port is installed first.
    uint64_t datagram = createWriteDatagram(address, reg, data);

    //Ensure all the earlier communication has been finished
    uart_wait_tx_done(uart_num, 100);
    uart_flush(uart_num);
    uint8_t send_buffer = 0;

    for (uint8_t i = 0; i < 8; ++i)
    {
        send_buffer = (uint8_t)(datagram >> (i * 8U));
        uart_write_bytes(uart_num, &send_buffer, sizeof(uint8_t));
    }
}

void tmc2209_request(uart_port_t uart_num, uint8_t address, uint8_t reg)
{
    //Maybe should check if the uart_port is installed first.
    uint32_t datagram = createReadDatagram(address, reg);

    //Ensure all the earlier communication has been finished
    uart_wait_tx_done(uart_num, 100);
    uart_flush(uart_num);
    uint8_t send_buffer = 0;

    printf("Sent the request: %lu\n", datagram);

    for (uint8_t i = 0; i < 4; ++i)
    {
        send_buffer = (uint8_t)(datagram >> (i * 8U));
        uart_write_bytes(uart_num, &send_buffer, sizeof(uint8_t));
    }
}

uint64_t readResponseDatagram(uart_port_t uart_num)
{
    uint64_t reply = 0;
    uint64_t discardBuffer = 0;
    uint64_t actual_reply = 0;
    size_t length = 0;

    //Ensure all the earlier communication has been finished
    uart_wait_tx_done(uart_num, 100);
    uart_flush(uart_num);

    //Clean the buffer
    uart_get_buffered_data_len(uart_num, &length);
    while (length > 0)
    {
        uart_read_bytes(uart_num, &discardBuffer, sizeof(discardBuffer), portMAX_DELAY);
        uart_get_buffered_data_len(uart_num, &length);
    }

    //Maybe should check if the uart_port is installed first.
    uart_read_bytes(uart_num, &reply, sizeof(reply), 1 / portTICK_PERIOD_MS);

    for (uint8_t i = 0; i < 8; ++i)
        actual_reply |= reply << (i * 8U);

    return actual_reply;
}

uint32_t tmc2209_read(uart_port_t uart_num, uint8_t address, uint8_t reg)
{
    tmc2209_request(uart_num, address, reg);
    uint64_t datagram = readResponseDatagram(uart_num);
    return createResponseDatagram(datagram);
}

void writeGCONF(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->GCONF.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_GCONF, config->GCONF.UINT32);
}

void writeIHOLD_IRUN(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->IHOLD_IRUN.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_IHOLD_IRUN, config->IHOLD_IRUN.UINT32);
}

void writeCHOPCONF(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->CHOPCONF.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_CHOPCONF, config->CHOPCONF.UINT32);
}

void writePWMCONF(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->PWMCONF.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_PWMCONF, config->PWMCONF.UINT32);
}

void writeCOOLCONF(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->COOLCONF.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_COOLCONF, config->COOLCONF.UINT32);
}

void writeTCOOLTHRS(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->TCOOLTHRS.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_TCOOLTHRS, config->TCOOLTHRS.UINT32);
}

void writeTPWMTHRS(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->TPWMTHRS.UINT32 = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_TPWMTHRS, config->TPWMTHRS.UINT32);
}

void writeSGTHRS(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->SGTHRS = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_SGTHRS, config->SGTHRS);
}

void writeTPOWERDOWN(TMC2209_UNIT *config, uint32_t value)
{
    if (value != UINT32_MAX)
        config->TPOWERDOWN = value;
    tmc2209_write(config->uart_num, config->address, ADDRESS_TPOWERDOWN, config->TPOWERDOWN);
}

uint32_t readIOIN(TMC2209_UNIT *config)
{
    uint32_t data = tmc2209_read(config->uart_num, config->address, ADDRESS_IOIN);
    if (data != UINT32_MAX)
        config->IOIN.UINT32 = data;
    return data;
}

uint32_t readSG_RESULT(TMC2209_UNIT *config)
{
    uint32_t data = tmc2209_read(config->uart_num, config->address, ADDRESS_SG_RESULT);
    if (data != UINT32_MAX)
        config->SG_RESULT = data;
    return data;
}

uint32_t readIFCNT(TMC2209_UNIT *config)
{
    uint32_t data = tmc2209_read(config->uart_num, config->address, ADDRESS_IFCNT);
    if (data != UINT32_MAX)
        config->IFCNT = data;
    return data;
}

bool init_tmc2209_default(uart_port_t uart_num, uint8_t address, TMC2209_UNIT *config)
{
    config->uart_num = uart_num;
    config->address = address;
    writeGCONF(config, GCONF_DEFAULT);
    writeIHOLD_IRUN(config, IHOLD_IRUN_DEFAULT);
    writeCHOPCONF(config, CHOPCONF_DEFAULT);
    writePWMCONF(config, PWMCONF_DEFAULT);
    writeTCOOLTHRS(config, TCOOLTHRS_DEFAULT);
    writeTPWMTHRS(config, TPWMTHRS_DEFAULT);

    return available_tmc2209(config);
}

bool available_tmc2209(TMC2209_UNIT *config)
{
    if (readIOIN(config) != UINT32_MAX)
        return true;
    return false;
}

void destroy_tmc2209(TMC2209_UNIT *config)
{
    free(config);
}