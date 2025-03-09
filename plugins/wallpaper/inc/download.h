#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <wallbase.h>

class DownloadJob {
  typedef std::optional<std::function<void()>> Callback;
public:
  enum class Error {
    NoError,
    NetworkError,
    FileError,
    RepeatError,
    HttpLibError,
    ImageInfoError,
  };
  [[nodiscard]]
  static AsyncAction<Error> DownloadImage(const ImageInfo& imageInfo);
  static bool IsImageFile(const std::u8string & fileName);

  static std::mutex m_Mutex;
  static void ClearPool();
  static bool IsPoolEmpty();
};

#endif // DOWNLOAD_H