#include <screenfetch.hpp>

#include <QGuiApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>

ScreenFetch::ScreenFetch()
  : ColorBack(QGuiApplication::screenAt(QCursor::pos())->grabWindow())
{
  // setWindowFlag(Qt::WindowStaysOnTopHint, true);
  setWindowFlag(Qt::Window, true);
  setMouseTracking(true);
}

ScreenFetch::~ScreenFetch() {
  setMouseTracking(false);
}

void ScreenFetch::leaveEvent(QEvent* event) {
  auto screen = QGuiApplication::screenAt(QCursor::pos());
  m_Pixmap = screen->grabWindow();
  m_Image = m_Pixmap.toImage();

  setGeometry(screen->geometry());

  emit InitShow();
}
