#ifndef SCREENFETCH_HPP
#define SCREENFETCH_HPP

#include <QWidget>

class ScreenFetch : public QWidget {
  Q_OBJECT

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void leaveEvent(QEvent *event) override;

 public:
  explicit ScreenFetch(QImage& image, QWidget* parent = nullptr);
  ~ScreenFetch();

 private:
  void SetCursor();
  QImage& m_Image;
  uint8_t m_bFirstClicked = 0;
  QPoint m_LeftTop, m_RectSize;
  QPixmap m_PixMap;

signals:
  void CatchImage();
};

#endif // SCREENFETCH_HPP
