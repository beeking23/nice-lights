#pragma once

#define PACKED __attribute__((__packed__))

/// Data format the serial data from the light mixer
struct PACKED SerialControllerPacket {
  uint8_t m_magicByte;
        
  uint8_t m_seqNum : 6;
  uint8_t m_zero1 : 1;
  uint8_t m_changed : 1;
        
  uint8_t m_buttonA : 1;
  uint8_t m_buttonB : 1;
  uint8_t m_buttonC : 1;
  uint8_t m_buttonX : 5;  

  union {
    struct {
      uint8_t m_switchC_L : 1;
      uint8_t m_switchB_L : 1;
      uint8_t m_switchA_L : 1;
      uint8_t m_switchC_R : 1;
      uint8_t m_switchB_R : 1;
      uint8_t m_switchA_R : 1;
      uint8_t m_zero2 : 2;                    
    };
    uint8_t m_allSwitches;
  };

  uint8_t m_faderA;
  uint8_t m_faderB;
  uint8_t m_faderC;

  uint8_t m_potA;
  uint8_t m_potB;
  uint8_t m_potC;         

  uint8_t m_joyAxisX;
  uint8_t m_joyAxisY;             
};

static_assert(sizeof(SerialControllerPacket) == 12);
