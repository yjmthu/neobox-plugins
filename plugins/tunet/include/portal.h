#ifndef PORTAL_H
#define PORTAL_H

#include <string>
#include <functional>
#include <mutex>
#include <neobox/httplib.h>

class Portal {
  std::u8string timestamp, token;
  using Mutex = std::mutex;
  // Mutex mtx;

  std::u8string const mainHost = u8"tsinghua.edu.cn";
  std::u8string const subHost = u8"auth4." + mainHost;
  std::u8string const loginHost = u8"login." + mainHost;

public:
  Portal()
    : client(HttpUrl(u8"login." + mainHost, u8"/"), true)
  { }

  ~Portal() {
    // std::lock_guard<Mutex> lock(mtx);
  }

  enum Error {
    NoError,
    AlreadyLogin,
    AlreadyLogout,
    NetworkError,
    UserInfoError,
    ParseError,
    TokenError,
    AuthError,
    HttpLibError,
  };

  typedef AsyncAction<Error> AsyncError;

private:
  struct UserInfo {
    std::u8string acID;
    std::u8string username;
    std::u8string password;
    std::u8string domain;
    std::u8string ip;
    bool isLogin = false;

    bool IsInfoValid() const {
      return !(username.empty() || password.empty() || acID.empty() || ip.empty());
    }
  } userInfo;

  typedef std::function<void(std::u8string)> Callback;

  static std::u8string GetTimestamp() {
    using namespace std::chrono;
    auto timeMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    auto count =  std::to_string(timeMs.count());
    return std::u8string(count.begin(), count.end());
  }
  
  HttpLib client;

public:
  AsyncError Init(std::u8string username, std::u8string password);
  AsyncError Login();
  AsyncError Logout();

  AsyncError GetInfo();
  HttpAwaiter SendAuth(std::u8string_view token);
  HttpAwaiter GetToken(std::u8string_view ip);
  std::optional<YJson> ParseJson(HttpResponse* res);
  std::u8string_view ParseToken(YJson& json);
  // std::u8string_view parseInfo(YJson& json);

  static std::u8string Base64(std::u8string data, const std::u8string& alpha);
  static std::u8string EncodeUserInfo(const class YJson& info, std::u8string_view token);
  static std::u8string Hmd5(std::u8string_view str, std::u8string_view key);
};


#endif  // PORTAL_H
