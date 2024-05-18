#ifndef PROTOCOL_FORWARDER_H
#define PROTOCOL_FORWARDER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <sw/redis++/redis++.h>
#include <map>
#include <memory>
#include <string>

namespace net = boost::asio;            // Namespace for Boost ASIO
using tcp = boost::asio::ip::tcp;       // Simplify the tcp type

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace net = boost::asio;
using request_type = http::request<http::string_body>;
using response_type = http::response<http::string_body>;


class ProtocolForwarder 
{
public:
    ProtocolForwarder(net::io_context& ioc);
    void process_request(std::shared_ptr<tcp::socket> client_socket, std::shared_ptr<request_type> request);
    void initRedis();  

public:
	void ForwardPhoneVerify(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void ForwardPhoneLogin(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void ForwardBindJumpw(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void VerifyToken(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //��ȡ��Ծ�˺��ֻ���
	void GetJumpAccountPhone(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //��ȡ��Ծ�˺Ŵ�����ɫ���Ͻӿ�
    void GetJumpAccountAreaRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //���ô�����ɫ�ӿ�
    void SetAreaRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);
    
    //ˢ�½�ɫ��Ϣ�ӿ�
    void RefreshRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void RoleInfoQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void RankQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void StandingsQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void GetAreaConf(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //�Ծ�ս��
    void GamePlayingStandings(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //Ӣ��ʤ�����а��ѯ
    void HeroWinRateRankingQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

private:

    bool checkTokenExists(const std::string& strToken ,std::string& strUserBase);

    void UniversalForwarding(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest ,bool bIsSetCache = false);

    void UniversalForwarding(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest,std::vector<std::string>& vecFilterParams);

    void SetCacheLoginSign(std::string strUniqueID);


private:
    net::io_context& ioc_;
    std::shared_ptr<sw::redis::Redis> m_Redis;
 
};

#endif