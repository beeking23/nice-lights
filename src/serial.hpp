#pragma once

class SerialIO {
public:
	bool open(const std::string& devname);
	int read(int len);
	
	void clearDataBuf() {
		m_dataBufLen = 0;
	}
	
	const uint8_t *getDataBuf() const { return m_dataBuf; }
	int getDataBufLen() const { return m_dataBufLen; }
	int getBytesRead() const { return m_bytesRead; }
	
private:
	int m_fd = -1;
  uint8_t m_dataBuf[256] = {0};
	int m_dataBufLen = 0;
	int m_bytesRead = 0;
};
