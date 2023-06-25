#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
#include <cstdio>
#include <list>

#include <e131.h>
#include <unistd.h>
#include <sys/select.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "icosahedron.hpp"
#include "network.hpp"

const unsigned int MAX_E131_LEDS = (sizeof(((e131_packet_t *)0)->dmp.prop_val) - 1) / 3;

struct NetworkSenderImpl {
	 e131_addr_t m_dest;
	 std::vector<e131_packet_t> m_packets;
};

static inline float norm(const float f) {
	if(f > 1.0)
		 return 1.0;
	else if(f < 0.0)
		 return 0.0;
	else
		 return f;
}

/*static*/ float GammaCorrection::g_gammaCorrection = 2.2;

static float gamma(const float f) {
	return pow(norm(f), 1.0/GammaCorrection::g_gammaCorrection);
}

// -----------------------------------------
// -----------------------------------------

bool NetworkMultiSender::initHosts()
{
	for(auto& host : m_hosts) {
		unsigned int numLEDs = host.m_dataEnd;		
		unsigned int numUniverses = (numLEDs / MAX_E131_LEDS)+1;
		host.m_impl = std::shared_ptr<NetworkSenderImpl>(new NetworkSenderImpl);
		host.m_impl->m_packets.resize(numUniverses);

		for(unsigned int n = 0; n<numUniverses; n++) {
			auto& packet =  host.m_impl->m_packets[n];
			strcpy((char *)&packet.frame.source_name, "NiceLights");		
			unsigned int universeSize = std::min(MAX_E131_LEDS, numLEDs);
			printf("Init packet, host %s, universe %i, size %i\n", host.m_ipAddr.c_str(), n+host.m_startUniverse, universeSize);
			e131_pkt_init(&packet, n+host.m_startUniverse, 3 * universeSize);
			numLEDs -= universeSize;
		}
	}

	return true;
}

void NetworkMultiSender::updateEnabled()
{
	std::cout << (m_enabled ? "Enabling":"Disabling") << " E131 multisender sender" << std::endl;
	
	if(m_enabled) {
		for(auto& host : m_hosts) {
			if((host.m_fd = e131_socket()) < 0) {
				std::cout << "Failed to create socket" << std::endl;
				return;
			}

			std::string ipAddr(host.m_ipAddr);
			int n = ipAddr.find('-');
			if(n) {
				ipAddr = ipAddr.substr(0, n);
				printf("Using %s for %s\n", ipAddr.c_str(), host.m_ipAddr.c_str());
			}
			if(e131_unicast_dest(&host.m_impl->m_dest, ipAddr.c_str(), E131_DEFAULT_PORT) < 0) {
				std::cout << "Failed to set E131 unicast destination" << std::endl;
				return;
			}
			m_frameCount = 0;
		}
	} else {
		for(auto& host : m_hosts) {		
			if(host.m_fd >= 0) {
				close(host.m_fd);
				host.m_fd = -1;
			}
		}
	}
}

void NetworkMultiSender::update(const icosahedron::LightPoint *lightPtr, unsigned int nLights)
{
	if(!m_enabled)
		return;

	++m_frameCount;
	if(m_frameCount < m_frameDivisor)
		return;
	m_frameCount = 0;
	
	for(auto& host : m_hosts) {
		for(auto& range : host.m_ranges) {
			if((range.m_srcEnd) > nLights) {
				static bool once = [range, nLights]() {
					printf("Range %i %i is outside of the total data area of length %i\n", range.m_srcStart, range.m_srcEnd, nLights);
					return true;
				}();
				continue;
			}

			auto WriteLightToPacket = [&](const int destIdx, const int idx) {
				const int targetUniverse = destIdx / MAX_E131_LEDS;
				const int packetPos = destIdx % MAX_E131_LEDS;
				assert(targetUniverse < host.m_impl->m_packets.size());
				auto& packet = host.m_impl->m_packets[targetUniverse];
				uint8_t *valPtr = &packet.dmp.prop_val[m_packetStartOffset + (packetPos * 3)];
				*valPtr++ = (uint8_t)(gamma(lightPtr[idx].px) * 255.0f);
				*valPtr++ = (uint8_t)(gamma(lightPtr[idx].py) * 255.0f);
				*valPtr++ = (uint8_t)(gamma(lightPtr[idx].pz) * 255.0f);
			};

			if(!range.m_reversed) {
				for(int destIdx = range.m_destOffset, idx = range.m_srcStart; idx<range.m_srcEnd; ++idx, ++destIdx)
					WriteLightToPacket(destIdx, idx);
			} else {
				for(int destIdx = range.m_destOffset, idx = (range.m_srcEnd-1); idx>=range.m_srcStart; --idx, ++destIdx)
					WriteLightToPacket(destIdx, idx);				
			}
		}

		for(auto& packet : host.m_impl->m_packets) {
			packet.frame.seq_number = host.m_seqNumber++;
			if(e131_send(host.m_fd, &packet, &host.m_impl->m_dest) < 0)
				std::cout << "E131 sending failed" << std::endl;
		}
	}
}

