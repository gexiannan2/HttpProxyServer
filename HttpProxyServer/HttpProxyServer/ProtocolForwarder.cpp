#include "ProtocolForwarder.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <sstream>
//#include <boost/asio/ssl.hpp> 
#include <boost/beast.hpp>
#include <regex> 
#include "config/MsgCmdCfgManager.h"
#include "ProtocolManager.h"
//#include <openssl/md5.h>
#include <sw/redis++/redis++.h>
#include <hiredis/hiredis.h>
#include "RateLimiter.h"

ProtocolForwarder::ProtocolForwarder(net::io_context& ioc ) : ioc_(ioc) 
{
	initRedis();  
}

void ProtocolForwarder::process_request(std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest) 
{
    std::istringstream iss(ptrRequest->body());

    boost::property_tree::ptree pt;
	boost::property_tree::read_json(iss, pt);
	int nMsgID = pt.get<int>("msgid");


	auto token_node = pt.get_child_optional("token");
	std::string strToken = "";
	if (token_node)
	{
		strToken = token_node.get().get_value<std::string>();
	}
   
#if define gexn
	// 获取IP地址字符串
	std::string strClientIP = ptrClientSocket->remote_endpoint().address().to_string();

	RateLimiter& limiter = RateLimiter::getInstance();
	std::pair<bool, std::string> result = limiter.IsRateLimited(strClientIP ,100);
	bool limited = result.first;
	std::string message = result.second;

	if (limited) 
	{
		std::cerr << message << std::endl;
		std::string httpResponse = "HTTP/1.1 429 Too Many Requests\r\n"
			"Content-Length: " + std::to_string(message.length() + 1) + "\r\n"  // +1 for newline character at the end of the message
			"Content-Type: text/plain\r\n"
			"Access-Control-Allow-Origin: *\r\n"  // 允许任何域进行跨域请求
			"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"  // 允许的HTTP方法
			"Access-Control-Allow-Headers: Content-Type, Authorization\r\n"  // 允许的头部
			"Access-Control-Allow-Credentials: true\r\n"  // 允许携带证书
			"\r\n" +
			message + "\n";

		try
		{
			boost::asio::write(*ptrClientSocket, boost::asio::buffer(httpResponse));
		}
		catch (const boost::system::system_error& e)
		{
			std::cerr << "Error writing to socket: " << e.what() << std::endl;

			// 关闭socket
			if (ptrClientSocket->is_open())
			{
				try
				{
					ptrClientSocket->close();
				}
				catch (const std::exception& e)
				{
					std::cerr << "Error closing socket: " << e.what() << std::endl;
				}
			}
		}
		return; // Stop processing since the rate limit has been exceeded
	}
	else
	{
		std::cout << "Request allowed." << std::endl;
	}

	if (!strToken.empty())
	{
		auto tokenLimitResult = limiter.IsRateLimited(strToken,200);
		if (tokenLimitResult.first)
		{
			std::cerr << tokenLimitResult.second << std::endl;
			std::string httpResponse = "HTTP/1.1 429 Too Many Requests\r\n"
				"Content-Length: " + std::to_string(tokenLimitResult.second.length() + 1) + "\r\n"
				"Content-Type: text/plain\r\n"
				"Access-Control-Allow-Origin: *\r\n"  // 允许所有域访问
				"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"  // 指定允许的HTTP方法
				"Access-Control-Allow-Headers: Content-Type, Authorization\r\n"  // 指定允许的HTTP头部
				"Access-Control-Allow-Credentials: true\r\n"  // 指示凭据是可以包括在请求中的
				"\r\n" +
				tokenLimitResult.second + "\n";


			try
			{
				boost::asio::write(*ptrClientSocket, boost::asio::buffer(httpResponse));
			}
			catch (const boost::system::system_error& e)
			{
				std::cerr << "Error writing to socket: " << e.what() << std::endl;

				// 关闭socket
				if (ptrClientSocket->is_open())
				{
					try
					{
						ptrClientSocket->close();
					}
					catch (const std::exception& e)
					{
						std::cerr << "Error closing socket: " << e.what() << std::endl;
					}
				}
			}
			return; // Stop processing since
		}
	}

#endif

    MsgCmdCfgManager* pCmdCfg = MsgCmdCfgManager::getInstance();
    if (pCmdCfg == nullptr)
    {
        std::cerr << "failed to get instance of configuration manager." << std::endl;
        return;
    }

    auto& manager = ProtocolManager::getInstance(ioc_);
    const std::map<int, std::string>& mapConfig = pCmdCfg->getConfigMap();
    auto target_it = mapConfig.find(nMsgID);
    if (target_it != mapConfig.end())
    {
        manager.processRequest(nMsgID, target_it->second, ptrClientSocket, ptrRequest);
    }
    else
    {
        std::cerr << "No target host configured for msgid: " << nMsgID << std::endl;
    }
}

