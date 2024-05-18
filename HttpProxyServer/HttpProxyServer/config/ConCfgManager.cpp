#include "ConCfgManager.h"
#include <tinyxml/tinyxml.h>
#include <iostream>
#include <string>
#include <map>

std::string ConCfgManager::m_strIP;
int ConCfgManager::m_nPort;

void ConCfgManager::loadConfig(const std::string& strXmlFilePath) 
{
	TiXmlDocument doc(strXmlFilePath.c_str());
	if (!doc.LoadFile()) {
		throw std::runtime_error("Failed to load XML file");
	}

	TiXmlElement* pRootElement = doc.FirstChildElement("Configs");
	if (!pRootElement) {
		throw std::runtime_error("Invalid XML config file: No 'Configs' element found.");
	}

	TiXmlElement* pServiceElement = pRootElement->FirstChildElement("Service");
	if (!pServiceElement) {
		throw std::runtime_error("No 'Service' element found.");
	}

	const char* ip = pServiceElement->Attribute("IP");
	const char* port = pServiceElement->Attribute("port");

	if (!ip || !port) {
		throw std::runtime_error("Service configuration incomplete.");
	}

	m_strIP = ip;
	m_nPort = atoi(port);
}

void ConCfgManager::displayConfig() const 
{
	std::cout << "Service IP: " << m_strIP << std::endl;
	std::cout << "Service port: " << m_nPort << std::endl;
}
