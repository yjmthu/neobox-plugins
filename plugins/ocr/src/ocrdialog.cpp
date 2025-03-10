#include <neobox/pluginmgr.h>

#include <neoocrplg.hpp>
#include <ocrdialog.h>

#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPainter>
#include <QCheckBox>
#include <QPen>

class OcrImageQFrame: public QWidget
{
public:
  explicit OcrImageQFrame(QWidget* parent);
  ~OcrImageQFrame();
  void SetImage(QString path);

protected:
  void paintEvent(QPaintEvent *event) override;

public:
  std::vector<OcrResult> m_Results;
  QImage m_Image;
};

OcrImageQFrame::OcrImageQFrame(QWidget* parent)
  : QWidget(parent)
{}

OcrImageQFrame::~OcrImageQFrame()
{
}

void OcrImageQFrame::paintEvent(QPaintEvent *event) {
  if (m_Image.isNull()) return;

  const float a = float(m_Image.width()) / width();
  const float b = float(m_Image.height()) / height();

  int x = 0;
  int y = 0;
  qreal scale = 1;

  if (a > b) {
    y = height() - m_Image.height();
    y /= 2;
    scale /= a;
  } else {
    x = width() - m_Image.width();
    x /= 2;
    scale /= b;
  }

  QPainter painter(this);
  QPen pen("red");
  pen.setWidth(1);
  painter.setPen(pen);
  painter.translate(x, y);
  painter.scale(scale, scale);

  painter.drawImage(0, 0, m_Image);
  for (auto const& item: m_Results) {
    painter.drawRect(item.x, item.y, item.w, item.h);
  }
}

void OcrImageQFrame::SetImage(QString path)
{
  if (path.isEmpty()) {
    mgr->ShowMsg("图片路径为空！");
    return;
  }
  if (!QFile::exists(path)) {
    mgr->ShowMsg("图片文件不存在！");
    return;
  }
  if (m_Image.load(path)) {
    m_Results.clear();
    update();
  } else {
    mgr->ShowMsg("加载图片文件失败！");
  }
}


OcrDialog::OcrDialog(NeoOcr& ocr, NeoOcrPlg& plg)
  : WidgetBase(nullptr)
  , m_Plugin(plg)
  , m_OcrEngine(ocr)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_TranslucentBackground);
  setFixedSize(800, 500);
  InitBaseLayout();
  AddCloseButton();
  AddMinButton();
}

OcrDialog::~OcrDialog()
{
  m_Plugin.m_OcrDialog = nullptr;
}

void OcrDialog::InitBaseLayout()
{
  auto const backWidget = new QWidget(this);
  backWidget->setGeometry(10, 10, width() - 20, height() - 20);
  backWidget->setStyleSheet(
    "QWidget {"
      "background-color: white;"
      "border-radius: 6px;"
    "}"
  );

  auto const title = AddTitle("图片文字扫描");
  // title->setParent(backWidget);
  title->move(20, 20);

  m_ImageLabel = new OcrImageQFrame(backWidget);
  m_ImageLabel->setGeometry(10, 30, 430, 435);
  m_ImageLabel->setStyleSheet("background-color: gray;");

  auto const panel = new QWidget(backWidget);
  panel->setGeometry(450, 30, 320, 435);
  panel->setStyleSheet("background-color: gray;");

  auto const controls = new QWidget(panel);
  controls->setGeometry(10, 10, 300, 100);
  controls->setStyleSheet(
    "QWidget {"
      "background-color:white;"
      "color:black;"
    "}"
    "QPushButton {"
      "background-color: #206620;"
      "padding: 4px 8px;"
      "border-radius: 6px;"
      "color: white;"
    "}"
    "QPushButton:hover {"
      "background-color: #101010;"
    "}"
  );

  auto const chkHideWhenFullScreen = new QCheckBox("截屏时隐藏", panel);
  auto const chkClearText = new QCheckBox("清空之前文本", panel);
  auto const chkAutoRecognize = new QCheckBox("自动识别", panel);
  chkAutoRecognize->setToolTip("勾选后，截取屏幕后将自动识别文字。");
  chkAutoRecognize->setChecked(true);

  auto const plainText = new QPlainTextEdit(panel);
  AddScrollBar(plainText->verticalScrollBar());

  controls->setContentsMargins(9, 9, 9, 9);
  auto gridLayout = new QGridLayout(controls);

  auto const btnScreenFech = new QPushButton("截取屏幕", controls);
  auto const btnOpenImg = new QPushButton("打开图片", controls);
  auto const btnScan = new QPushButton("识别文字", controls);

  gridLayout->addWidget(btnScreenFech, 0, 0);
  connect(btnScreenFech, &QPushButton::clicked, this, [=, this](){
    if (m_Plugin.m_IsBusy) {
      mgr->ShowMsg("正在识别中，请稍后再试。");
      return;
    }
    if (chkHideWhenFullScreen->isChecked()) this->hide();
    NeoOcrPlg::Guard guard(m_Plugin.m_IsBusy);
    if (auto image = m_Plugin.GrubImage(); !image.isNull()) {
      m_ImageLabel->m_Image = std::move(image);
      m_ImageLabel->m_Results.clear();
      m_ImageLabel->update();
    }
    if (!this->isVisible()) this->show();

    if (chkAutoRecognize->isChecked()) {
      m_Plugin.m_IsBusy = false;
      btnScan->click();
    }
  });

  gridLayout->addWidget(btnOpenImg, 0, 1);
  connect(btnOpenImg, &QPushButton::clicked, this, [this](){
    auto path = OpenImage();
    m_ImageLabel->SetImage(path);
  });

  gridLayout->addWidget(btnScan, 0, 2);
  connect(btnScan, &QPushButton::clicked, this, [=, this](){
    if (m_Plugin.m_IsBusy) {
      mgr->ShowMsg("正在识别中，请稍后再试。");
      return;
    }
    NeoOcrPlg::Guard guard(m_Plugin.m_IsBusy);

    if (m_ImageLabel->m_Image.isNull()) {
      mgr->ShowMsg("请先打开图片！");
      return;
    }
    if (chkClearText->isChecked()) plainText->clear();

    auto result = m_Plugin.GetTextEx(m_ImageLabel->m_Image);
    if (!result || result->empty()) {
      mgr->ShowMsg("识别失败");
      return;
    }

    for (const auto &i: *result) {
      plainText->appendPlainText(QString::fromUtf8(i.text.data(), i.text.size()));
    }
    m_ImageLabel->m_Results = std::move(*result);
    m_ImageLabel->update();
  });

  gridLayout->addWidget(chkHideWhenFullScreen, 1, 0);
  gridLayout->addWidget(chkClearText, 1, 1);
  gridLayout->addWidget(chkAutoRecognize, 1, 2);

  plainText->setGeometry(10, 125, 300, 300);
  plainText->setStyleSheet(
    "color:white;"
    "background-color:black;"
  );
}

QString OcrDialog::OpenImage()
{
  QString openFile = QFileDialog::getOpenFileName(
    this, "请选择一个图片文件",
    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
    "Image Files(*.jpg *.png *.bmp *.pgm *.pbm);;All(*.*)"
  );
  return openFile;
}
