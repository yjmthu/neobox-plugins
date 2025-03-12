#ifndef SIDE_CHOOSER_H
#define SIDE_CHOOSER_H

#include <neobox/widgetbase.hpp>

class SideChooser : public WidgetBase {

  Q_OBJECT

private:
  friend class SpeedBox;

  enum ScreenSide: uint32_t {
    Left = 0b1000, Right = 0b0010,
    Top = 0b0001, Bottom = 0b0100,
    None = 0
  } m_HideSide = ScreenSide::None;
protected:
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;
public:
  SideChooser(SpeedBox* parent);
  ~SideChooser();
  void Exec();
private:
  void SetWinMode();
  void SetupUi(SpeedBox* speedbox);
signals:
  void Finished();
};

#endif // SIDE_CHOOSER_H