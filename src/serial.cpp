#include <iostream>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#include "serial.hpp"

bool SerialIO::open(const std::string& devname)
{
#ifdef _WIN32
  
  if(m_file != INVALID_HANDLE_VALUE) {
    CloseHandle(m_file);
    m_file = INVALID_HANDLE_VALUE;
  }
  
  std::string filename = "\\\\.\\" + devname;
  m_file = CreateFile(filename.c_str(),
		      GENERIC_READ|GENERIC_WRITE,
		      0,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
		      NULL);

  
  if(m_file == INVALID_HANDLE_VALUE) {
    std::cout << "Unable to open: " << filename << std::endl;
    return false;
  }

  std::cout << "Open ok : " << filename << std::endl;

  // ensure we have the right baudrate. Don't change anything else
  // almost certainly this is connected to a USB to serial device
  // so the rest of the comm stuff is pretty meaningless anyways.
  DCB dcb = {0};
  if(!GetCommState(m_file, &dcb)) {
    std::cout << "Failed GetCommState" << std::endl;
    return false;
  }
  dcb.BaudRate = CBR_115200;

  if(!SetCommState(m_file, &dcb)) {
    std::cout << "Failed GetCommState" << std::endl;
    return false;
  }

  if(!m_ovl.hEvent)
    m_ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  SetCommMask(m_file, EV_RXCHAR);

  // start a background wait for a comm event - we will check this when the app calls read()
  if(!WaitCommEvent(m_file, &m_commEvent, &m_ovl)) {
    if(GetLastError() != ERROR_IO_PENDING) {
      std::cout << "Failed WaitCommEvent " << GetLastError() << std::endl;
      return false;
    }
  }

  std::cout << "Serial open success" << std::endl;  
  return true;
  
#else
  
  m_bytesRead = 0;
	
  if(m_fd != -1) {
    ::close(m_fd);
    m_fd = -1;
  }
	

  if((m_fd = ::open(devname.c_str(), O_RDWR/* | O_EXCL | O_NDELAY*/)) == -1) {
    std::cout << "unable to open" << std::endl;
    return false;
  }

  // Create new termios struct, we call it 'tty' for convention
  struct termios tty;

  // Read in existing settings, and handle any error
  if(tcgetattr(m_fd, &tty) != 0) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    return false;
  }

  auto speed = B115200;
	
  tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
  tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
  tty.c_cflag |= CS8; // 8 bits per byte (most common)
  tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO; // Disable echo
  tty.c_lflag &= ~ECHOE; // Disable erasure
  tty.c_lflag &= ~ECHONL; // Disable new-line echo
  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
  // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
  // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;

  // Set in/out baud rate to be 9600
  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  // Save tty settings, also checking for error
  if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    return 1;
  }
	
  tcflush(m_fd, TCIOFLUSH);
  std::cout << "Opened device" << std::endl;
  return true;
#endif
}

int SerialIO::read(int readLen)
{
#ifdef _WIN32
  // if there is already serial data buffered push a single byte and return as if we actually
  // read it like would have happened with Linux.
  if(m_readTmpSize > 0) {
    pushByte(m_readTmp[m_readTmpOffset]);
    ++m_readTmpOffset;
    --m_readTmpSize;
    return 1;
  }

  // otherwise check for the previous comm event wait to complete.
  DWORD nBytes = 0;
  if(!GetOverlappedResult(m_file, &m_ovl, &nBytes, FALSE)) {
    if(GetLastError() != ERROR_IO_INCOMPLETE)
      std::cout << "Error waiting for comm event" << std::endl;
    // no data yet
    return 0;
  }

  bool byteWasRead = false;

  // if this is a rx data data event
  if(m_commEvent == EV_RXCHAR) {
    // find out how much data is queued.
    COMSTAT commStat;
    if(!ClearCommError(m_file, NULL, &commStat)) {
      std::cout << "Failed ClearCommError: " << GetLastError() << std::endl;
      return false;
    }
    
    // work out how much data we can consume.
    DWORD dataSizeToRead = std::min(commStat.cbInQue, COMMDATA_SIZE);

    if(dataSizeToRead > 0) {    
      // we did read data unless we discover otherwise...
      byteWasRead = true;
    
      // try and read a decent sized amount of data from the comm port
      if(!ReadFile(m_file, &m_readTmp, dataSizeToRead, &nBytes, &m_ovl)) {
	// failed because the driver is working.
	if(GetLastError() == ERROR_IO_PENDING) {
	  // we know there is data there to read 
	  if(!GetOverlappedResult(m_file, &m_ovl, &nBytes, TRUE)) {
	    std::cout << "Failed to wait for ReadFile to complete" << std::endl;
	    byteWasRead = false;
	  }
	} else {
	  std::cout << "ReadFile failed: " << GetLastError() << std::endl;
	  byteWasRead = false;
	}
      }
    }
  }

  // start another wait for the next comm event
  if(!WaitCommEvent(m_file, &m_commEvent, &m_ovl)) {
    if(GetLastError() != ERROR_IO_PENDING) {
      std::cout << "Failed WaitCommEvent" << std::endl;
      return false;
    }
  }  

  // if we did read data queue the first byte and return that we read just one byte
  // like the linux version despite having actually read probably a lot more.
  if(byteWasRead) {
    pushByte(m_readTmp[0]);
    m_readTmpSize = nBytes-1;
    m_readTmpOffset = 1;
    return 1;
  } else
    return 0;

#else

  int dataRead = 0;

  while(dataRead < readLen) {
    fd_set readfs;
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
	
    FD_ZERO(&readfs);
    FD_SET(m_fd, &readfs);

    int s = 0;
    if((s = select(m_fd+1, &readfs, NULL, NULL, &tv)) < 0) {
      std::cout << "Failed select() on serial port" << std::endl;
      break;
    }
	
    if(FD_ISSET(m_fd, &readfs)) {
      uint8_t tmp = 0;
      int len = ::read(m_fd, &tmp, 1);
      if(len != 1) {
	std::cout << "Error reading serial port" << std::endl;
	break;
      }
      pushByte(tmp);
      dataRead += len;
      break;
    } else
      break;
  }
	
  return dataRead;
#endif
}

void SerialIO::pushByte(const uint8_t b)
{
  m_dataBuf[m_dataBufLen] = b;			
  m_bytesRead += 1;
  m_dataBufLen += 1;
  
  if(m_dataBufLen == sizeof(m_dataBuf)) {
    printf("Overflow\n");
    m_dataBuf[0] = b;
    m_dataBufLen = 1;
  }
}

void SerialIO::clearDataBuf()
{
#ifdef _WIN32
#endif
  m_dataBufLen = 0;
}
