#include <wallhavenex.h>
#include <bingapiex.h>
#include <directapiex.h>
#include <nativeex.h>
#include <favoriteex.h>
#include <scriptex.h>
#include <neowallpaperplg.h>
#include <wallpaper.h>
#include <yjson.h>
#include <menubase.hpp>
#include <pluginmgr.h>
#include <neomenu.hpp>

#include <QString>
#include <QInputDialog>
#include <QDesktopServices>
#include <QFileDialog>
#include <QActionGroup>
#include <QDropEvent>
#include <QMimeData>

#ifdef _WIN32
#include <windows.h>
#else
#endif

#include <ranges>

#define PluginName NeoWallpaperPlg
#include <pluginexport.cpp>

PluginName::PluginName(YJson& settings):
  PluginObject(InitSettings(settings), u8"neowallpaperplg", u8"壁纸引擎"),
  m_Wallpaper(new Wallpaper(m_Settings))
{
  InitFunctionMap();
}

PluginName::~PluginName()
{
  delete m_MainMenuAction;
  // delete m_MainMenu;
  delete m_Wallpaper;
}

void PluginName::InitFunctionMap()
{
  m_PluginMethod = {
    {u8"setTimeInterval",
      {u8"时间间隔", u8"设置更换壁纸的时间间隔", [this](PluginEvent, void*){
        int const iOldTime = m_Wallpaper->m_Settings.GetTimeInterval();
        auto iNewTime = mgr->m_Menu->GetNewInt("输入时间间隔", "时间间隔（分钟）", 5, 65535, iOldTime);
        if (!iNewTime) {
          mgr->ShowMsg("设置时间间隔失败！");
        } else {
          m_Wallpaper->SetTimeInterval(*iNewTime);
          mgr->ShowMsg("设置时间间隔成功！");
        }
      }, PluginEvent::Void}
    },
    {u8"cleanRubish",
      { u8"清理垃圾", u8"删除垃圾箱内壁纸", [this](PluginEvent, void*){
        m_Wallpaper->ClearJunk();
        mgr->ShowMsg("清除壁纸垃圾成功！");
      }, PluginEvent::Void
    }},
    {u8"setAutoChange",
      {u8"自动更换", u8"设置是否按照时间间隔自动切换壁纸", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Wallpaper->SetAutoChange(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg("设置成功！");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Wallpaper->m_Settings.GetAutoChange();
        }
      },
      PluginEvent::Bool }
    },
    {u8"setFirstChange",
      {u8"首次更换", u8"设置是否在插件启动时更换一次壁纸", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Wallpaper->SetFirstChange(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg("设置成功！");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Wallpaper->m_Settings.GetFirstChange();
        }
      }, PluginEvent::Bool },
    }
  };

  m_Following.push_back({u8"neospeedboxplg", [this](PluginEvent event, void* data){
    if (event == PluginEvent::Drop) {
      const auto mimeData = reinterpret_cast<QDropEvent*>(data)->mimeData();
      if (!mimeData->hasUrls()) return;
      std::list<QByteArray> dataRaw;
      std::queue<std::u8string_view> dataRef;
      for (const auto& i: mimeData->urls()) {
        if (!i.isValid()) continue;
        if (i.isLocalFile()) {
          dataRaw.push_back(i.toLocalFile().toUtf8());
        } else if (i.scheme().startsWith("http")) {
          dataRaw.push_back(i.toString().toUtf8());
        } else {
          continue;
        }
        dataRef.push(std::u8string_view(
          reinterpret_cast<const char8_t*>(dataRaw.back().constData()),
          dataRaw.back().size()
        ));
      }
      m_Wallpaper->SetDropFile(dataRef);
    }
  }});
}

QAction* PluginName::InitMenuAction()
{
  m_MoreSettingsAction = new QAction("更多设置", m_MainMenu);
  LoadWallpaperTypeMenu(m_MainMenu);
  this->PluginObject::InitMenuAction();
  LoadDropMenu(m_MainMenu->addAction("拖拽设置"));
  m_MainMenu->addAction(m_MoreSettingsAction);
  LoadWallpaperExMenu(m_MainMenu);
  LoadMainMenuAction();

  return m_MainMenuAction;
}