bool NetworkMultiSender::readRangesFile(const std::string& filename)
{
	m_enabled = false;
	updateEnabled();
	m_hosts.clear();
	
	std::ifstream fi(filename);
	if(!fi) {
		printf("Failed to open %s\n", filename.c_str());
		return false;
	}

	int lineNum = 0;
	std::string line;
	while(std::getline(fi, line)) {
		++lineNum;		
		if(line.length() == 0)
			 continue;
		if(line[0] == '#')
			 continue;

		std::stringstream strm;
		strm << line;
		
		std::string cmd;
		if(!(strm >> cmd)) {
			printf("Failed to read command, line %i\n", lineNum);
			continue;
		}

		if(cmd == "host") {
			std::string ipaddr;
			if(!(strm >> ipaddr)) {
				printf("Failed to read ipaddress, line %i\n", lineNum);
				continue;
			}
			int startUniverse;
			if(!(strm >> startUniverse)) {
				printf("Failed to read start universe, line %i\n", lineNum);
				continue;
			}
			printf("Adding host %s, start universe %i\n", ipaddr.c_str(), startUniverse);
			
			m_hosts.push_back({ipaddr, startUniverse});
		} else if(cmd == "r" || cmd == "i") {
			int start;
			if(!(strm >> start)) {
				printf("Failed to read start, line %i\n", lineNum);
				continue;
			}
			
			int end;
			if(!(strm >> end)) {
				printf("Failed to read end, line %i\n", lineNum);
				continue;
			}

			if(start > end) {
				printf("Invalid range %i > %i, line %i\n", start, end, lineNum);
				continue;
			}

			std::string ipaddr;
			if(!(strm >> ipaddr)) {
				printf("Failed to read ipaddr, line %i\n", lineNum);
				continue;
			}

			auto itr = std::find_if(m_hosts.begin(), m_hosts.end(), [ipaddr](auto& elem) -> bool {
				return (elem.m_ipAddr == ipaddr);
			});

			if(itr == m_hosts.end()) {
				printf("Unknown host/ipaddr %s, line %i\n", ipaddr.c_str(), lineNum);
				continue;
			}

			int offset;
			if(!(strm >> offset)) {
				printf("Failed to read offset, line %i\n", lineNum);
				continue;
			}			

			printf("Adding range; host %s, start %i, end %i\n", ipaddr.c_str(), start, end);
			auto& host = *itr;
			int len = end - start;
			host.m_ranges.push_back({start, end, offset, cmd == "i"});
			int dataEnd = offset + len;
			if(dataEnd > host.m_dataEnd)
				host.m_dataEnd = dataEnd;
		} else {
			printf("Unknown line command '%s', line %i\n", cmd.c_str(), lineNum);
			continue;
		}
	}

	return initHosts();
}

// -----------------------------------------
// -----------------------------------------

NetworkSender::NetworkSender()
	: m_enabled(false),
		m_fd(-1),
		m_divisor(3),
		m_frameCount(0),
		m_maxUniverse(0),
		m_seqNumber(0),
		m_impl(new NetworkSenderImpl) {
	//strcpy(m_ipAddress, "4.3.2.1");
	strcpy(m_ipAddress, "127.0.0.1");	
}

void NetworkSender::initPackets(unsigned int numLEDs)
{
	unsigned int numUniverses = (numLEDs / MAX_E131_LEDS)+1;
	m_impl->m_packets.resize(numUniverses);
	m_maxUniverse = 1;

	for(unsigned int n = 0; n<numUniverses; n++) {
		auto& packet = m_impl->m_packets[n];
		strcpy((char *)&packet.frame.source_name, "NiceLights");		
		unsigned int universeSize = std::min(MAX_E131_LEDS, numLEDs);
		//printf("Universe %i - %u leds\n", (n+1), universeSize);		
		// note universe numbered from 1 not 0
		e131_pkt_init(&packet, n+1, 3 * universeSize);
		numLEDs -= universeSize;
	}
}

void NetworkSender::updateEnabled()
{
	std::cout << (m_enabled ? "Enabling":"Disabling") << " E131 sender" << std::endl;
	
	if(m_enabled) {
		if((m_fd = e131_socket()) < 0) {
			std::cout << "Failed to create socket" << std::endl;
			return;
		}

		if(e131_unicast_dest(&m_impl->m_dest, m_ipAddress, E131_DEFAULT_PORT) < 0) {
			std::cout << "Failed to set E131 unicast destination" << std::endl;
			return;
		}
		m_frameCount = 0;
	} else {
		if(m_fd >= 0) {
			close(m_fd);
			m_fd = -1;
		}
	}
}

void NetworkSender::updateDivisor()
{
	m_frameCount = 0;
}

void NetworkSender::update(const icosahedron::LightPoint *lightPtr, unsigned int nLights)
{
	if(m_enabled) {
		m_frameCount++;
		if(m_frameCount == m_divisor) {
			sendFrame(lightPtr, nLights);
			m_frameCount = 0;
		}			
	}
}

