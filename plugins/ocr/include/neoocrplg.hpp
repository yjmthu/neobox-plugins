#ifndef NEOOCRPLG_H
#define NEOOCRPLG_H

#include <neobox/pluginobject.h>
#include <ocrconfig.h>
#include <neoocr.hpp>

#include <QObject>

class QWidget;
class QVBoxLayout;

class NeoOcrPlg: public QObject, public PluginObject
{
  Q_OBJECT

  struct Guard {
    Guard(bool &b) : busy(b) { busy = true; }
    ~Guard() { busy = false; }
    bool &busy;
  };

protected:
  class QAction* InitMenuAction() override;
  void InitFunctionMap() override;
public:
  explicit NeoOcrPlg(YJson& settings);
  virtual ~NeoOcrPlg();
private:
  YJson& InitSettings(YJson& settings);
  void ChooseLanguages();
  void InitMenu();
  void AddEngineMenu();
#ifdef _WIN32
  void AddWindowsSection(QWidget* parent, QVBoxLayout* layout);
#endif
  void AddTesseractSection(QWidget* parent, QVBoxLayout* layout);
  std::optional<std::u8string> GetText(const QImage& image);
  std::optional<std::vector<OcrResult>> GetTextEx(const QImage& image);
  static QImage GrubImage();
private:
  friend class OcrDialog;
  OcrConfig m_Settings;
  QAction* m_MainMenuAction;
  class NeoOcr* const m_Ocr;
  OcrDialog* m_OcrDialog;
  bool m_IsBusy = false;
signals:
  void RecognizeFinished();
};

#endif // NEOOCRPLG_H
