#include <sidechooser.hpp>
#include <speedbox.hpp>

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEventLoop>
#include <QCloseEvent>
#include <QButtonGroup>

SideChooser::SideChooser(SpeedBox* parent)
  : WidgetBase(parent, false, false)
{
  SetWinMode();
  SetupUi(parent);
  AddTitle("贴边选择");
  AddCloseButton();
}

SideChooser::~SideChooser()
{
}

void SideChooser::SetWinMode()
{
  setWindowTitle("贴边选择");
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
  setAttribute(Qt::WA_TranslucentBackground, true);
}

void SideChooser::SetupUi(SpeedBox* speedbox) {
  auto mainLayout = new QVBoxLayout(this);
  auto const center = new QWidget(this);
  center->setStyleSheet(R"(
QWidget {
  background-color: white;
  border-radius: 5px;
}
QPushButton {
  background-color: #A8BEC7;
  border: 1px solid #A8BEC7;
  border-radius: 5px;
  padding: 8px 16px;
}
QPushButton:hover {
  background-color: #88B2CE;
  border: 1px solid #4682B4;
}
QPushButton:pressed {
  background-color: #4682B4;
  padding: 7px 15px;
}
QPushButton:checked {
  background-color: #2B78B6;
  border: 1px solid #2B6783;
}
QPushButton:disabled {
  background-color: #CFECF9;
  color: #A0A0A0;
  border: 1px solid #B0E0E6;
}
  )");
  mainLayout->addWidget(center);
  SetShadowAround(center);

  const auto subLayout = new QVBoxLayout(center);
  subLayout->setContentsMargins(11, 30, 11, 11);
  auto const chkBtnLayout = new QHBoxLayout();
  subLayout->addLayout(chkBtnLayout);

  auto lst = { tr("上"), tr("右"), tr("下"), tr("左") };
  uint32_t set = speedbox->m_Settings.GetHideAside(), bit = 1;
  auto group = new QButtonGroup(this);
  group->setExclusive(false);
  for (auto i: lst) {
    auto const button = new QPushButton(i, this);
    button->setCheckable(true);
    button->setChecked(set & bit);
    chkBtnLayout->addWidget(button);
    group->addButton(button);
    bit <<= 1;
  }

  auto const buttonLayout = new QHBoxLayout();
  subLayout->addLayout(buttonLayout);

  auto const cancelButton = new QPushButton(tr("取消"), this);
  buttonLayout->addWidget(cancelButton);
  connect(cancelButton, &QPushButton::clicked, this, &QWidget::close);

  auto const applyButton = new QPushButton(tr("应用"), this);
  buttonLayout->addWidget(applyButton);

  connect(applyButton, &QPushButton::clicked, this, [this, group, speedbox, set] {
    uint32_t hideSide = 0, bit = 1;
    for (auto i: group->buttons()) {
      if (i->isChecked()) hideSide |= bit;
      bit <<= 1;
    }
    if (hideSide != set) {
      speedbox->m_Settings.SetHideAside(hideSide);
      mgr->ShowMsg("设置成功！");
    } else {
      mgr->ShowMsg("设置未改变！");
    }
    close();
  });
}

void SideChooser::showEvent(QShowEvent* event) {
  auto const speedbox = qobject_cast<SpeedBox*>(parent());
  if (!speedbox) return;
  auto const box = speedbox->geometry();
  const auto& screen = speedbox->m_ScreenGeometry;
  auto const size = frameSize();
  int x, y;
  y = (size.height() + box.bottom() > screen.bottom()) ?
      box.top() - size.height() : box.bottom();
  if (((size.width() + box.width()) / 2) + box.left() > screen.right()) {
    x = screen.right() - size.width();
  } else if (box.left() + box.right() < size.width() + screen.left() * 2) {
    x = screen.left();
  } else {
    x = box.right() - ((box.width() + size.width()) / 2);
  }
  move(x, y);
  event->accept();
}

void SideChooser::closeEvent(QCloseEvent* event) {
  emit Finished();
  event->accept();
}

void SideChooser::Exec()
{
  QEventLoop loop(parent());
  connect(this, &SideChooser::Finished, &loop, &QEventLoop::quit);
  show();
  // move(QCursor::pos() - QPoint(width() / 2, height() / 2));
  loop.exec();
}
