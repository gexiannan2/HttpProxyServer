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

    //获取跳跃账号手机号
	void GetJumpAccountPhone(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //获取跳跃账号大区角色集合接口
    void GetJumpAccountAreaRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //设置大区角色接口
    void SetAreaRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);
    
    //刷新角色信息接口
    void RefreshRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void RoleInfoQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void RankQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void StandingsQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    void GetAreaConf(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //对局战绩
    void GamePlayingStandings(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);

    //英雄胜率排行榜查询
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