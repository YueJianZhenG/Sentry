<template>
    <div>
        <el-row :gutter="20">
            <el-col :span="7">
                <el-card shadow="hover" class="mgb20" style="height: 40%">
                    <el-descriptions title="用户信息" border column="1">
                        <el-descriptions-item label="昵称">{{ user.name }}</el-descriptions-item>
                        <el-descriptions-item label="权限">{{ format_permiss(null, null, user.permission) }}
                        </el-descriptions-item>
                        <el-descriptions-item label="登录IP">{{ user.login_ip }}</el-descriptions-item>
                        <el-descriptions-item label="注册时间">{{ timeToStrTime(user.create_time * 1000) }}
                        </el-descriptions-item>
                        <el-descriptions-item label="登录时间">{{ timeToStrTime(user.login_time * 1000) }}
                        </el-descriptions-item>
                    </el-descriptions>
                </el-card>
                <el-card shadow="hover" class="mgb20" style="height: 55%">
                    <el-form inline="inline">
                        <el-form-item>
                            <h3>服务器信息</h3>
                        </el-form-item>
                        <el-form-item>
                            <el-button icon="refresh" @click="refresh_sever_info"></el-button>
                        </el-form-item>
                        <el-form-item>
                            <el-button icon="search" @click="show_view=true"></el-button>
                        </el-form-item>
                    </el-form>
                    <el-descriptions border column="1">
                        <el-descriptions-item label="名字">{{ run_info.name }}</el-descriptions-item>
                        <el-descriptions-item label="CPU使用率">{{
                                run_info.cpu
                            }}
                        </el-descriptions-item>

                        <el-descriptions-item label="使用内存">
                            {{ run_info.use_memory }}
                        </el-descriptions-item>
                        <el-descriptions-item label="物理内存">
                            {{ run_info.max_memory }}
                        </el-descriptions-item>
                        <el-descriptions-item label="Lua内存">{{ run_info.lua.memory }}</el-descriptions-item>

                        <el-descriptions-item label="增长内存">{{ run_info.cost_memory }}</el-descriptions-item>
                        <el-descriptions-item label="http总处理">{{ run_info.web.sum }}</el-descriptions-item>
                        <el-descriptions-item label="启动时间">{{
                                run_info.start_time
                            }}
                        </el-descriptions-item>
                    </el-descriptions>
                </el-card>
            </el-col>
            <el-col :span="17">
                <el-card shadow="hover" style="height: 100%">
                    <h4>操作记录</h4>
                    <el-table :data="tab.list" v-loading="loading" border style="margin-top: 20px">
                        <el-table-column min-width="100" label="用户" prop="name"></el-table-column>
                        <el-table-column min-width="100" label="IP" show-overflow-tooltip prop="ip"></el-table-column>
                        <el-table-column min-width="100" label="方法" prop="method"></el-table-column>
                        <el-table-column min-width="150" label="路径" prop="url" show-overflow-tooltip></el-table-column>
                        <el-table-column min-width="100" label="描述" prop="desc" show-overflow-tooltip></el-table-column>
                        <el-table-column min-width="150" label="时间" prop="time" show-overflow-tooltip
                                         :formatter="format_time"></el-table-column>
                        <el-table-column label="查看" fixed="right" min-width="200" align="center" v-if="user.permission === 100">
                            <template #default="scope">
                                <el-button type="primary" @click="look_http(scope.row)">查看</el-button>
                                <el-button type="danger">删除</el-button>
                            </template>
                        </el-table-column>
                        <!--                        <el-table-column label="请求" prop="request" show-overflow-tooltip></el-table-column>-->
                        <!--                        <el-table-column label="返回" prop="response" show-overflow-tooltip></el-table-column>-->
                    </el-table>

                    <div class="pagination">
                        <el-pagination
                            background
                            layout="total, prev, pager, next"
                            :current-page="tab.page.index"
                            :page-size="tab.page.count"
                            :total="tab.page.total"
                            @current-change="handlePageChange"
                        ></el-pagination>
                    </div>
                </el-card>
            </el-col>
        </el-row>
        <el-dialog v-model="view.show">
            <json-viewer style="max-width: 100%" :data="view.data" :key="view.index"></json-viewer>
        </el-dialog>

        <el-dialog v-model="show_view" title="服务器运行信息">
            <json-viewer style="max-width: 100%" :data="run_info" :key="index"></json-viewer>
        </el-dialog>
    </div>
