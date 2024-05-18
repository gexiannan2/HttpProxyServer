#ifndef POST_MANAGER_H
#define POST_MANAGER_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <string>
#include <mutex>
#include <future>
#include <unordered_map>
#include <list>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

struct Post {
    int uniqueId;
    std::string title;
};

typedef boost::multi_index_container<
    Post,
    boost::multi_index::indexed_by<
    boost::multi_index::ordered_non_unique<boost::multi_index::member<Post, std::string, &Post::title>>
    >
> PostIndex;

class PostManager {
private:
    static PostManager* instance;
    static std::mutex mtx;
    PostIndex posts;
    std::unordered_map<int, std::streamoff> fileIndex; // 文件索引
    std::unordered_map<int, Post> cache;               // 内存缓存
    std::list<int> lru;                                // LRU 缓存

    const size_t CACHE_SIZE = 10000;                   // 缓存大小

#if defined(_WIN32) || defined(_WIN64)
    HANDLE fileHandle;
    HANDLE mapHandle;
    DWORD fileSize;
#else
    int fileDescriptor;
    size_t fileSize;
#endif
    std::shared_ptr<char> mappedData;

    // 私有构造函数
    PostManager();

    void savePostsToFile();
    void loadPostsFromFile();
    void saveIndexToFile();
    void loadIndexFromFile();

    void updateLRU(int uniqueId);
    void evictIfNeeded();

    void mapFile();
    void unmapFile();

public:
    // 禁止拷贝和赋值
    PostManager(const PostManager&) = delete;
    PostManager& operator=(const PostManager&) = delete;

    static PostManager* getInstance();

    void addPost(int uniqueId, const std::string& title);
    void deletePost(int uniqueId);
    void searchByTitle(const std::string& title);
    Post* findPost(int uniqueId);
    std::vector<Post> searchByTitleFromFile(const std::string& title);
};

#endif // POST_MANAGER_H