void NetworkSender::sendFrame(const icosahedron::LightPoint *lightPtr, unsigned int ledCount)
{
	for(auto& packet : m_impl->m_packets) {
		uint8_t *valPtr = packet.dmp.prop_val;
		unsigned int universeSize = std::min(MAX_E131_LEDS, ledCount);
		for(int c = 0; c<universeSize; c++) {
			*valPtr++ = (uint8_t)(norm(lightPtr->px) * 255.0);
			*valPtr++ = (uint8_t)(norm(lightPtr->py) * 255.0);
			*valPtr++ = (uint8_t)(norm(lightPtr->pz) * 255.0);
			++lightPtr;
		}
		ledCount -= universeSize;
	}

	int universe = 0;
	for(auto& packet : m_impl->m_packets) {
		packet.frame.seq_number = m_seqNumber++;
		if(e131_send(m_fd, &packet, &m_impl->m_dest) < 0)
			 std::cout << "E131 sending failed" << std::endl;
		++universe;
		if(universe >= m_maxUniverse)
			 break;
	}
}

int NetworkSender::getNumUniverses() const
{
	return m_impl->m_packets.size();
}

// -----------------------------------------
// -----------------------------------------

NetworkReceiver::NetworkReceiver()
	: m_fd(-1),
		m_dontWaitForAllUniverses(false),
		m_enabled(false),
		m_lastSeq(0x00),
		m_numUniverses(0),
		m_validUniverses(0)
{}

void NetworkReceiver::init(unsigned int numLEDs)
{
	m_numUniverses = (numLEDs / MAX_E131_LEDS)+1;
	printf("Num universes: %i, Leds per universe: %i\n", m_numUniverses, MAX_E131_LEDS);
	m_savedFrame.resize(numLEDs * 3);
}

void NetworkReceiver::updateEnabled()
{
	if(m_fd >= 0) {
		close(m_fd);
		m_fd = -1;
	}
	
	if(!m_enabled)
		 return;
	
	do {
		if((m_fd = e131_socket()) < 0) {
			std::cout << "Failed to create socket" << std::endl;
			return;
		}
		
		if(e131_bind(m_fd, E131_DEFAULT_PORT) < 0) {
			std::cout << "Failed to bind to port " << E131_DEFAULT_PORT << std::endl;
			break;
		}

		/*
		if(e131_multicast_join(m_fd, 1) < 0) {
			std::cout << "Failed to join multicast group" << std::endl;
			break;
		}
		*/
		std::cout << "E131 receiver enabled" << std::endl;
		return;
	} while(0);

	// fall through to here on error enabling.
	if(m_fd >= 0) {
		close(m_fd);
		m_fd = -1;
	}
}

void NetworkReceiver::update(icosahedron::LightPoint *lightPtr, unsigned int nLights)
{
	if(!m_enabled)
		 return;
	
  e131_packet_t packet;
  e131_error_t error;
	timeval tv = {0, 0};
	
	while(1) {
		// keep polling for new data until the socket is empty.
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_fd, &fds);
		int s = select(m_fd+1, &fds, NULL, NULL, &tv);
		if(s != 1) {
			break;
		}
		auto len = e131_recv(m_fd, &packet);
		if(len < 0)
			 return;
		if((error = e131_pkt_validate(&packet)) != E131_ERR_NONE) {
			std::cout << "Failed E131 packet validate: " << e131_strerror(error) << std::endl;
			continue;
		}

		if(e131_pkt_discard(&packet, m_lastSeq)) {
			std::cout << "Warning: packet out of order received.\n" << std::endl;
			m_lastSeq = packet.frame.seq_number;
		}
		
		//e131_pkt_dump(stderr, &packet);
		m_lastSeq = packet.frame.seq_number;

		short universe = ntohs(packet.frame.universe);
		short propCount = ntohs(packet.dmp.prop_val_cnt);

		//printf("Received packet %i %i\n", universe, propCount);
		
		uint8_t *propData = packet.dmp.prop_val;
		handlePacket(universe, propCount, propData);
	}

	syncLights(lightPtr, nLights);
}

void NetworkReceiver::handlePacket(short universe, short propCount, uint8_t *propData)
{
	if(universe > m_numUniverses)
		return;
	
	// universes are indexed from 1 not zero as we need.
	universe--;
	
	int dataStart = (universe * MAX_E131_LEDS * 3);
	int dataEnd = dataStart + propCount;
	m_savedFrame.resize(dataEnd);
	memcpy(&m_savedFrame[dataStart], propData, propCount);
	// maintain a bit mask of which universes have been received.
	m_validUniverses |= (1<<universe);
}

void NetworkReceiver::syncLights(icosahedron::LightPoint *lightPtr, unsigned int nLights)
{
	// if all the universes have been received then copy the data to the rendering.
	if(m_dontWaitForAllUniverses || ( m_validUniverses == ((1 << (m_numUniverses))-1))) {
		m_validUniverses = 0;
		uint8_t *ptr = &m_savedFrame[0];
		for(unsigned int n = 0; n<nLights; n++) {
			auto& pnt = *lightPtr++;
			pnt.px = (*ptr++ / 255.0);
			pnt.py = (*ptr++ / 255.0);
			pnt.pz = (*ptr++ / 255.0);
		}
	}
}
		
