#include "PostManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <future>
#include <cstring>

// ��ʼ����̬��Ա
PostManager* PostManager::instance = nullptr;
std::mutex PostManager::mtx;

PostManager::PostManager() {
    loadIndexFromFile();
    mapFile();
}

PostManager* PostManager::getInstance() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        instance = new PostManager();
    }
    return instance;
}

void PostManager::savePostsToFile() {
    std::ofstream outFile("posts.txt", std::ios::trunc);
    if (outFile.is_open()) {
        for (const auto& post : posts) {
            std::streamoff pos = outFile.tellp();
            fileIndex[post.uniqueId] = pos;
            outFile << post.uniqueId << "," << post.title << std::endl;
        }
        outFile.close();
    }
    else {
        std::cerr << "Unable to open file for writing." << std::endl;
    }
    saveIndexToFile(); // ���������ļ�
    mapFile(); // ����ӳ���ļ�
}

void PostManager::loadPostsFromFile() {
    // ����Ҫ������ʱ�����������ӣ�ֻ��Ҫ��������
}

void PostManager::saveIndexToFile() {
    std::ofstream indexFile("index.txt", std::ios::trunc);
    if (indexFile.is_open()) {
        for (const auto& entry : fileIndex) {
            indexFile << entry.first << "," << entry.second << std::endl;
        }
        indexFile.close();
    }
    else {
        std::cerr << "Unable to open index file for writing." << std::endl;
    }
}

void PostManager::loadIndexFromFile() {
    std::ifstream indexFile("index.txt");
    if (indexFile.is_open()) {
        std::string line;
        while (getline(indexFile, line)) {
            std::istringstream iss(line);
            int uniqueId;
            std::streamoff pos;
            if (iss >> uniqueId && iss >> pos) {
                fileIndex[uniqueId] = pos;
            }
        }
        indexFile.close();
    }
    else {
        std::cerr << "Unable to open index file for reading." << std::endl;
    }
}

void PostManager::addPost(int uniqueId, const std::string& title) {
    posts.insert({ uniqueId, title });
    std::async(std::launch::async, &PostManager::savePostsToFile, this);
    // ���»���
    cache[uniqueId] = { uniqueId, title };
    updateLRU(uniqueId);
    evictIfNeeded();
}

void PostManager::deletePost(int uniqueId) {
    auto& id_index = posts.get<0>();
    for (auto it = id_index.begin(); it != id_index.end(); ++it) {
        if (it->uniqueId == uniqueId) {
            id_index.erase(it);
            std::async(std::launch::async, &PostManager::savePostsToFile, this);
            // �ӻ�����ɾ��
            cache.erase(uniqueId);
            lru.remove(uniqueId);
            return;
        }
    }
    std::cout << "Post with UniqueId " << uniqueId << " not found." << std::endl;
}

void PostManager::searchByTitle(const std::string& title) {
    auto& title_index = posts.get<0>();
    bool found = false;

    // �ȴ��ڴ�������
    for (auto it = title_index.begin(); it != title_index.end(); ++it) {
        if (it->title.find(title) != std::string::npos) {
            if (!found) {
                std::cout << "Posts found in memory:" << std::endl;
                found = true;
            }
            std::cout << "UniqueId: " << it->uniqueId << " - Title: " << it->title << std::endl;
            // ���»���
            cache[it->uniqueId] = *it;
            updateLRU(it->uniqueId);
            evictIfNeeded();
        }
    }

    // ����ڴ���û���ҵ�������ļ�������
    if (!found) {
        std::vector<Post> filePosts = searchByTitleFromFile(title);
        if (!filePosts.empty()) {
            std::cout << "Posts found in file:" << std::endl;
            for (const auto& post : filePosts) {
                std::cout << "UniqueId: " << post.uniqueId << " - Title: " << post.title << std::endl;
                // ���»���
                cache[post.uniqueId] = post;
                updateLRU(post.uniqueId);
                evictIfNeeded();
            }
        }
        else {
            std::cout << "No posts found." << std::endl;
        }
    }
}

