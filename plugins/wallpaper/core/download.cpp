#include <neobox/systemapi.h>
#include <download.h>
#include <neobox/pluginmgr.h>
#include <neobox/httplib.h>

using namespace std::literals;

fs::path FileNameFilter(std::u8string path) {
  std::u8string_view pattern(u8":*?\"<>|");
  std::u8string result;
  result.reserve(path.size());
  for (int count=0; auto c : path) {
    if (pattern.find(c) == pattern.npos) {
      result.push_back(c);
    } else if (c == u8':') {
      // 当路径为相对路径时，可能会有Bug
      if (++count == 1) {
        result.push_back(c);
      }
    }
  }
  path = std::move(result);
  fs::path ret = path;
  return ret.make_preferred();
}

static std::map<std::filesystem::path, HttpLib*> m_Pool;
std::mutex DownloadJob::m_Mutex;

bool DownloadJob::IsPoolEmpty() {
  std::lock_guard<std::mutex> locker(m_Mutex);
  return m_Pool.empty();
}

static void PoolInsert(std::filesystem::path path, HttpLib* job) {
  std::lock_guard<std::mutex> locker(DownloadJob::m_Mutex);
  m_Pool[path] = job;
}

static void PoolErase(std::filesystem::path path) {
  std::lock_guard<std::mutex> locker(DownloadJob::m_Mutex);
  m_Pool.erase(path);
}

static bool IsPoolExist(std::filesystem::path path) {
  std::lock_guard<std::mutex> locker(DownloadJob::m_Mutex);
  return m_Pool.find(path) != m_Pool.end();
}

void DownloadJob::ClearPool()
{
  m_Mutex.lock();
  for (auto& [url, worker]: m_Pool) {
    worker->ExitAsync();
  }
  m_Mutex.unlock();

  while (!DownloadJob::IsPoolEmpty());
}

static Wall::Bool StartJob(fs::path path, std::u8string url) {
  HttpLib job = HttpLib(HttpUrl(url), true);
  job.SetRedirect(3);

  PoolInsert(path, &job);

  auto file = std::ofstream
    (path, std::ios::out | std::ios::binary);
  HttpLib::Callback callback = {
    .onWrite = [&file](auto data, auto size) {
      file.write(reinterpret_cast<const char*>(data), size);
    },
  };
  auto res = co_await job.GetAsync(std::move(callback));
  file.close();

  PoolErase(path);

  if (file.fail() || res->status != 200) {
    fs::remove(path);
    co_return false;
  }

  co_return true;
}

AsyncAction<DownloadJob::Error> DownloadJob::DownloadImage(const ImageInfo& imageInfo)
{
  using Error = DownloadJob::Error;
  if (imageInfo.ErrorCode != ImageInfo::NoErr) {
    mgr->ShowMsgbox(L"出错", Utf82WideString(imageInfo.ErrorMsg));
    co_return Error::ImageInfoError;
  }

  // Check image dir and file.
  const auto filePath = FileNameFilter(imageInfo.ImagePath);
  const auto dir = filePath.parent_path();
  const auto imageUri = imageInfo.ImageUrl;

  std::error_code error;
  if (!fs::exists(dir) && !fs::create_directories(dir, error)) {
    mgr->ShowMsgbox(L"出错", std::format(L"创建文件夹失败！\n{}", error.value()));
  }
  if (fs::exists(filePath)) {
    if (!fs::file_size(filePath)){
      fs::remove(filePath);
    } else {
      co_return Error::NoError;
    }
  }
  if (imageUri.empty()) {
    co_return Error::ImageInfoError;
  }

  if (!HttpLib::IsOnline()) {
    mgr->ShowMsgbox(L"出错", L"网络异常");
    co_return Error::NetworkError;
  }

  if (IsPoolExist(filePath)) {
    co_return Error::ImageInfoError;
  }

  auto const res = co_await StartJob(filePath, imageUri).awaiter();

  if (!res || !*res) {
    mgr->ShowMsgbox(L"出错", L"下载失败");
    co_return Error::NetworkError;
  }

  co_return Error::NoError;
}

bool DownloadJob::IsImageFile(const std::u8string& filesName) {
  // BMP, PNG, GIF, JPG
  auto wideString { Utf82WideString(filesName) };
  return std::regex_match(wideString, std::wregex(m_ImgNamePattern, std::wregex::icase));
}

const DownloadJob::String DownloadJob::m_ImgNamePattern {
  L".*\\.(jpg|bmp|gif|jpeg|png)$"
};

