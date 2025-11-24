#!/bin/bash

# 创建存储数据的目录
mkdir -p ./data/mysql_data
mkdir -p ./data/redis_data
mkdir -p ./data/mongo_data
mkdir -p ./data/pgsql_data

# 定义容器的名称
mysql_container="mysql-container"
redis_container="redis-container"
mongo_container="mongo-container"
pgsql_container="pgsql-container"

# 从环境变量或者参数获取密码，提升安全性
MYSQL_ROOT_PASSWORD=${MYSQL_ROOT_PASSWORD:-199595yjz.}
MYSQL_PASSWORD=${MYSQL_PASSWORD:-199595yjz.}
REDIS_PASSWORD=${REDIS_PASSWORD:-199595yjz.}
MONGO_PASSWORD=${MONGO_PASSWORD:-199595yjz.}
PGSQL_PASSWORD=${PGSQL_PASSWORD:-199595yjz.}

# 定义日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# 定义启动容器的函数
start_container() {
    local container_name=$1
    local container_exists=$(docker ps -q -f name=$container_name)

    if [ ! -z "$container_exists" ]; then
        log "Stopping and removing existing $container_name container..."
        docker stop $container_name
        docker rm $container_name
    fi
}

# 启动MySQL容器
start_container $mysql_container
log "Starting MySQL container..."
docker run -d --name $mysql_container -p 3306:3306 \
    --restart=always \
    -e MYSQL_ROOT_PASSWORD=$MYSQL_ROOT_PASSWORD \
    -e MYSQL_DATABASE=mydatabase \
    -e MYSQL_USER=yjz \
    -e MYSQL_PASSWORD=$MYSQL_PASSWORD \
    -v $(pwd)/data/mysql_data:/var/lib/mysql \
    mysql:5.7 || { log "Failed to start MySQL container"; exit 1; }

# 启动Redis容器（正确设置密码的方式）
start_container $redis_container
log "Starting Redis container..."
docker run -d --name $redis_container -p 6379:6379 \
    --restart=always \
    -v $(pwd)/data/redis_data:/data \
    redis redis-server --requirepass "$REDIS_PASSWORD" || { log "Failed to start Redis container"; exit 1; }

# 启动MongoDB容器
start_container $mongo_container
log "Starting MongoDB container..."
docker run -d --name $mongo_container -p 27017:27017 \
    --restart=always \
    -e MONGO_INITDB_ROOT_USERNAME=yjz \
    -e MONGO_INITDB_ROOT_PASSWORD=$MONGO_PASSWORD \
    -v $(pwd)/data/mongo_data:/data/db \
    mongo || { log "Failed to start MongoDB container"; exit 1; }

# 启动PostgreSQL容器（补充缺失的部分）
start_container $pgsql_container
log "Starting PostgreSQL container..."
docker run -d --name $pgsql_container -p 5432:5432 \
    --restart=always \
    -e POSTGRES_USER=yjz \
    -e POSTGRES_PASSWORD=$PGSQL_PASSWORD \
    -e POSTGRES_DB=mydatabase \
    -v $(pwd)/data/pgsql_data:/var/lib/postgresql/data \
    postgres:13 || { log "Failed to start PostgreSQL container"; exit 1; }

log "All database containers started successfully!"