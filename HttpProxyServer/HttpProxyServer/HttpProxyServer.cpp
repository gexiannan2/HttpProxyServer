#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <sstream>
#include "ProtocolForwarder.h"  
#include "config/MsgCmdCfgManager.h"
#include "config/ConCfgManager.h"
#include "ProtocolManager.h"
#include "PostManager.h"


using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace net = boost::asio;
using request_type = http::request<http::string_body>;
using response_type = http::response<http::string_body>;


// 用于存储socket和请求的结构
using request_pair = std::pair<std::shared_ptr<tcp::socket>, std::shared_ptr<request_type>>;
std::queue<request_pair> request_queue;
std::mutex queue_mutex;
std::condition_variable queue_cond_var;

// 异步读取请求并加入队列
void async_read_request(std::shared_ptr<tcp::socket> socket, net::io_context& ioc) 
{
	auto buffer = std::make_shared<boost::beast::flat_buffer>();
	auto request = std::make_shared<request_type>();

	// 异步读取请求
	http::async_read(*socket, *buffer, *request,
		[socket, buffer, request, &ioc](boost::beast::error_code ec, std::size_t bytes_transferred) 
		{
			std::cout << " async read start!" << std::endl;
			if (!ec)
			{
				// 检查是否是OPTIONS预检请求
				if (request->method() == http::verb::options) {
					// 创建一个响应
					auto response = std::make_shared<response_type>(http::status::ok, request->version());
					response->set(http::field::server, "C++ Server");
					response->set(http::field::access_control_allow_origin, "*");
					response->set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
					response->set(http::field::access_control_allow_headers, "Content-Type, Authorization");
					response->set(http::field::access_control_max_age, "3600"); // 可选，设置预检请求的结果能够被缓存多长时间
					response->content_length(response->body().size());
					response->prepare_payload();

					// 发送响应
					http::async_write(*socket, *response,
						[socket, response](boost::beast::error_code ec, std::size_t) {
							socket->shutdown(tcp::socket::shutdown_send, ec);
						}
					);
					return;
				}

				// 打印读取成功的信息
				std::cout << "read success，request target：" << request->target() << std::endl;
				std::cout << "read success，request body：" << request->body() << std::endl;

				// 放入请求队列
				std::lock_guard<std::mutex> lock(queue_mutex);
				request_queue.emplace(socket, request);
				queue_cond_var.notify_one();
			}
			else
			{
				std::cerr << "read error：" << ec.message() << std::endl;
			}
			std::cout << " async read callback end !" << std::endl;
		}
	);
}


// 处理每个请求并转发到目标服务器
void process_request(std::shared_ptr<tcp::socket> client_socket, std::shared_ptr<request_type> request, net::io_context& ioc)
{
	ProtocolForwarder forwarder(ioc);
	forwarder.process_request(client_socket, request);
}

void handle_requests(net::io_context& ioc)
{
	while (true)
	{
		request_pair req_pair;

		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			queue_cond_var.wait(lock, [] { return !request_queue.empty(); });
			req_pair = request_queue.front();
			request_queue.pop();
		}

		if (req_pair.first && req_pair.second)
		{
			process_request(req_pair.first, req_pair.second, ioc);
		}
	}
}


int main()
{
	MsgCmdCfgManager::Init("Config/msg_cmd.xml");  

	ConCfgManager::Init("Config/config.xml");

	net::io_context ioc;

	auto& manager = ProtocolManager::getInstance(ioc);
	manager.Init();



	//////////////////////////test /////////////////////////////////////////
	PostManager* pPostMgr = PostManager::getInstance();
	if (!pPostMgr) {
		std::cerr << "Failed to initialize the PostManager." << std::endl;
		return -1;
	}

	pPostMgr->addPost(1, "哈哈 我是英国人");
	pPostMgr->addPost(2, "哈哈 你好吗");
	pPostMgr->addPost(3, "你好 我是哈哈");

	//pPostMgr->searchByTitle("哈哈");
	pPostMgr->searchByTitle("gexiannan");

	Post* post = pPostMgr->findPost(1);
	if (post) {
		std::cout << "Found post: UniqueId: " << post->uniqueId << " - Title: " << post->title << std::endl;
	}
	else {
		std::cout << "Post not found." << std::endl;
	}

	pPostMgr->deletePost(2);

	pPostMgr->searchByTitle("哈哈");
	////////////////////////////test////////////////////////////////////////


	tcp::acceptor acceptor(ioc, tcp::endpoint(net::ip::make_address(ConCfgManager::getInstance()->GetIP()), ConCfgManager::getInstance()->GetPort()));

	std::function<void()> do_accept;
	std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(ioc);

	do_accept = [&]() 
		{
			acceptor.async_accept(*socket, [&](boost::system::error_code ec) 
				{
					if (!ec) 
					{
						async_read_request(socket, ioc);
						socket = std::make_shared<tcp::socket>(ioc); // 创建新的socket以接受更多连接
						do_accept();
					}
			});
		};

	do_accept(); // 开始接受连接

	std::thread ioc_thread([&]() { ioc.run(); });

	std::cout << "HttpProxyServer is starting ..." << std::endl;

	std::thread request_handler_thread(handle_requests, std::ref(ioc));

	// 等待线程结束
	ioc_thread.join();
	request_handler_thread.join();

	return  0;
}
