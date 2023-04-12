if(UNIX)
    if(NOT EXISTS ${libs_path}/mysql-connector-c-6.1.11-src)
        message("download mysql file " ${mysql_url})
        file(DOWNLOAD ${mysql_url} ${libs_path}/mysql.zip)
        if(NOT __OS_WIN__)
            execute_process(COMMAND unzip ${libs_path}/mysql.zip WORKING_DIRECTORY ${libs_path})
        endif()
        file(REMOVE ${libs_path}/mysql.zip)
    endif()
    LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/Libs/mysql/lib)
endif()


if(MSVC)
    if(EXISTS ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/libmysql.lib)
        if(EXISTS ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/mysqlclient.lib)
            message("MSVC启用mysql客户端")
            add_definitions(-D __ENABLE_MYSQL__)
            target_link_libraries(app ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/libmysql.lib)
            target_link_libraries(app ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/mysqlclient.lib)
        endif()
    endif()
else()
    if(EXISTS ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/libyassl.a)
        if(EXISTS ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/libmysqlclient.a)
            message("GCC启用mysql客户端")
            add_definitions(-D __ENABLE_MYSQL__)
            target_link_libraries(app ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/libyassl.a)
            target_link_libraries(app ${PROJECT_SOURCE_DIR}/Libs/mysql/lib/libmysqlclient.a)
        endif()
    endif()
endif()