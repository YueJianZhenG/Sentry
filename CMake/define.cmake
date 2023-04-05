if(MSVC)
    enable_language(C CXX ASM_MASM)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    enable_language(C CXX ASM)
endif()

#add_definitions(-w) #忽略警告
add_definitions(-D ASIO_STANDALONE)

option(protobuf_BUILD_TESTS OFF)
option(protobuf_BUILD_EXAMPLES OFF)
option(protobuf_BUILD_CONFORMANCE OFF)
#option(protobuf_BUILD_PROTOC_BINARIES OFF)

option(__DEBUG__ "debug模式" ON)

option(__DEBUG_STACK__ "开启堆栈打印" ON)
option(__ENABLE_CLIENT__ "开启客户端" ON)
option(__ENABLE_OPEN_SSL__ "开启SSL" OFF)
option(__HTTP_DEBUG_LOG__ "打印http日志" OFF)
option(__REDIS_DEBUG__ "开始redis调试" OFF)
option(ONLY_MAIN_THREAD "启用单线程模式" OFF)
option(__NET_ERROR_LOG__ "打印网络层错误" ON)
option(__ENABLE_MYSQL__ "是否使用mysql" ON)
option(__ENABLE_REDIS__ "是否使用redis" ON)

option(__ENABLE_MONGODB__ "是否使用mongodb" ON)
option(__ENABLE_MIMALLOC__ "启用mimalloc管理内存" OFF)

set(CMAKE_COMMON_DIR ${PROJECT_SOURCE_DIR})


if(__ENABLE_OPEN_SSL__)
    message("启用ssl")
    add_definitions(-D __ENABLE_OPEN_SSL__)
endif()


if(ONLY_MAIN_THREAD)
    message("当前网络为单线程模型")
    add_definitions(-D ONLY_MAIN_THREAD)
else()
    message("当前网络为多线程模型")
endif()

if(__ENABLE_MYSQL__)
    message("启用mysql数据库")
    add_definitions(-D __ENABLE_MYSQL__)
endif()

if(__ENABLE_REDIS__)
    message("启用redis数据库")
    add_definitions(-D __ENABLE_REDIS__)
endif()

if(__ENABLE_MONGODB__)
    message("启用mongodb数据库")
    add_definitions(-D __ENABLE_MONGODB__)
endif()

if(__DEBUG__)
    message("当前为debug模式")
    add_definitions(-D __DEBUG__)

    if(__REDIS_DEBUG__)
        message("打印redis命令到控制台")
        add_definitions(-D __REDIS_DEBUG__)
    endif()

    if(__ENABLE_CLIENT__)
        message("启用客户端")
        add_definitions(-D __ENABLE_CLIENT__)
    endif()
endif()

if(WIN32)
    add_definitions(-D NOMINMAX)
    add_definitions(-D WIN32_LEAN_AND_MEAN)
    remove_definitions(GetMessage)
endif()

if(MSVC)
    add_compile_options(/W4 /fp:fast /EHsc)
    add_link_options(/SAFESEH:NO)
else()
    add_compile_options(-Wall -g -Wno-sign-compare -Wno-class-memaccess -Wno-strict-aliasing)
    if(APPLE)
        add_compile_options(-fno-pie)
    endif()
endif()
