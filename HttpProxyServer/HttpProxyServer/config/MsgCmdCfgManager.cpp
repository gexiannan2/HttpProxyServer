#include "MsgCmdCfgManager.h"
#include <tinyxml/tinyxml.h>
#include <iostream>
#include <string>
#include <map>

void MsgCmdCfgManager::loadConfig(const std::string& strXmlFilePath)
{
    TiXmlDocument doc(strXmlFilePath.c_str());
    if (!doc.LoadFile())
    {
        std::cerr << "Failed to load file: " << strXmlFilePath << std::endl;
        return;
    }

    TiXmlElement* root = doc.FirstChildElement("configuration");
    if (!root)
    {
        std::cerr << "Failed to find root element in XML file" << std::endl;
        return;
    }

    TiXmlElement* forwarding = root->FirstChildElement("forwarding");
    if (!forwarding)
    {
        std::cerr << "Failed to find 'forwarding' element in XML file" << std::endl;
        return;
    }

    for (TiXmlElement* elem = forwarding->FirstChildElement("entry"); elem != nullptr; elem = elem->NextSiblingElement("entry"))
    {
        TiXmlElement* msgIdElem = elem->FirstChildElement("msgid");
        TiXmlElement* urlElem = elem->FirstChildElement("url");

        if (msgIdElem && urlElem) 
        {
            const char* pId = msgIdElem->GetText();
            const char* pUrl = urlElem->GetText();

            if (pId ) 
            {
                int idNum = std::stoi(pId);

				if (pUrl)
				{
					m_mapConfig[idNum] = std::string(pUrl);
				}
				else
				{
					m_mapConfig[idNum] = ""; 
				}
            }
        }
    }

    return ;

}

void MsgCmdCfgManager::displayConfig() const
{
    for (const auto& pair : m_mapConfig)
    {
        std::cout << "ID: " << pair.first << " URL: " << pair.second << std::endl;
    }
}

const std::map<int, std::string>& MsgCmdCfgManager::getConfigMap() const 
{
    return m_mapConfig;
}

