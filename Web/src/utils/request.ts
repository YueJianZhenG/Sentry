import axios, {AxiosInstance, AxiosError, AxiosResponse, AxiosRequestConfig} from 'axios';
import {app} from "../api/token";

const service: AxiosInstance = axios.create({
    baseURL: "http://127.0.0.1:8080",
    //baseURL : "http://43.143.239.75:8080",
    timeout: 5000
});

service.interceptors.request.use(
    (config: AxiosRequestConfig) => {
        if(config.url?.indexOf("/city/list") === -1) {
            const userInfo = app.get_user_info()
            if (userInfo != null) {
                config.headers = {
                    "Authorization": userInfo.token
                }
            }
        }
        return config;
    },
    (error: AxiosError) => {
        console.log(error);
        return Promise.reject();
    }
);

service.interceptors.response.use(
    (response: AxiosResponse) => {
        if (response.status === 200) {
            return response;
        } else {
            Promise.reject();
        }
    },
    (error: AxiosError) => {
        return Promise.reject();
    }
);

// @ts-ignore
export default service;
