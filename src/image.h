#pragma once
#include <Arduino.h>

const uint8_t chao[] = {0x00, 0x00, 0x00, 0x40, 0x48, 0x30, 0x48, 0x0F,
                        0x48, 0x10, 0xFE, 0x3F, 0x48, 0x22, 0x48, 0x42,
                        0x40, 0x40, 0x44, 0x5F, 0x34, 0x51, 0x0C, 0x51,
                        0x44, 0x51, 0x44, 0x51, 0x3C, 0x5F, 0x00, 0x40};
const uint8_t gao[] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x7F, 0x04, 0x01,
                       0x74, 0x01, 0x54, 0x1D, 0x54, 0x15, 0x56, 0x15,
                       0x56, 0x15, 0x54, 0x15, 0x54, 0x15, 0x54, 0x1D,
                       0x74, 0x41, 0x04, 0x41, 0x04, 0x3F, 0x00, 0x00};
const uint8_t guo[] = {0x00, 0x00, 0x00, 0x40, 0x82, 0x40, 0x86, 0x20,
                       0x88, 0x1F, 0x08, 0x20, 0x00, 0x40, 0x48, 0x40,
                       0xE8, 0x40, 0x08, 0x43, 0x08, 0x50, 0x08, 0x50,
                       0xFF, 0x5F, 0x08, 0x40, 0x08, 0x40, 0x00, 0x40};
const uint8_t jiao[] = {0x00, 0x00, 0xC4, 0x08, 0xBC, 0x08, 0x87, 0x08,
                        0xE4, 0x7F, 0x84, 0x04, 0x04, 0x04, 0x10, 0x41,
                        0x88, 0x20, 0x68, 0x11, 0x0A, 0x0A, 0x0C, 0x0C,
                        0xA8, 0x13, 0x48, 0x20, 0x88, 0x40, 0x00, 0x20};
const uint8_t lianghao[] = {
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x7F, 0x24, 0x41, 0x24, 0x41, 0x24,
    0x21, 0x27, 0x23, 0x24, 0x05, 0x24, 0x09, 0x24, 0x11, 0x24, 0x29,
    0xFC, 0x25, 0x00, 0x44, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x10,
    0x40, 0x90, 0x21, 0x70, 0x12, 0x1E, 0x0C, 0x10, 0x0B, 0xF0, 0x30,
    0x00, 0x00, 0x84, 0x20, 0x84, 0x40, 0x84, 0x40, 0xE4, 0x3F, 0x94,
    0x00, 0x8C, 0x00, 0x84, 0x00, 0x80, 0x00, 0x00, 0x00};
const uint8_t logo[] = {0x3C, 0x00, 0x0A, 0x00, 0x09, 0x00, 0x09, 0x00,
                        0x0A, 0x00, 0x7C, 0x01, 0x50, 0x01, 0x50, 0x01,
                        0x50, 0x01, 0xD0, 0x01, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t siwang[] = {
    0x00, 0x02, 0x04, 0x43, 0xC4, 0x20, 0x34, 0x22, 0x2C, 0x1A, 0x24,
    0x04, 0xA4, 0x03, 0x64, 0x00, 0x04, 0x00, 0xFC, 0x7F, 0x04, 0x41,
    0x84, 0x40, 0x84, 0x40, 0x64, 0x40, 0x44, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x10, 0x00, 0xF0, 0x7F, 0x10, 0x40, 0x10, 0x40,
    0x12, 0x40, 0x14, 0x40, 0x18, 0x40, 0x10, 0x40, 0x10, 0x40, 0x10,
    0x40, 0x10, 0x40, 0x10, 0x40, 0x10, 0x00, 0x00, 0x00};
const uint8_t upload[] = {0x04, 0x00, 0x06, 0x00, 0x3F, 0x00, 0x46, 0x00,
                          0xC4, 0x00, 0xF8, 0x01, 0xC0, 0x00, 0x40, 0x00};
const uint8_t wifi[] = {0x04, 0x00, 0x12, 0x00, 0x49, 0x00, 0x25, 0x00,
                        0x95, 0x01, 0x95, 0x01, 0x25, 0x00, 0x49, 0x00,
                        0x12, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};