#include "ProtocolManager.h"
#include "ProtocolForwarder.h"
#include <iostream>

ProtocolManager& ProtocolManager::getInstance(boost::asio::io_context& io_context)
{
	static ProtocolManager instance(io_context);
	return instance;
}

void ProtocolManager::registerProtocol(int msgId, Callback callback) {
	callbacks[msgId] = callback;
}

void ProtocolManager::processRequest(int msgId, const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest) {
	auto it = callbacks.find(msgId);
	if (it != callbacks.end()) 
	{
		it->second(strTargetHost, ptrClientSocket, ptrRequest);
	}
	else 
	{
		std::cerr << "No callback registered for msgId: " << msgId << std::endl;
	}
}

void ProtocolManager::Init() 
{
	static ProtocolForwarder forwarder(io_context_); // 使用传入的io_context创建forwarder

	registerProtocol(1001, std::bind(&ProtocolForwarder::ForwardPhoneVerify, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1002, std::bind(&ProtocolForwarder::ForwardPhoneLogin, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1003, std::bind(&ProtocolForwarder::ForwardBindJumpw, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1004, std::bind(&ProtocolForwarder::VerifyToken, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1005, std::bind(&ProtocolForwarder::GetJumpAccountPhone, &forwarder ,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1006, std::bind(&ProtocolForwarder::GetJumpAccountAreaRole, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1007, std::bind(&ProtocolForwarder::SetAreaRole, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1008, std::bind(&ProtocolForwarder::RefreshRole, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1009, std::bind(&ProtocolForwarder::RoleInfoQuery, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1010, std::bind(&ProtocolForwarder::RankQuery, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1011, std::bind(&ProtocolForwarder::StandingsQuery, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1012, std::bind(&ProtocolForwarder::GetAreaConf, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1013, std::bind(&ProtocolForwarder::GamePlayingStandings, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	registerProtocol(1014, std::bind(&ProtocolForwarder::HeroWinRateRankingQuery, &forwarder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

}
