#ifndef NEOOCRPLG_H
#define NEOOCRPLG_H

#include <neobox/pluginobject.h>
#include <ocrconfig.h>

#include <QObject>

class QWidget;
class QVBoxLayout;

class NeoOcrPlg: public QObject, public PluginObject
{
  Q_OBJECT

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
private:
  OcrConfig m_Settings;
  class OcrDialog* m_OcrDialog;
  QAction* m_MainMenuAction;
  class NeoOcr* const m_Ocr;
signals:
  void RecognizeFinished();
};

#endif // NEOOCRPLG_H
