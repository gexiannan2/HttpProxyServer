PROJECT(tinyxml)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 
INCLUDE_DIRECTORIES( . )
AUX_SOURCE_DIRECTORY(. DIR_SRCS)

SET(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/../bin)	##编译完成后输出路径

SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -Wno-unused-variable -Wno-deprecated -Wall -rdynamic -std=c++11")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -g -ggdb")##编译Debug
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")##编译release
add_library( tinyxmlx ${DIR_SRCS} )   