void ProtocolForwarder::initRedis() 
{
#if 0
	try 
	{
		sw::redis::ConnectionOptions options;
		options.host = "123.56.24.230";
		options.port = 6379;
		options.password = "jumpwpt";  
		options.db = 9;

		// 使用 make_shared 来创建和初始化 Redis 对象
		m_Redis = std::make_shared<sw::redis::Redis>(options);
		std::cout << "Connected to Redis successfully." << std::endl;
	}
	catch (const sw::redis::Error& e) 
	{
		std::cerr << "Failed to connect to Redis: " << e.what() << std::endl;
	}
#endif
}

void ProtocolForwarder::ForwardPhoneVerify(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	std::vector<std::string> vecFilterParams = { "msg_type" };
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

void ProtocolForwarder::ForwardPhoneLogin(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{

	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest ,true); 
}

void ProtocolForwarder::ForwardBindJumpw(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest);
}

void  ProtocolForwarder::VerifyToken(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	std::string strRemoteIP = ptrClientSocket->remote_endpoint().address().to_string();

	std::istringstream iss(ptrRequest->body());
	boost::property_tree::ptree pt;

	std::string strToken;
	try
	{
		boost::property_tree::read_json(iss, pt);
		strToken = pt.get<std::string>("token");  // 获取 token 字段
	}
	catch (const boost::property_tree::ptree_error& e)
	{
		std::cerr << "Token not found in JSON: " << e.what() << std::endl;
	}

	std::string strUserBase = "";
	std::string strUser = "zjd300h_user:token:";
	std::string strCombinedToken = strUser + strToken;
	bool bIsExist = checkTokenExists(strCombinedToken, strUserBase);

	auto response = std::make_shared<response_type>();
	// 构建响应体
	boost::property_tree::ptree resBody;
	resBody.put("RES", bIsExist ? 0 : 1);
	resBody.put("ERR", "");

	if (bIsExist)
	{
		resBody.put("MSG", strUserBase);
		std::string strUniqueID = "";

		try
		{
			std::istringstream userBaseIss(strUserBase);
			boost::property_tree::ptree userBasePt;
			boost::property_tree::read_json(userBaseIss, userBasePt);
			strUniqueID = userBasePt.get<std::string>("UniqueId");
			std::cout << "Extracted UniqueId: " << strUniqueID << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error parsing user base JSON: " << e.what() << std::endl;
		}

		SetCacheLoginSign(strUniqueID);
	}
	else
	{
		resBody.put("MSG", NULL);
	}

	std::ostringstream buf;
	write_json(buf, resBody, false);

	std::cout << "Original JSON from oss: " << buf.str() << std::endl;
	std::regex regex("\"RES\":\\s*\"(\\d+)\"");
	std::string strJsonOutput = std::regex_replace(buf.str(), regex, "\"RES\":$1");

	std::cout << "Modified JSON: " << strJsonOutput << std::endl;

	response->body() = strJsonOutput;

	// 设置CORS头
	response->set(http::field::access_control_allow_origin, "*");
	response->set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
	response->set(http::field::access_control_allow_headers, "Content-Type, Authorization");
	response->set(http::field::access_control_allow_credentials, "true");

	std::cout << "send client  body: " << response->body() << std::endl;
	if (!std::cout)
	{
		std::cout.clear(); // 清除错误状态
	}

	try
	{
		http::write(*ptrClientSocket, *response);
	}
	catch (const boost::system::system_error& e) 
	{
		std::cerr << "Error writing to socket: " << e.what() << std::endl;

		// 关闭socket
		if (ptrClientSocket->is_open()) 
		{
			try 
			{
				ptrClientSocket->close();
			}
			catch (const std::exception& e) 
			{
				std::cerr << "Error closing socket: " << e.what() << std::endl;
			}
		}
	}
}

void ProtocolForwarder::GetJumpAccountPhone(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest);
}

void ProtocolForwarder::GetJumpAccountAreaRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	UniversalForwarding(strTargetHost, ptrClientSocket , ptrRequest);
}

void ProtocolForwarder::SetAreaRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	std::vector<std::string> vecFilterParams = { "jumpw_guid" ,  "jumpw_rlevel" };
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