std::vector<Post> PostManager::searchByTitleFromFile(const std::string& title) {
    std::vector<Post> result;
    if (mappedData) {
#if defined(_WIN32) || defined(_WIN64)
        char* ptr = mappedData.get();
        while (ptr < mappedData.get() + fileSize) {
#else
        char* ptr = mappedData.get();
        while (ptr < mappedData.get() + fileSize) {
#endif
            int uniqueId;
            char* endPtr;
            uniqueId = strtol(ptr, &endPtr, 10);
            if (endPtr == ptr) break; // �����ļ���β
            ptr = endPtr;
            std::string postTitle;
            while (*ptr != '\n' && *ptr != '\0') {
                postTitle += *ptr++;
            }
            ptr++; // �������з�
            if (postTitle.find(title) != std::string::npos) {
                result.push_back({ uniqueId, postTitle });
            }
        }
        }
    return result;
    }

Post* PostManager::findPost(int uniqueId) {
    // �ȼ�黺��
    auto cacheIt = cache.find(uniqueId);
    if (cacheIt != cache.end()) {
        updateLRU(uniqueId);
        return &cacheIt->second;
    }

    // ���������û�У�����ļ��в���
    auto it = fileIndex.find(uniqueId);
    if (it != fileIndex.end()) {
        if (mappedData) {
            char* ptr = mappedData.get() + it->second;
            int fileUniqueId;
            char* endPtr;
            fileUniqueId = strtol(ptr, &endPtr, 10);
            if (endPtr == ptr) return nullptr;
            ptr = endPtr;
            std::string title;
            while (*ptr != '\n' && *ptr != '\0') {
                title += *ptr++;
            }
            Post post{ fileUniqueId, title };
            // ���»���
            cache[fileUniqueId] = post;
            updateLRU(fileUniqueId);
            evictIfNeeded();
            return &cache[fileUniqueId];
        }
    }
    return nullptr;
}

void PostManager::updateLRU(int uniqueId) {
    lru.remove(uniqueId);
    lru.push_front(uniqueId);
}

void PostManager::evictIfNeeded() {
    if (cache.size() > CACHE_SIZE) {
        int leastUsed = lru.back();
        lru.pop_back();
        cache.erase(leastUsed);
    }
}

void PostManager::mapFile() {
#if defined(_WIN32) || defined(_WIN64)
    fileHandle = CreateFile(TEXT("posts.txt"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Unable to open file for mapping." << std::endl;
        return;
    }

    fileSize = GetFileSize(fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        std::cerr << "Unable to determine file size." << std::endl;
        CloseHandle(fileHandle);
        return;
    }

    mapHandle = CreateFileMapping(fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapHandle == NULL) {
        std::cerr << "Unable to create file mapping." << std::endl;
        CloseHandle(fileHandle);
        return;
    }

    mappedData.reset((char*)MapViewOfFile(mapHandle, FILE_MAP_READ, 0, 0, fileSize), [this](char* ptr) {
        UnmapViewOfFile(ptr);
        CloseHandle(mapHandle);
        CloseHandle(fileHandle);
        });

    if (mappedData.get() == NULL) {
        std::cerr << "Mapping file failed." << std::endl;
        mappedData.reset();
        CloseHandle(mapHandle);
        CloseHandle(fileHandle);
    }
#else
    fileDescriptor = open("posts.txt", O_RDONLY);
    if (fileDescriptor == -1) {
        std::cerr << "Unable to open file for mapping." << std::endl;
        return;
    }

    fileSize = lseek(fileDescriptor, 0, SEEK_END);
    if (fileSize == -1) {
        std::cerr << "Unable to determine file size." << std::endl;
        close(fileDescriptor);
        return;
    }

    mappedData.reset((char*)mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor, 0), [this](char* ptr) {
        munmap(ptr, fileSize);
        close(fileDescriptor);
        });

    if (mappedData.get() == MAP_FAILED) {
        std::cerr << "Mapping file failed." << std::endl;
        mappedData.reset();
        close(fileDescriptor);
    }
#endif
}

void PostManager::unmapFile() {
    mappedData.reset();
}
