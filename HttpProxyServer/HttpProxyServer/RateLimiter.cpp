#include "RateLimiter.h"

RateLimiter& RateLimiter::getInstance() 
{
	static RateLimiter instance;
	return instance;
}

std::pair<bool, std::string> RateLimiter::IsRateLimited(const std::string& strIdentifier, int nIntervalMillis) 
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto& record = m_mapAccessRecords[strIdentifier][nIntervalMillis];
	auto now = std::chrono::steady_clock::now();

	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - record.lastRequestTime) < std::chrono::milliseconds(nIntervalMillis))
	{
		return { true, "�������Ƶ�������Ժ����ԡ�" };
	}

	record.lastRequestTime = now;
	return { false, "" };
}

RateLimiter::RateLimiter() {}