void PluginName::LoadMainMenuAction()
{
  m_MainMenuAction = new QAction("壁纸切换");
  auto const menu = new MenuBase(m_MainMenu);
  m_MainMenuAction->setMenu(menu);

  std::vector<std::pair<std::u8string, FunctionInfo>> temp = {
    { u8"prev",
      {u8"上一张图", u8"切换到上一张壁纸", [this](PluginEvent, void*) {
          m_Wallpaper->SetSlot(OperatorType::UNext);
        }, PluginEvent::Void }
    },
    {u8"next",
      {u8"下一张图", u8"切换到下一张壁纸", [this](PluginEvent, void*) {
          m_Wallpaper->SetSlot(OperatorType::Next);
        }, PluginEvent::Void
      },
    },
    {u8"dislike",
      {u8"不看此图", u8"把这张图移动到垃圾桶", [this](PluginEvent, void*) {
          m_Wallpaper->SetSlot(OperatorType::Dislike);
        }, PluginEvent::Void
      },
    },
    {u8"undoDislike",
      {u8"撤销删除", u8"撤销上次的删除操作",
       [this](PluginEvent, void*){
          m_Wallpaper->SetSlot(OperatorType::UDislike);
        }, PluginEvent::Void
      },
    },
    {u8"collect",
      {u8"收藏图片", u8"把当前壁纸复制到收藏夹", [this](PluginEvent, void*) {
        m_Wallpaper->SetSlot(OperatorType::Favorite);
        mgr->ShowMsg("收藏壁纸成功！");
      }, PluginEvent::Void},
    },
    {u8"undoCollect",
      {u8"撤销收藏", u8"如果当前壁纸在收藏夹内，则将其移出", [this](PluginEvent, void*) {
          m_Wallpaper->SetSlot(OperatorType::UFavorite);
          mgr->ShowMsg("撤销收藏壁纸成功！");
        }, PluginEvent::Void
      },
    },
    {u8"openCurrentDir",
      { u8"定位文件", u8"打开当前壁纸位置", [this](PluginEvent, void*) {
        auto curImage = m_Wallpaper->GetCurIamge();
        if (!curImage) {
          mgr->ShowMsg("找不到当前壁纸！");
          return;
        }
#ifdef _WIN32
        auto const args = L"/select, " + curImage->wstring();
        ShellExecuteW(nullptr, L"open", L"explorer", args.c_str(), NULL, SW_SHOWNORMAL);
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdWString(curImage->wstring())));
#endif
      }, PluginEvent::Void
    }},
  };

  for (auto& [name, info]: temp) {
    auto const action = menu->addAction(
          Utf82QString(info.friendlyName));
    action->setToolTip(PluginObject::Utf82QString(info.description));
    QObject::connect(action, &QAction::triggered, menu, std::bind(info.function, PluginEvent::Void, nullptr));
    m_PluginMethod[name] = std::move(info);
  }
  
  menu->addAction(m_MoreSettingsAction);
}

YJson& PluginName::InitSettings(YJson& settings)
{
  if (!settings.isObject()) {
    settings = YJson::O {
      {u8"AutoChange", false},
      {u8"FirstChange", false},
      {u8"TimeInterval", 30},
      {u8"ImageType", 0},
      // {u8"DropToCurDir", true},
      {u8"DropDir", (fs::current_path() / "junk").u8string()},
      {u8"DropImgUseUrlName", true},
      {u8"DropNameFmt", u8"drop-image {1} {0:%Y-%m-%d}"},
    };
  }
  auto& nameFmt = settings[u8"DropNameFmt"];
  if (!nameFmt.isString())
    nameFmt = u8"drop-image {1} {0:%Y-%m-%d}";
  return settings;
  // we may not need to call SaveSettings;
}

void PluginName::LoadWallpaperTypeMenu(MenuBase* pluginMenu)
{
  static const char* sources[12] = {
    "壁纸天堂", "来自https://wallhaven.cc的壁纸",
    "必应壁纸", "来自https://www.bing.com的壁纸",
    "直链壁纸", "下载并设置链接中的壁纸，例如https://source.unsplash.com/random",
    "本地壁纸", "将本地文件夹内的图片作为壁纸来源",
    "脚本壁纸", "使用脚本运行输出的本地图片路径作为壁纸",
    "收藏壁纸", "来自用户主动收藏的壁纸",
  };
  auto mainAction = pluginMenu->addAction("壁纸来源");
  mainAction->setToolTip("设置壁纸来源");
  m_WallpaperTypesMenu = new MenuBase(pluginMenu);
  mainAction->setMenu(m_WallpaperTypesMenu);
  m_WallpaperTypesGroup = new QActionGroup(m_WallpaperTypesMenu);

  const int curType = m_Wallpaper->m_Settings.GetImageType();

  for (size_t i = 0; i != 6; ++i) {
    QAction* action = m_WallpaperTypesMenu->addAction(sources[2*i]);
    action->setToolTip(sources[2*i+1]);
    action->setCheckable(true);
    m_WallpaperTypesGroup->addAction(action);
    QObject::connect(action, &QAction::triggered, m_WallpaperTypesMenu, [this, i, pluginMenu]() {
      const int oldType = m_Wallpaper->m_Settings.GetImageType();
      if (m_Wallpaper->SetImageType(i)) {
        LoadWallpaperExMenu(pluginMenu);
        mgr->ShowMsg("设置壁纸来源成功！");
      } else {
        m_WallpaperTypesGroup->actions().at(oldType)->setChecked(true);
        mgr->ShowMsg("当前正忙，设置失败！");
      }
    });
    action->setChecked(i == curType);   // very nice
  }
}

