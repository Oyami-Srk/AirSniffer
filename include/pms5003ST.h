#ifndef PMS_H
#define PMS_H

#include "Stream.h"

class PMS {
public:
  struct DATA {
    // Factory environment
    uint16_t PM_FE_UG_1_0;
    uint16_t PM_FE_UG_2_5;
    uint16_t PM_FE_UG_10_0;

    // Atmospheric environment
    uint16_t PM_AE_UG_1_0;
    uint16_t PM_AE_UG_2_5;
    uint16_t PM_AE_UG_10_0;

    // ST
    uint16_t ABOVE_0dot3_um;
    uint16_t ABOVE_0dot5_um;
    uint16_t ABOVE_1dot0_um;
    uint16_t ABOVE_2dot5_um;
    uint16_t ABOVE_5dot0_um;
    uint16_t ABOVE_10_um;

    // ST 2
    uint16_t HCHO;
    uint16_t TEMP;
    uint16_t HUMD;
  };

  enum STATUS { STATUS_WAITING, STATUS_OK };

  PMS(Stream &, DATA *data);

  bool read(DATA &data, uint16_t timeout = 0);
  void loop();
  STATUS status;

private:
  uint8_t _payload[30];
  Stream *_stream;
  DATA *_data;

  uint8_t _index = 0;
  uint16_t _frameLen;
  uint16_t _checksum;
  uint16_t _calculatedChecksum;
};

#endif