</template>

<script>

import {format_permiss, format_time} from "../api/format";
import {RequestOptRecord} from "../api/login";
import Avatar from '../views/Avatar.vue';
import axios from "axios";
import service from "../utils/request";
import {httpRequest} from "../utils/httpRequest";
import JsonViewer from "./json_view.vue";
import {ElMessage} from "element-plus";


export default {
    name: "welcome",
    components: {
        JsonViewer,
        Avatar
    },
    data() {
        return {
            tab: {
                list: [],
                page: {
                    index: 1,
                    count: 10,
                    total: 0
                },
            },
            index: 1,
            loading : false,
            show_view: false,
            run_info: {
                name: "",
                fps: "",
                time: 0,
                cpu: 0,
                use_memory: 0,
                max_memory: 0,
                cost_memory : "",
                web: {
                    sum: 0
                },
                mongo: {
                    sum: 0
                },
                lua : {
                    memory : ""
                }
            },
            user: {
                name: "",
                login_ip: "",
                city_name: "",
                permission: 0,
                login_time: 0,
                create_time: 0,
            },
            view: {
                index: 0,
                show: false,
                data : {

                }
            },
            format_time: format_time,
            format_permiss: format_permiss
        }
    },
    methods: {
        service,
        timeToStrTime(timestamp) {
            const date = new Date(timestamp);
            return date.toLocaleString('zh-CN', {timeZone: 'Asia/Shanghai', hour12: false});
        },
        async look_http(data) {
            this.view.index++
            const response = await httpRequest.GET(`/record/find?id=${data._id}`)
            if(response.data.code !== 0) {
                ElMessage.error(response.data.error)
                return
            }
            const result = response.data.data
            this.view.data.request = this.parse_data(result.request)
            this.view.data.response = this.parse_data(result.response)
            this.view.show = true
        },

        parse_data(data) {
            try {
                return JSON.parse(data)
            } catch (e) {
                const params = new URLSearchParams(data);
                return Object.fromEntries(params.entries());
            }
        },

        async handlePageChange(page) {
            this.loading = true
            try {
                this.tab.list = []
                this.tab.page.index = page
                await this.query_operator_record();
            }
            catch (e) {
                //ElMessage.error("拉取操作记录失败")
            }

            this.loading = false
        },
        async query_operator_record() {
            const response = await RequestOptRecord(this.tab.page.index)
            const result = response.data
            if (result.code === 0) {
                this.tab.list = result.data.list
                this.tab.page.total = result.data.count
            }
        },
        async refresh_sever_info() {
            const response = await httpRequest.GET("/admin/info")
            if (response.data.code === 0) {
                this.index++
                this.run_info = response.data.data
            }
        }
    },
    async mounted() {
        const user_json = localStorage.getItem("user_info")
        if (user_json && user_json.length > 0) {
            this.user = JSON.parse(user_json)
            try {
                const response = await axios.get(`https://ip-api.com/json/${this.user.login_ip}?lang=zh-CN`)
                this.user.city_name = response.data.city
            } catch (e) {
                this.user.city_name = "未知"
            }
        }
        await this.refresh_sever_info();
        await this.handlePageChange(1)
    }
}
</script>

<style>
.preserve-space {
    white-space: pre;
}
.json-viewer {
    max-width: 100%; /* 让组件宽度不超过父级 */
    overflow-x: auto; /* 允许横向滚动，防止内容撑破 */
}
</style>

