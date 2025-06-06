#pragma once

#include <colorback.hpp>
#include <QPixmap>

class ScreenFetch: public ColorBack {
  Q_OBJECT

public:
  explicit ScreenFetch();
  virtual ~ScreenFetch();

protected:
  void leaveEvent(QEvent* event) override;

signals:
  void ScreenChanged();
};
