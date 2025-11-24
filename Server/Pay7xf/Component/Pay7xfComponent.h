//
// Created by yy on 2025/9/29.
//

#ifndef PAY7XFCOMPONENT_H
#define PAY7XFCOMPONENT_H

#include "Entity/Component/Component.h"
#include "Yyjson/Object/JsonObject.h"
#include "Http/Common/HttpResponse.h"

namespace pay_7xf
{
    constexpr int API_V1 = 1;
    constexpr int API_V2 = 2;

    const std::string host = "https://pay.7xf.cn";
    struct Config : public json::Object<Config>
    {
    public:
        int pid; //商户id
        std::string md5; //商户MD5密钥
        std::string public_key; //平台公钥
        std::string private_key; //私钥
        std::string notify_url; //通知url
        std::string return_url; //跳转通知地址
    };
    namespace type
    {
        const std::string wxpay = "wxpay"; //微信支付
        const std::string alipay = "alipay"; //支付宝
    }
    namespace method
    {
        const std::string web = "web"; //通用网页支付（会根据device判断，自动返回跳转url二维码/小程序跳转url等）
        const std::string jump = "jump"; //跳转支付（仅会返回跳转url）
        const std::string jsapi = "jsapi";
        const std::string app = "app";
        const std::string scan = "scan";
        const std::string applet = "applet";
    }
    namespace device
    {
        const std::string pc = "pc";
        const std::string qq = "qq";
        const std::string mobile = "mobile";
        const std::string wechat = "wechat";
        const std::string alipay = "alipay";
    }

    namespace sign
    {
        const std::string md5 = "MD5";
        const std::string rsa = "RSA";
    }

    struct CreateRequest
    {
    public:
        int pid = 0;
        std::string type = pay_7xf::type::wxpay;
        std::string out_trade_no;
        std::string notify_url;
        std::string return_url;
        std::string name;
        std::string money; //单位：元，最大2位小数
        std::string clientip;
        std::string param;
        std::string sign;

        std::string method = pay_7xf::method::web;
        std::string device = pay_7xf::device::mobile;

        std::string auth_code;
        std::string sub_openid;
        std::string sub_appid;
        std::string channel_id;
        std::string timestamp;
        std::string sign_type = pay_7xf::sign::rsa; //签名类型
    };

    class CreateResponse
    {
    public:
        int code = 0;
        std::string trade_no;
        std::string qrcode;
        std::string pay_type;
        std::string pay_info;
        std::string timestamp;
    };

    struct ProductInfo
    {
    public:
        int id = 0; //商品id
        std::string money; //金额
        std::string name; //名称
    };

    struct Notify
    {
    public:
        std::string pid;
        std::string trade_no;
        std::string out_trade_no;
        std::string api_trade_no;
        std::string type;
        std::string trade_status;
        std::string addtime;
        std::string endtime;
        std::string name;
        std::string money;
        std::string param;
        std::string buyer;
        std::string timestamp;
        std::string sign;
        std::string sign_type;
    };
}

namespace acs
{
    class Pay7xfComponent : public Component
    {
    public:
        Pay7xfComponent();
    private:
        bool Awake() final;
        bool LateAwake() final;
    public:
        const pay_7xf::ProductInfo * GetProduct(int productId);
        bool UpdateProduct(int id, int money, const std::string & name);
        std::unique_ptr<pay_7xf::CreateResponse> Create(pay_7xf::CreateRequest & request); //下单
        std::unique_ptr<pay_7xf::CreateResponse> Create(int productId, const std::string & ip); //下单
    public:
        inline void SetApiVersion(int version) { this->mApiVersion = version; }
        inline void SetSignType(const std::string & type) { this->mSignType = type; }
        bool SignParams(const std::map<std::string, std::string> & param, std::string & sign) const;
    private:
        bool InitPublicKey();
        bool InitPrivateKey();
        bool Do(const std::string & path, std::map<std::string, std::string> & param, std::string * result);
        bool Do(const std::string & path, std::map<std::string, std::string> & param, json::r::Document * result);
    private:
        int mApiVersion;
        std::string mSignType;
        pay_7xf::Config mConfig;
        class HttpComponent * mHttp;
        std::unordered_map<int, pay_7xf::ProductInfo> mProducts;
    };
}




#endif //PAY7XFCOMPONENT_H
