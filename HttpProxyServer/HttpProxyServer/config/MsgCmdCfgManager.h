#ifndef MSG_CMD_CFG_MANAGER_H
#define MSG_CMD_CFG_MANAGER_H

#include "ConfigManagerTemplate.h"
#include <string>
#include <map>

class MsgCmdCfgManager : public ConfigManagerTemplate<MsgCmdCfgManager>
{

public:
    void loadConfig(const std::string& strXmlFilePath) override;

    void displayConfig() const;

    const std::map<int, std::string>& getConfigMap() const;

public:
	static void Init(const std::string& strXmlFilePath)
	{
		MsgCmdCfgManager* pCmdCfg = getInstance();
		if (pCmdCfg)
		{
			pCmdCfg->loadConfig(strXmlFilePath);
		}

		return;
	}

private:
	std::map<int, std::string> m_mapConfig;
};

#endif 





