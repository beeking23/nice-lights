#pragma once

class NetworkSenderImpl;

struct GammaCorrection {
	static float g_gammaCorrection;
};
class NetworkSender {
public:
	 NetworkSender();

	 void update(const icosahedron::LightPoint *lightPtr, unsigned int nLights);
	 void initPackets(unsigned int numLeds);
	 void sendFrame(const icosahedron::LightPoint *lightPtr, unsigned int nLights);	 

	 void updateEnabled();
	 void updateDivisor();
	 int getNumUniverses() const;

	 bool m_enabled;
	 int m_divisor;
	 int m_maxUniverse;
	 char m_ipAddress[64];	 
	 
private:
	int m_fd;
	std::shared_ptr<NetworkSenderImpl> m_impl;
	int m_frameCount;
	uint8_t m_seqNumber;
};

class NetworkReceiver {
public:
	NetworkReceiver();
	void init(unsigned int numLEDs);
	void update(icosahedron::LightPoint *lightPtr, unsigned int nLights);

	bool m_dontWaitForAllUniverses;
	bool m_enabled;
	void updateEnabled();
	
private:
	void handlePacket(short universe, short propCount, uint8_t *propData);
	void syncLights(icosahedron::LightPoint *lightPtr, unsigned int nLights);
	int m_fd;
	uint8_t m_lastSeq;
	int m_numUniverses;
	unsigned int m_validUniverses;
	std::vector<uint8_t> m_savedFrame;
};

class NetworkMultiSender {
public:
	NetworkMultiSender() {}
	void update(const icosahedron::LightPoint *lightPtr, unsigned int nLights);	
	bool readRangesFile(const std::string& filename);
	bool initHosts();
	void updateEnabled();

	bool m_enabled = false;
	int m_frameDivisor = 2;	
	int m_packetStartOffset = 1;
	
protected:
	struct DeviceLEDRange {
		int m_srcStart = 0;
		int m_srcEnd = 0;
		int m_destOffset = 0;
		bool m_reversed = false;
	};
	
	struct HostDef {
		std::string m_ipAddr;
		int m_startUniverse = -1;
		int m_fd = -1;
		std::vector<DeviceLEDRange> m_ranges;			
		std::shared_ptr<NetworkSenderImpl> m_impl;
		uint8_t m_seqNumber = 0;
		int m_dataEnd = 0;
	};
	
	std::vector<HostDef> m_hosts;
	
	int m_frameCount = 0;
	

};
	 
