#pragma once

class SerialIO {
public:
  bool open(const std::string& devname);
  int read(int len);
	
  void clearDataBuf();
	
  const uint8_t *getDataBuf() const { return m_dataBuf; }
  int getDataBufLen() const { return m_dataBufLen; }
  int getBytesRead() const { return m_bytesRead; }
	
private:
#ifdef _WIN32
  HANDLE m_file = INVALID_HANDLE_VALUE;
  OVERLAPPED m_ovl = {0};
  DWORD m_commEvent = 0;

  static constexpr DWORD COMMDATA_SIZE = 512;  
  uint8_t m_readTmp[COMMDATA_SIZE];
  DWORD m_readTmpSize = 0;
  DWORD m_readTmpOffset = 0;  
#else
  int m_fd = -1;
#endif

  void pushByte(const uint8_t b);
  
  uint8_t m_dataBuf[256] = {0};
  int m_dataBufLen = 0;
  int m_bytesRead = 0;
};
