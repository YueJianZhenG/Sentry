if (MSVC)
    #add_compile_options(/EHs-) # 添加全局编译选项
    add_compile_options(/wd4267)
    add_compile_options(/wd4244)
    add_compile_options(/wd4996)
    enable_language(C CXX ASM_MASM)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else ()
    enable_language(C CXX ASM)
    #add_compile_options(-fno-exceptions) # 添加全局编译选项
endif ()

add_definitions(-w) #忽略警告
add_definitions(-D ASIO_HAS_CHRONO)
add_definitions(-D ASIO_STANDALONE)
add_definitions(-D ASIO_HAS_MOVE)

add_definitions(-D ASIO_HAS_STD_THREAD)

#add_definitions(-D ASIO_HAS_THREADS)

if (MSVC)
    add_definitions(-D _WIN32_WINNT=0x0601)
    add_compile_options(/W4 /wd4100 /wd4127 /wd4819)
endif ()

#option(protobuf_BUILD_TESTS OFF)
#option(protobuf_BUILD_EXAMPLES OFF)
#option(protobuf_BUILD_CONFORMANCE OFF)
#option(protobuf_BUILD_SHARED_LIBS ON)

option(protobuf_BUILD_TESTS OFF)
option(protobuf_BUILD_EXAMPLES OFF)
option(protobuf_BUILD_CONFORMANCE OFF)
option(LEVELDB_BUILD_TESTS OFF)
option(LEVELDB_BUILD_BENCHMARKS OFF)
option(MI_BUILD_SHARED OFF)
option(MI_BUILD_OBJECT OFF)
option(MI_BUILD_TESTS OFF)

option(__ENABLE_LEVEL_DB OFF)

if (WIN32 AND NOT MSVC)
    #set(protobuf_BUILD_SHARED_LIBS ON) #编译动态库
endif ()

option(__ENABLE_MI_MALLOC__ "使用mimalloc" OFF)
option(ONLY_MAIN_THREAD "启用单线程模式" OFF)
option(__NET_ERROR_LOG__ "打印网络层错误" ON)

option(__CONSOLE_LOG__ "控制台打印日志" ON)
option(__ENABLE_SYSTEM_DEBUG "打印系统日志" ON)
option(__ENABLE_OPEN_SSL__ "开启openssl" ON)

option(__SHARE_PTR_COUNTER__ "开启指针计算查询" OFF)

option(__ENABLE_SHARE_STACK__ "协程使用共享栈" ON)
option(__ENABLE_LEVEL_DB__ "使用leveldb数据库" OFF)
option(__ENABLE_A_SCAN__ "使用AddressSanitizer检查内存问题" OFF)

if (__ENABLE_A_SCAN__)
    message("使用asan禁用mimalloc")
    message("使用asan禁用协程共享栈")
    set(__ENABLE_MI_MALLOC__ OFF CACHE BOOL "" FORCE)
    set(__ENABLE_SHARE_STACK__ OFF CACHE BOOL "" FORCE)
endif ()

add_definitions(-DLOG_LEVEL_NONE=0) #none
add_definitions(-DLOG_LEVEL_DEBUG=1) #debug
add_definitions(-DLOG_LEVEL_INFO=2) #info
add_definitions(-DLOG_LEVEL_WARN=3) #warn
add_definitions(-DLOG_LEVEL_ERROR=4) #error
add_definitions(-DLOG_LEVEL_FATAL=5) #fatal

if (__ENABLE_MI_MALLOC__)
    message("使用mimalloc")
    add_definitions(-D __ENABLE_MI_MALLOC__)
endif ()

if (__ENABLE_SYSTEM_DEBUG)
    add_definitions(-D __ENABLE_SYSTEM_DEBUG)
endif ()

if(__ENABLE_SHARE_STACK__)
    message("协程使用共享栈")
    add_definitions(-D __ENABLE_SHARE_STACK__)
else ()
    message("协程使用独立栈")
endif ()

if (__SHARE_PTR_COUNTER__)
    add_definitions(-D __SHARE_PTR_COUNTER__)
endif ()

if(__ENABLE_LEVEL_DB__)
    message("启用leveldb数据库")
    option(LEVELDB_INSTALL "" OFF)
    option(LEVELDB_BUILD_TESTS "" OFF)
    option(LEVELDB_BUILD_BENCHMARKS "" OFF)
    add_definitions(-D __ENABLE_LEVEL_DB__)
endif ()

if (__ENABLE_OPEN_SSL__)
    message("使用openssl")
    add_definitions(-D __ENABLE_OPEN_SSL__)
endif ()

set(CMAKE_COMMON_DIR ${PROJECT_SOURCE_DIR})
message("================ [" ${CMAKE_BUILD_TYPE} "] ==============")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D __DEBUG__)
    add_definitions(-D __APP_HOTFIX__)
    add_definitions(-DSET_LOG_LEVEL=1) #debug

    set(CMAKE_C_FLAGS_RELEASE "-O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O0 -g")
else ()
    add_definitions(-DSET_LOG_LEVEL=2) #info
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif ()

if (ONLY_MAIN_THREAD)
    message("当前网络为单线程模型")
    add_definitions(-D ONLY_MAIN_THREAD)
    #add_definitions(-D ASIO_DISABLE_THREADS)
    add_definitions(-D ASIO_CONCURRENCY_HINT_UNSAFE)
else ()
    message("当前网络为多线程模型")
endif ()

if (WIN32)
    message("当前为win平台")
    add_definitions(-D __OS_WIN__)
    if (MSVC)
        add_definitions(-D __OS_WIN_MSVC_)
    endif ()
    add_definitions(-D NOMINMAX)
    remove_definitions(Yield ())
    remove_definitions(GetMessage)
    add_definitions(-D ASIO_HAS_IOCP) #win上使用iocp
    add_definitions(-D WIN32_LEAN_AND_MEAN)
elseif (APPLE)
    message("当前为mac平台强制禁用mimalloc")
    add_definitions(-D __OS_MAC__)
    remove_definitions(__ENABLE_MI_MALLOC__)
elseif (UNIX)
    message("当前为linux平台")
    add_definitions(-D __OS_LINUX__)
endif ()

if (MSVC)
    add_compile_options(/W4 /fp:fast /EHsc)
    add_link_options(/SAFESEH:NO)
else ()
    #- - wall:启用所有警告。
    #- -g:输出调试信息。
    #- - wno -sign-compare:禁用与符号比较相关的警告。
    #- - wno -class-memaccess:禁用类成员访问相关的警告。
    #- - wno -strict-aliasing:禁用严格混叠相关的警告。
    add_compile_options(-Wall -g -Wno-sign-compare -Wno-class-memaccess -Wno-strict-aliasing)
    if (APPLE)
        add_compile_options(-fno-pie)
    endif ()
endif ()

if (__OS_LINUX__)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        message("使用ccache作为编译缓存")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    else ()
        message("[warning]:请安装ccache来提高编译速度")
    endif ()
endif ()
include(CMake/proto.cmake)
