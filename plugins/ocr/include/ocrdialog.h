#ifndef OCRDIALOG_H
#define OCRDIALOG_H

#include <neobox/widgetbase.hpp>
#include <neoocr.hpp>

class OcrDialog: public WidgetBase {
public:
  explicit OcrDialog(NeoOcr& ocr, class NeoOcrPlg& plg);
  virtual ~OcrDialog();
private:
  void InitBaseLayout();
  QString OpenImage();
private:
  NeoOcr& m_OcrEngine;
  NeoOcrPlg& m_Plugin;
  // OcrDialog* & m_Self;
  class OcrImageQFrame* m_ImageLabel;
};

#endif // OCRDIALOG_H