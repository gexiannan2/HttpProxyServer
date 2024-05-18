#ifndef CONFIG_MANAGER_TEMPLATE_H
#define CONFIG_MANAGER_TEMPLATE_H

#include <string>
#include <mutex>

template <typename T>
class ConfigManagerTemplate 
{
protected:
	static T* instance;
	static std::mutex mutex;

	ConfigManagerTemplate() {}

public:
	// Disable copy constructor and assignment operator
	ConfigManagerTemplate(const ConfigManagerTemplate&) = delete;
	ConfigManagerTemplate& operator=(const ConfigManagerTemplate&) = delete;

	static T* getInstance() 
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (instance == nullptr) 
		{
			instance = new T();
		}
		return instance;
	}

	virtual void loadConfig(const std::string& xmlFilePath) = 0; 
};

template <typename T>
T* ConfigManagerTemplate<T>::instance = nullptr;

template <typename T>
std::mutex ConfigManagerTemplate<T>::mutex;

#endif