#ifndef NEOOCR_H
#define NEOOCR_H

#include <neobox/httplib.h>
#include <string>
#include <filesystem>
#include <queue>

namespace tesseract {
  class TessBaseAPI;
}

class QImage;

struct OcrResult {
  std::u8string text;
  int x, y, w, h;
  int confidence;
};

class NeoOcr {
public:
  typedef AsyncU8String String;
  typedef AsyncAction<std::vector<OcrResult>> OcrResultAction;
  enum class Engine { Windows, Tesseract, Paddle, Other };
  NeoOcr(class OcrConfig& settings);
  ~NeoOcr();
  [[nodiscard]] String GetText(QImage image);
  [[nodiscard]] OcrResultAction GetTextEx(const QImage& image);
  void InitLanguagesList();
  void AddLanguages(const std::vector<std::u8string>& urls);
  void RmoveLanguages(const std::vector<std::u8string>& names);
  void SetDataDir(const std::u8string& dirname);
  void SetDropData(std::queue<std::u8string_view>& data);
#ifdef _WIN32
  static std::vector<std::pair<std::wstring, std::wstring>> GetLanguages();
#endif
private:
  [[nodiscard]] String OcrWindows(const QImage& image);
  std::u8string OcrTesseract(const QImage& image);
  std::vector<OcrResult> OcrTesseractEx(const QImage& image);
private:
  OcrConfig& m_Settings;
  std::u8string GetLanguageName(const std::u8string& url);
  static void DownloadFile(std::u8string_view url,
      const std::filesystem::path& path);
  tesseract::TessBaseAPI* const m_TessApi;
  std::u8string m_Languages;
  std::string m_TrainedDataDir;
};

#endif
