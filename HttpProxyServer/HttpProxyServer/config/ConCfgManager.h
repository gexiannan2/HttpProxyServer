#ifndef _CON_CFG_MANAGER_H_
#define _CON_CFG_MANAGER_H_

#include "ConfigManagerTemplate.h"
#include <string>
#include <map>

class ConCfgManager : public ConfigManagerTemplate<ConCfgManager>
{

public:
	void loadConfig(const std::string& strXmlFilePath) override;

	void displayConfig() const;

	
	static const std::string& GetIP()  { return m_strIP; }

	static int GetPort()  { return m_nPort;  }

public:
	static void Init(const std::string& strXmlFilePath)
	{
		ConCfgManager* pCmdCfg = getInstance();
		if (pCmdCfg)
		{
			pCmdCfg->loadConfig(strXmlFilePath);
		}

		return;
	}

private:
	static std::string m_strIP;
	static int m_nPort;
};

#endif 