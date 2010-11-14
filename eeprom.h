/*
 * interrupt-based eeprom I/O
 */

#ifndef EEPROM_H_INC
#define EEPROM_H_INC

#include <stddef.h>
#include <stdint.h>

typedef uint8_t eesize_t;

size_t eeprom_write_block_intr(const void *src,void *dst, size_t size);
uint8_t eeprom_check_complete();

#endif /* EEPROM_H_INC */