void PluginName::LoadDropMenu(QAction* action)
{
  auto const menu = new MenuBase(m_MainMenu);
  action->setMenu(menu);

  // auto actionDrop2Cur = menu->addAction("拖拽至当前");
  // actionDrop2Cur->setCheckable(true);
  // actionDrop2Cur->setToolTip("是否将拖拽进来的壁纸复制到当前类型壁纸存放的文件夹");
  // actionDrop2Cur->setChecked(m_Settings[u8"DropToCurDir"].isTrue());
  // QObject::connect(actionDrop2Cur, &QAction::triggered, m_MainMenu, [this](bool on){
  //   m_Settings[u8"DropToCurDir"] = on;
  //   mgr->SaveSettings();
  // });

  auto actionDir = menu->addAction("拖拽文件夹");
  actionDir->setToolTip("当\"拖拽至当前\"取消勾选时，拖拽壁纸存放的文件夹。");
  QObject::connect(actionDir, &QAction::triggered, m_MainMenu, [this](){
    auto u8Folder = m_Wallpaper->m_Settings.GetDropDir();
    auto qFolder = m_MainMenu->GetExistingDirectory("选择拖拽壁纸存放的文件夹", u8Folder);

    if (!qFolder) {
      mgr->ShowMsg("取消设置成功");
      return;
    }
    m_Wallpaper->m_Settings.SetDropDir(std::move(*qFolder));
    mgr->ShowMsg("设置成功！");
  });

  auto actionDefaultName = menu->addAction("使用链接名称");
  actionDefaultName->setCheckable(true);
  actionDefaultName->setToolTip("拖拽壁纸后，使用链接中的名称作为壁纸文件名称");
  actionDefaultName->setChecked(m_Settings[u8"DropImgUseUrlName"].isTrue());
  QObject::connect(actionDefaultName, &QAction::triggered, m_MainMenu, [this](bool on){
    m_Wallpaper->m_Settings.SetDropImgUseUrlName(on);
    mgr->ShowMsg("保存成功！");
  });

  auto const actionNameFmt = menu->addAction("自定义名称");
  actionNameFmt->setToolTip("当\"使用链接名称\"取消勾选时，拖拽壁纸存使用的名称。");
  QObject::connect(actionNameFmt, &QAction::triggered, m_MainMenu, [this](){
    auto u8name = m_Wallpaper->m_Settings.GetDropNameFmt();
    auto qNewName = m_MainMenu->GetNewU8String("请输入格式字符串", "输入拖拽壁纸的c++ format命名格式，错误输入会导致程序崩溃。0: 当前时间; 1: 链接自带名称.", u8name);
    if (!qNewName) {
      mgr->ShowMsg("取消设置成功！");
      return;
    }
    m_Wallpaper->m_Settings.SetDropNameFmt(std::move(*qNewName));
    mgr->ShowMsg("设置成功！");
  });
}

void PluginName::LoadWallpaperExMenu(MenuBase* parent)
{
  static MenuBase* (PluginName::*const m_MenuLoaders[6])(MenuBase*) {
    &PluginName::LoadWallavenMenu,
    &PluginName::LoadBingApiMenu,
    &PluginName::LoadDirectApiMenu,
    &PluginName::LoadNativeMenu,
    &PluginName::LoadScriptMenu,
    &PluginName::LoadFavoriteMenu,
  };
  auto menu = m_MoreSettingsAction->menu();
  if (menu) menu->deleteLater();
  menu = (this->*m_MenuLoaders[static_cast<size_t>(m_Wallpaper->m_Settings.GetImageType())])(parent);
  m_MoreSettingsAction->setMenu(menu);
}

MenuBase* PluginName::LoadWallavenMenu(MenuBase* parent)
{
  return new WallhavenExMenu(
    m_Wallpaper->Engine()->m_Setting,
    parent,
    std::bind(&WallBase::SetJson, m_Wallpaper->Engine(), std::placeholders::_1),
    std::bind(&Wallpaper::GetCurIamge, m_Wallpaper)
  );
}

MenuBase* PluginName::LoadBingApiMenu(MenuBase* parent)
{
  return new BingApiExMenu(
    m_Wallpaper->Engine()->m_Setting,
    parent,
    std::bind(&WallBase::SetJson, m_Wallpaper->Engine(), std::placeholders::_1)
  );
}

MenuBase* PluginName::LoadDirectApiMenu(MenuBase* parent)
{
  return new DirectApiExMenu(
    m_Wallpaper->Engine()->m_Setting,
    parent,
    std::bind(&WallBase::SetJson, m_Wallpaper->Engine(), std::placeholders::_1)
  );
}

MenuBase* PluginName::LoadNativeMenu(MenuBase* parent)
{
  return new NativeExMenu(
    m_Wallpaper->Engine()->m_Setting,
    parent,
    std::bind(&WallBase::SetJson, m_Wallpaper->Engine(), std::placeholders::_1)
  );
}

MenuBase* PluginName::LoadScriptMenu(MenuBase* parent)
{
  return new ScriptExMenu(
    m_Wallpaper->Engine()->m_Setting,
    parent,
    std::bind(&WallBase::SetJson, m_Wallpaper->Engine(), std::placeholders::_1)
  );
}

MenuBase* PluginName::LoadFavoriteMenu(MenuBase* parent)
{
  return new FavoriteExMenu(
    m_Wallpaper->Engine()->m_Setting,
    parent,
    std::bind(&WallBase::SetJson, m_Wallpaper->Engine(), std::placeholders::_1)
  );
}