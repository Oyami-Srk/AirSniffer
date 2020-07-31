#include "pms5003ST.h"
#include "Arduino.h"

PMS::PMS(Stream &stream, DATA *data) {
  this->_stream = &stream;
  this->_data = data;
}

// Non-blocking function for parse response.
// If you want to wait for the full response (blocking), specify timeout
// parameter (1000ms is sufficient). This makes it easier to work in passive
// mode.
bool PMS::read(DATA &data, uint16_t timeout) {
  _data = &data;
  if (timeout > 0) {
    uint32_t start = millis();
    do {
      loop();
      if (status == STATUS_OK)
        break;
    } while (millis() - start < timeout);
  } else {
    loop();
  }

  return status == STATUS_OK;
}

void PMS::loop() {
  status = STATUS_WAITING;
  if (_stream->available()) {
    uint8_t ch = _stream->read();

    switch (_index) {
    case 0:
      if (ch != 0x42) {
        return;
      }
      _calculatedChecksum = ch;
      break;

    case 1:
      if (ch != 0x4D) {
        _index = 0;
        return;
      }
      _calculatedChecksum += ch;
      break;

    case 2:
      _calculatedChecksum += ch;
      _frameLen = ch << 8;
      break;

    case 3:
      _frameLen |= ch;
      // Unsupported sensor, different frame length, transmission error e.t.c.
      if (_frameLen != 2 * 17 + 2) {
        _index = 0;
        return;
      }
      _calculatedChecksum += ch;
      break;

    default:
      if (_index == _frameLen + 2) {
        _checksum = ch << 8;
      } else if (_index == _frameLen + 2 + 1) {
        _checksum |= ch;

        if (_calculatedChecksum == _checksum) {
          status = STATUS_OK;

          // Factory environment.
          _data->PM_FE_UG_1_0 = makeWord(_payload[0], _payload[1]);
          _data->PM_FE_UG_2_5 = makeWord(_payload[2], _payload[3]);
          _data->PM_FE_UG_10_0 = makeWord(_payload[4], _payload[5]);

          // Atmospheric environment.
          _data->PM_AE_UG_1_0 = makeWord(_payload[6], _payload[7]);
          _data->PM_AE_UG_2_5 = makeWord(_payload[8], _payload[9]);
          _data->PM_AE_UG_10_0 = makeWord(_payload[10], _payload[11]);

          _data->ABOVE_0dot3_um = makeWord(_payload[12], _payload[13]);
          _data->ABOVE_0dot5_um = makeWord(_payload[14], _payload[15]);
          _data->ABOVE_1dot0_um = makeWord(_payload[16], _payload[17]);
          _data->ABOVE_2dot5_um = makeWord(_payload[18], _payload[19]);
          _data->ABOVE_5dot0_um = makeWord(_payload[20], _payload[21]);
          _data->ABOVE_10_um = makeWord(_payload[22], _payload[23]);

          _data->HCHO = makeWord(_payload[24], _payload[25]);
          _data->TEMP = makeWord(_payload[26], _payload[27]);
          _data->HUMD = makeWord(_payload[28], _payload[29]);
        }

        _index = 0;
        return;
      } else {
        _calculatedChecksum += ch;
        uint8_t payloadIndex = _index - 4;

        // Payload is common to all sensors (first 2x6 bytes).
        if (payloadIndex < sizeof(_payload)) {
          _payload[payloadIndex] = ch;
        }
      }

      break;
    }

    _index++;
  }
}