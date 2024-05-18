#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <unordered_map>
#include <string>
#include <chrono>
#include <mutex>
#include <utility>

class RateLimiter 
{
public:
	static RateLimiter& getInstance();

	std::pair<bool, std::string> IsRateLimited(const std::string& strIdentifier, int nIntervalMillis);

private:
	RateLimiter();
	RateLimiter(const RateLimiter&) = delete;
	RateLimiter& operator=(const RateLimiter&) = delete;

	struct stRateLimitInfo
	{
		std::chrono::steady_clock::time_point lastRequestTime;
	};

	std::unordered_map<std::string, std::unordered_map<int, stRateLimitInfo>> m_mapAccessRecords;
	std::mutex mutex_;
};

#endif // RATELIMITER_H


