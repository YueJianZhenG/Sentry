#!/bin/bash

# shellcheck disable=SC2124
cmd="$@"
start_time=$(date)
current_path=$(pwd)
cd $current_path || exit

if [ ! -n "$1" ] ;then
  echo "***********************************************************"
  echo "*                                                         *"
  echo "*           debug        build server type=debug          *"
  echo "*           release      build server type=release        *"
  echo "*           lib          build protobuf and lua           *"
  echo "*           all          build all cmake project          *"
  echo "*           asan         use asan check memory            *"
  echo "*           openssl      download openssl and build       *"
  echo "*                                                         *"
  echo "***********************************************************"
else
    find ./ -name CMakeCache.txt -delete
    echo "start build server..."
fi
for arg in $cmd; do

     if [[ $arg == "asan" ]]; then
        cd "$current_path" || exit
        cmake -D__ENABLE_A_SCAN__=ON -DONLY_MAIN_THREAD=ON -DCMAKE_BUILD_TYPE=Debug ./CMakeLists.txt
        make protoc -j$(nproc)
        make libprotobuf -j$(nproc)
        make lua-share -j$(nproc)
        make lua-static -j$(nproc)
        make mimalloc-static -j$(nproc)
        make app -j$(nproc)
    fi

    if [[ $arg == "val" ]]; then
          cd "$current_path" || exit
          cmake -DENABLE_VALGRIND=ON -D__ENABLE_MI_MALLOC__=OFF -D__ENABLE_SHARE_STACK__=OFF -DBUILD_USE_CLANG=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-gdwarf-4" -DCMAKE_C_FLAGS="-gdwarf-4" ./CMakeLists.txt

          make protoc -j$(nproc)
          make libprotobuf -j$(nproc)
          make lua-static -j$(nproc)
          make mimalloc-static -j$(nproc)
          make app -j$(nproc)
    fi

    if [[ $arg == "openssl" ]]; then
        cd ./Libs/bin || exit
        if [ -d "./Libs/bin/openssl" ]; then
            echo "openssl already exists locally"
        else
            git clone https://gitee.com/yjz1995/openssl.git
        fi
        cd ./openssl || exit
            chmod -R 777 ./
            ./config
        make -j
        if [ -d "../../openssl/lib" ]; then
            mkdir -p "../../openssl/lib"
        fi
        cp -r ./*.a ../../openssl/lib
        cd "$current_path" || exit
        rm -rf ./Lib/bin/openssl
    fi

    if [[ $arg == "lib" ]]; then
        cd "$current_path" || exit
        make protoc
        make libprotobuf
        make lua-static
        make mimalloc-static
    fi

    if [[ $arg == "debug" ]]; then
        cd "$current_path" || exit
        cmake -DCMAKE_BUILD_TYPE=Debug ./CMakeLists.txt
        make app
    fi

    if [[ $arg == "release" ]]; then
          cd "$current_path" || exit
          cmake -DCMAKE_BUILD_TYPE=Release ./CMakeLists.txt
          make protoc -j$(nproc)
          make libprotobuf -j$(nproc)
          make lua-static -j$(nproc)
          make mimalloc-static -j$(nproc)
          make app -j$(nproc)
    fi
done

# shellcheck disable=SC2038
find ./ -name CMakeFiles | xargs rm -r
find ./ -name cmake_install.cmake -delete
find ./ -name CMakeCache.txt -delete

end_time=$(date)
# shellcheck disable=SC2046
elapsed_time=$(expr $(date -d "$end_time" +%s) - $(date -d "$start_time" +%s))

# Print the elapsed time in seconds
echo "Elapsed time: $elapsed_time seconds"
