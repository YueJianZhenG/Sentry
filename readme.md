# Tendo 是一个轻量级，分布式rpc微服务框架
# 特点：
### 1. 跨平台
支持windows，Linux，Mac(支持m1和intel芯片)
### 2. 协程
使用汇编实现高效协程，同步方式写异步代码，避免使用回调  
使用协程封装服务器之间的rpc调用,与数据库的交互等,拒绝回调,纵享丝滑  
使用共享栈(一个协程大约2000字节)，只要内存足够，理论上开启数量无上限,通过共享栈,减少内存开辟,理想情况下开启新协程不需要开辟内存
### 3. 服务
支持rpc服务和http服务,高可用服务注册与发现  
使用c++或者lua实现服务，可以使用lua替换c++服务方法实现    
服务通过配置方式加载，自由组合，服务可使用http协议进行调试  
开发时候可以所有服务可以运行在一个进程，发布的时候可以部署在多个进程  
### 3. http
可作为web服务器,支持文件下载  
asio实现异步http客户端，在c++和lua层都可以    
asio实现了一套http1.1协议，实现了GET,POST方法，支持c++和lua处理，支持使用lua替换c++  
### 4. 数据库
使用asio实现了redis客户端协议,支持同步异步(协程)接口  
使用asio实现了mongo客户端协议，实现了增删查改基本操作,支持同步异步(协程)接口  
封装了mysql相关，通过指定一个protobuf结构创建sql表，新增字段也会自动创建,支持同步异步(协程)接口  
### 5. 协议
rpc通信基于自己实现的二进制协议  
通过反射可以在lua层热更协议，不需要生成c++代码，protobuf结构体可以直接转lua表  
### 6. 脚本
基于原生api实现的c++和lua交互引擎    
支持把c++的方法，类导给lua使用，在c++中等待lua协程完成  
### 7. 调试
所有接口支持使用ApiPost发送一个json的形式调试,或者直接使用框架自带客户端调试,客户端支持模拟多个用户操作  
# [编译和运行](./ReadMe/build.md)
# [配置文件说明](./ReadMe/config.md)