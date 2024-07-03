if(MSVC)
    enable_language(C CXX ASM_MASM)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    enable_language(C CXX ASM)
endif()

add_definitions(-w) #忽略警告
add_definitions(-D ASIO_STANDALONE)

if(MSVC)
    add_definitions(-D _WIN32_WINNT=0x0601)
    add_compile_options(/W4 /wd4100 /wd4127 /wd4819)
endif()

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

if(WIN32 AND NOT MSVC)
    #set(protobuf_BUILD_SHARED_LIBS ON) #编译动态库
endif()

option(__ENABLE_SPD_LOG__ "使用spdlog" OFF)

option(__DEBUG_STACK__ "开启堆栈打印" ON)
option(ONLY_MAIN_THREAD "启用单线程模式" OFF)
option(__NET_ERROR_LOG__ "打印网络层错误" ON)
option(__COR_SHARED_STACK__ "使用共享栈" ON)

option(__CONSOLE_LOG__ "控制台打印日志" ON)
option(__ENABLE_SYSTEM_DEBUG "打印系统日志" ON)
option(__ENABLE_OPEN_SSL__ "开启openssl" ON)

if(APPLE)
    option(__ENABLE_DING_DING_PUSH "开启钉钉通知" OFF)
elseif(UNIX)
    option(__ENABLE_DING_DING_PUSH "开启钉钉通知" ON)
else()
    option(__ENABLE_DING_DING_PUSH "开启钉钉通知" OFF)
endif()

add_definitions(-DLOG_LEVEL_DEBUG=1) #debug
add_definitions(-DLOG_LEVEL_INFO=2) #info
add_definitions(-DLOG_LEVEL_WARN=3) #warn
add_definitions(-DLOG_LEVEL_ERROR=4) #error
add_definitions(-DLOG_LEVEL_FATAL=5) #fatal
add_definitions(-DLOG_LEVEL_OFF=6) #关闭

if(__ENABLE_SYSTEM_DEBUG)
    add_definitions(-D __ENABLE_SYSTEM_DEBUG)
endif()

if(__CONSOLE_LOG__)
    add_definitions(-D __CONSOLE_LOG__)
endif()

if(__ENABLE_DING_DING_PUSH)
    message("开启钉钉通知")
    add_definitions(-D __ENABLE_DING_DING_PUSH)
else()
    message("关闭钉钉通知")
endif()

if(__ENABLE_OPEN_SSL__)
    message("使用openssl")
    add_definitions(-D __ENABLE_OPEN_SSL__)
endif()


set(CMAKE_COMMON_DIR ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
message("================ [" ${CMAKE_BUILD_TYPE} "] ==============")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D __DEBUG__)
    add_definitions(-D __APP_HOTFIX__)
    add_definitions(-DSET_LOG_LEVEL=1)
else()
    add_definitions(-DSET_LOG_LEVEL=2)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
endif()

if(__ENABLE_SPD_LOG__)
    message("使用spdlog")
    add_definitions(-D __ENABLE_SPD_LOG__)
endif()

if(__COR_SHARED_STACK__)
    message("协程使用共享栈")
    add_definitions(-D __COR_SHARED_STACK__)
endif()

if(ONLY_MAIN_THREAD)
    message("当前网络为单线程模型")
    add_definitions(-D ONLY_MAIN_THREAD)
else()
    message("当前网络为多线程模型")
endif()

if(WIN32)
    message("当前为win平台")
    add_definitions(-D __OS_WIN__)
    if(MSVC)
        add_definitions(-D __OS_WIN_MSVC_)
    endif()
    add_definitions(-D NOMINMAX)
    remove_definitions(Yield())
    remove_definitions(GetMessage)
    add_definitions(-D WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    message("当前为mac平台")
    add_definitions(-D __OS_MAC__)
elseif(UNIX)
    message("当前为linux平台")
    add_definitions(-D __OS_LINUX__)
endif()

if(MSVC)
    add_compile_options(/W4 /fp:fast /EHsc)
    add_link_options(/SAFESEH:NO)
else()
#- - wall:启用所有警告。
#- -g:输出调试信息。
#- - wno -sign-compare:禁用与符号比较相关的警告。
#- - wno -class-memaccess:禁用类成员访问相关的警告。
#- - wno -strict-aliasing:禁用严格混叠相关的警告。
    add_compile_options(-Wall -g -Wno-sign-compare -Wno-class-memaccess -Wno-strict-aliasing)
    if(APPLE)
        add_compile_options(-fno-pie)
    endif()
endif()

if(__OS_LINUX__)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        message("使用ccache作为编译缓存")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    else()
        message("[warning]:请安装ccache来提高编译速度")
    endif()
endif()
include(CMake/proto.cmake)
