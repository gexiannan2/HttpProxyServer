#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include <unordered_map>
#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>


using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace net = boost::asio;
using request_type = http::request<http::string_body>;
using response_type = http::response<http::string_body>;


class ProtocolManager 
{
public:
	using Callback = std::function<void(const std::string&, std::shared_ptr<tcp::socket>, std::shared_ptr<request_type>)>;

	explicit ProtocolManager(boost::asio::io_context& io_context)
		: io_context_(io_context) {}

	static ProtocolManager& getInstance(boost::asio::io_context& io_context);
	void registerProtocol(int msgId, Callback callback);
	void processRequest(int msgId, const std::string& strTargetHost, std::shared_ptr<tcp::socket> ptrClientSocket, std::shared_ptr<request_type> ptrRequest);
	void Init();  // 初始化所有协议

private:
	boost::asio::io_context& io_context_;
	std::unordered_map<int, Callback> callbacks;
};
#endif 