void ProtocolForwarder::RefreshRole(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest);
}

void  ProtocolForwarder::RoleInfoQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{

	std::vector<std::string> vecFilterParams = { "AccountID" ,  "Guid" };
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

void ProtocolForwarder::RankQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{

	std::vector<std::string> vecFilterParams = { "TypeID" ,  "Guid" };
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

void ProtocolForwarder::StandingsQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	
	std::vector<std::string> vecFilterParams = { "RoleID" ,  "MatchType" ,"SearchIndex"};
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

void ProtocolForwarder::GetAreaConf(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest);
}

void ProtocolForwarder::GamePlayingStandings(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	std::vector<std::string> vecFilterParams = { "MTID" };
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

void ProtocolForwarder::HeroWinRateRankingQuery(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest)
{
	std::vector<std::string> vecFilterParams = { "MatchType" ,"HeroType","DateType"};
	UniversalForwarding(strTargetHost, ptrClientSocket, ptrRequest, vecFilterParams);
}

bool ProtocolForwarder::checkTokenExists(const std::string& strToken , std::string& strUserBase)
{
#if 0
	try
	{
		sw::redis::StringView tokenView(strToken);
		auto tokenExists = m_Redis->exists(tokenView);  // 使用 StringView 对象

		if (tokenExists) 
		{
			std::cout << "Token exists in Redis." << std::endl;
			return true;
		}
		else 
		{
			std::cout << "Token does not exist in Redis." << std::endl;
			return false;
		}
	}
	catch (const sw::redis::Error& e)
	{
		std::cerr << "Redis query failed: " << e.what() << std::endl;
		return false;
	}
#endif

	redisContext* c = redisConnect("123.56.24.230", 6379);
	if (c == NULL || c->err) 
	{
		if (c) 
		{
			std::cout << "Connection error: " << c->errstr << std::endl;
			redisFree(c);
		}
		else 
		{
			std::cout << "Connection error: can't allocate redis context" << std::endl;
		}
		return false;
	}
	std::string strPasswd = "jumpwpt";
	// 进行密码验证
	redisReply* auth_reply = (redisReply*)redisCommand(c, "AUTH %s", strPasswd.c_str());
	if (auth_reply == NULL || auth_reply->type == REDIS_REPLY_ERROR) 
	{
		std::cout << "Authentication failed." << std::endl;
		if (auth_reply)
		{
			freeReplyObject(auth_reply);
		}
		redisFree(c);
		return false;
	}
	freeReplyObject(auth_reply);  // 清理认证回应对象

	// 切换到 db9 数据库
	redisReply* select_reply = (redisReply*)redisCommand(c, "SELECT 9");
	if (select_reply == NULL || select_reply->type == REDIS_REPLY_ERROR) 
	{
		std::cout << "Database selection failed." << std::endl;
		if (select_reply)
		{
			freeReplyObject(select_reply);
		}
		redisFree(c);
		return false;
	}
	freeReplyObject(select_reply);  // 清理选择数据库回应对象
	std::string strSpliceUniqueID ;
	redisReply* reply = (redisReply*)redisCommand(c, "GET %s", strToken.c_str());
	bool found = false;
	if (reply != NULL && reply->type == REDIS_REPLY_STRING)
	{
		strUserBase = reply->str;  
		std::cout << "Retrieved token, value: " << strUserBase << std::endl;
		freeReplyObject(reply);  

		strSpliceUniqueID = "zjd300h_user:uniqueid:" + strUserBase;
		reply = (redisReply*)redisCommand(c, "GET %s", strSpliceUniqueID.c_str());
		if (reply != NULL && reply->type == REDIS_REPLY_STRING)
		{
			std::cout << "Retrieved user data for base: " << reply->str << std::endl;
			strUserBase = reply->str;  
			found = true;
		}
	}

	// 清理
	if (reply)
	{
		freeReplyObject(reply);
	}
	redisFree(c);
	return found;
}

void ProtocolForwarder::UniversalForwarding(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest ,bool bIsSetCache)
{
	std::regex url_regex(R"(^(https?)://([^/:]+)(?::(\d+))?(/.*)?$)");
	std::smatch url_parts;
	std::string hostname;
	std::string strProtocol;
	std::string strPort = "80"; // Default HTTP port

	if (std::regex_match(strTargetHost, url_parts, url_regex))
	{
		if (url_parts.size() < 5u)
		{
			return;
		}

		strProtocol = url_parts[1];
		hostname = url_parts[2];
		if (url_parts[3].matched)
		{
			strPort = url_parts[3]; // Use the specified port if available
		}
		else
		{
			strPort = (strProtocol == "https") ? "443" : "80"; // Default to 443 for HTTPS
		}
	}

	tcp::resolver resolver(ioc_);
	boost::asio::ip::tcp::resolver::results_type results;

	try
	{
		results = resolver.resolve(hostname, strPort);
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << "Network error: " << e.what() << std::endl;
		return;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl; 
		return;
	}

	std::string strRemoteIP = ptrClientSocket->remote_endpoint().address().to_string();
	std::istringstream iss(ptrRequest->body());
	boost::property_tree::ptree pt;

	boost::property_tree::read_json(iss, pt);
	pt.put("user_ip", strRemoteIP);
	pt.erase("msgid");

	std::ostringstream oss;
	boost::property_tree::write_json(oss, pt, false);

	ptrRequest->body() = oss.str();

	ptrRequest->prepare_payload();

	std::cout << "forward original target：" << ptrRequest->target() << std::endl;
	std::cout << "forward  original body: " << ptrRequest->body() << std::endl;

	std::string strTarget = url_parts[4];
	http::request<http::string_body> req{ http::verb::post, strTarget, 11 };
	req.set(http::field::host, hostname);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.set(http::field::content_type, "application/json");


	std::cout << "Original JSON from oss: " << oss.str() << std::endl;

	req.body() = oss.str();
	req.prepare_payload();


	tcp::socket forward_socket(ioc_);
	try
	{
		net::connect(forward_socket, results.begin(), results.end());
		http::write(forward_socket, req);
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << "连接异常: " << e.what() << std::endl;
		return;
	}

	auto buffer = std::make_shared<boost::beast::flat_buffer>();
	auto response = std::make_shared<response_type>();
	http::read(forward_socket, *buffer, *response);

	// 设置CORS头
	response->set(http::field::access_control_allow_origin, "*");
	response->set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
	response->set(http::field::access_control_allow_headers, "Content-Type, Authorization");
	response->set(http::field::access_control_allow_credentials, "true");


	if (bIsSetCache)
	{
		std::string strUniqueId = "";
		try
		{
			std::istringstream response_iss(response->body());
			boost::property_tree::ptree response_pt;
			boost::property_tree::read_json(response_iss, response_pt);

			if (response_pt.get_optional<std::string>("UniqueId"))
			{
				strUniqueId = response_pt.get<std::string>("UniqueId");
				std::cout << "Extracted unique_id: " << strUniqueId << std::endl;
			}
			else
			{
				std::cerr << "unique_id not found in response." << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error parsing response JSON: " << e.what() << std::endl;
		}

		SetCacheLoginSign(strUniqueId);
	}

	std::cout << "send client  body: " << response->body() << std::endl;
	if (!std::cout)
	{
		std::cout.clear(); // 清除错误状态
	}

	try
	{
		http::write(*ptrClientSocket, *response);
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << "Error writing to socket: " << e.what() << std::endl;

		// 关闭socket
		if (ptrClientSocket->is_open())
		{
			try
			{
				ptrClientSocket->close();
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error closing socket: " << e.what() << std::endl;
			}
		}
	}
}

void ProtocolForwarder::UniversalForwarding(const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest, std::vector<std::string>& vecFilterParams)
{
	std::regex url_regex(R"(^(https?)://([^/:]+)(?::(\d+))?(/.*)?$)");
	std::smatch url_parts;
	std::string hostname;
	std::string strProtocol;
	std::string strPort = "80"; // Default HTTP port

	if (std::regex_match(strTargetHost, url_parts, url_regex))
	{
		if (url_parts.size() < 5u)
		{
			return;
		}

		strProtocol = url_parts[1];
		hostname = url_parts[2];
		if (url_parts[3].matched)
		{
			strPort = url_parts[3]; // Use the specified port if available
		}
		else
		{
			strPort = (strProtocol == "https") ? "443" : "80"; // Default to 443 for HTTPS
		}
	}

	tcp::resolver resolver(ioc_);
	boost::asio::ip::tcp::resolver::results_type results;

	try
	{
		results = resolver.resolve(hostname, strPort);
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << "Network error: " << e.what() << std::endl;
		return;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return;
	}

	std::string strRemoteIP = ptrClientSocket->remote_endpoint().address().to_string();
	std::istringstream iss(ptrRequest->body());
	boost::property_tree::ptree pt;

	boost::property_tree::read_json(iss, pt);
	pt.put("user_ip", strRemoteIP);
	pt.erase("msgid");

	std::ostringstream oss;
	boost::property_tree::write_json(oss, pt, false);

	std::string strJsonOutput = oss.str();

	for (const auto& param : vecFilterParams)
	{
		std::regex regex_param("\"" + param + "\":\\s*\"(\\d+)\"");
		strJsonOutput = std::regex_replace(strJsonOutput, regex_param, "\"" + param + "\":$1");
	}

	ptrRequest->body() = strJsonOutput;
	ptrRequest->prepare_payload();

	std::string strTarget = url_parts[4];
	http::request<http::string_body> req{ http::verb::post, strTarget, 11 };
	req.set(http::field::host, hostname);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.set(http::field::content_type, "application/json");

	req.body() = strJsonOutput;
	req.prepare_payload();

	std::cout << "forward  original body: " << strJsonOutput << std::endl;

	tcp::socket forward_socket(ioc_);
	try
	{
		net::connect(forward_socket, results.begin(), results.end());
		http::write(forward_socket, req);
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << "连接异常: " << e.what() << std::endl;
		return;
	}

	auto buffer = std::make_shared<boost::beast::flat_buffer>();
	auto response = std::make_shared<response_type>();
	http::read(forward_socket, *buffer, *response);

	// 设置CORS头
	response->set(http::field::access_control_allow_origin, "*");
	response->set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
	response->set(http::field::access_control_allow_headers, "Content-Type, Authorization");
	response->set(http::field::access_control_allow_credentials, "true");

	std::cout << "send client  body: " << response->body() << std::endl;
	if (!std::cout)
	{
		std::cout.clear(); // 清除错误状态
	}

	try
	{
		http::write(*ptrClientSocket, *response);
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << "Error writing to socket: " << e.what() << std::endl;

		// 关闭socket
		if (ptrClientSocket->is_open())
		{
			try
			{
				ptrClientSocket->close();
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error closing socket: " << e.what() << std::endl;
			}
		}
	}
}

void ProtocolForwarder::SetCacheLoginSign(std::string strUniqueID)
{
	redisContext* c = redisConnect("123.56.24.230", 6379);
	if (c == NULL || c->err)
	{
		if (c)
		{
			std::cout << "Connection error: " << c->errstr << std::endl;
			redisFree(c);
		}
		else
		{
			std::cout << "Connection error: can't allocate redis context" << std::endl;
		}
		return;
	}

	std::string strPasswd = "jumpwpt";
	// 进行密码验证
	redisReply* auth_reply = (redisReply*)redisCommand(c, "AUTH %s", strPasswd.c_str());
	if (auth_reply == NULL || auth_reply->type == REDIS_REPLY_ERROR)
	{
		std::cout << "Authentication failed." << std::endl;
		if (auth_reply)
		{
			freeReplyObject(auth_reply);
		}
		redisFree(c);
		return;
	}
	freeReplyObject(auth_reply);  // 清理认证回应对象

	// 切换到 db9 数据库
	redisReply* select_reply = (redisReply*)redisCommand(c, "SELECT 9");
	if (select_reply == NULL || select_reply->type == REDIS_REPLY_ERROR)
	{
		std::cout << "Database selection failed." << std::endl;
		if (select_reply)
		{
			freeReplyObject(select_reply);
		}
		redisFree(c);
		return;
	}
	freeReplyObject(select_reply);  // 清理选择数据库回应对象

	// 获取服务器当前日期
	time_t now = time(0);
	tm ltm;
	char dateBuffer[64] = { 0 };
	localtime_s(&ltm, &now);
	snprintf(dateBuffer, sizeof(dateBuffer), "%04d%02d%02d", 1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday);
	std::string strDate = dateBuffer;

	// 执行 SADD 命令
	std::string strCommand = "SADD zjd300h_reports:day_login:" + strDate + " " + strUniqueID;
	redisReply* sadd_reply = (redisReply*)redisCommand(c, strCommand.c_str());
	if (sadd_reply == NULL || sadd_reply->type == REDIS_REPLY_ERROR)
	{
		std::cout << "Failed to execute SADD command." << std::endl;
		if (sadd_reply)
		{
			freeReplyObject(sadd_reply);
		}
		redisFree(c);
		return;
	}
	std::cout << "SADD command executed successfully for uniqueID: " << strUniqueID << std::endl;
	freeReplyObject(sadd_reply);  // 清理 SADD 命令回应对象

	redisFree(c);  // 关闭 Redis 连接
	return;
}

