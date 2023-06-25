#include <iostream>
#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

#include "serial.hpp"

bool SerialIO::open(const std::string& devname)
{
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
}

int SerialIO::read(int readLen)
{
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

			m_dataBuf[m_dataBufLen] = tmp;			
			m_bytesRead += len;
			m_dataBufLen += len;

			if(m_dataBufLen == sizeof(m_dataBuf)) {
				printf("Overflow\n");
				m_dataBuf[0] = tmp;
				m_dataBufLen = 1;
			}
			dataRead += len;
			break;
		} else
			break;
	}
	
	return dataRead;
}

