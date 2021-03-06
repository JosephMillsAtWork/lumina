//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LStartButton.h"
#include "../../LSession.h"

#include <LuminaXDG.h>
#include <LuminaUtils.h> //This contains the "ResizeMenu" class

LStartButtonPlugin::LStartButtonPlugin(QWidget *parent, QString id, bool horizontal) : LPPlugin(parent, id, horizontal){
  button = new QToolButton(this);
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setPopupMode(QToolButton::DelayedPopup); //make sure it runs the update routine first
    connect(button, SIGNAL(clicked()), this, SLOT(openMenu()));
    this->layout()->setContentsMargins(0,0,0,0);
    this->layout()->addWidget(button);
  menu = new ResizeMenu(this);
    menu->setContentsMargins(1,1,1,1);
    connect(menu, SIGNAL(aboutToHide()), this, SIGNAL(MenuClosed()));
    connect(menu, SIGNAL(MenuResized(QSize)), this, SLOT(SaveMenuSize(QSize)) );
  startmenu = new StartMenu(this);
    connect(startmenu, SIGNAL(CloseMenu()), this, SLOT(closeMenu()) );
    connect(startmenu, SIGNAL(UpdateQuickLaunch(QStringList)), this, SLOT(updateQuickLaunch(QStringList)));
  menu->setContents(startmenu);
  QSize saved = LSession::handle()->DesktopPluginSettings()->value("panelPlugs/"+this->type()+"/MenuSize", QSize(0,0)).toSize();
  if(!saved.isNull()){ startmenu->setFixedSize(saved); } //re-load the previously saved value
  
  button->setMenu(menu);
  connect(menu, SIGNAL(aboutToHide()), this, SLOT(updateButtonVisuals()) );
  QTimer::singleShot(0,this, SLOT(OrientationChange())); //Update icons/sizes
  QTimer::singleShot(0, startmenu, SLOT(ReLoadQuickLaunch()) );
}

LStartButtonPlugin::~LStartButtonPlugin(){

}

void LStartButtonPlugin::updateButtonVisuals(){
    button->setToolTip(tr(""));
    button->setText( SYSTEM::user() );
    button->setIcon( LXDG::findIcon("pcbsd","Lumina-DE") ); //force icon refresh
}

void LStartButtonPlugin::updateQuickLaunch(QStringList apps){
  //First clear any obsolete apps
  QStringList old;
  qDebug() << "Update QuickLaunch Buttons";
  for(int i=0; i<QUICKL.length(); i++){
    if( !apps.contains(QUICKL[i]->whatsThis()) ){
      //App was removed
      delete QUICKL.takeAt(i);
      i--;
    }else{
      //App still listed - update the button
      old << QUICKL[i]->whatsThis(); //add the list of current buttons
      LFileInfo info(QUICKL[i]->whatsThis());
      QUICKL[i]->setIcon( LXDG::findIcon(info.iconfile(),"unknown") );
      if(info.isDesktopFile()){ QUICKL[i]->setToolTip( info.XDG()->name ); }
      else{ QUICKL[i]->setToolTip( info.fileName() ); }
    }
  }
  //Now go through and create any new buttons
  for(int i=0; i<apps.length(); i++){
    if( !old.contains(apps[i]) ){
      //New App
      LQuickLaunchButton *tmp = new LQuickLaunchButton(apps[i], this);
      QUICKL << tmp;
      LFileInfo info(apps[i]);
      tmp->setIcon( LXDG::findIcon( info.iconfile() ) );
      if(info.isDesktopFile()){ tmp->setToolTip( info.XDG()->name ); }
      else{ tmp->setToolTip( info.fileName() ); }
      //Now add the button to the layout and connect the signal/slots
      this->layout()->insertWidget(i+1,tmp); //"button" is always in slot 0
      connect(tmp, SIGNAL(Launch(QString)), this, SLOT(LaunchQuick(QString)) );
      connect(tmp, SIGNAL(Remove(QString)), this, SLOT(RemoveQuick(QString)) );
    }
  }
  qDebug() << " - Done updateing QuickLaunch Buttons";
  QTimer::singleShot(0,this, SLOT(OrientationChange())); //Update icons/sizes
}

void LStartButtonPlugin::LaunchQuick(QString file){
  //Need to get which button was clicked
  qDebug() << "Quick Launch triggered:" << file;
  if(!file.isEmpty()){
    LSession::LaunchApplication("lumina-open \""+file+"\"");
    emit MenuClosed();
  }
}

void LStartButtonPlugin::RemoveQuick(QString file){
  qDebug() << "Remove Quicklaunch Button:" << file;
  if(!file.isEmpty()){
    startmenu->UpdateQuickLaunch(file, false); //always a removal
    emit MenuClosed();
  }
}

void LStartButtonPlugin::SaveMenuSize(QSize sz){
  //Save this size for the menu
  LSession::handle()->DesktopPluginSettings()->setValue("panelPlugs/"+this->type()+"/MenuSize", sz);
}

// ========================
//    PRIVATE FUNCTIONS
// ========================
void LStartButtonPlugin::openMenu(){
  if(menu->isVisible()){ return; } //don't re-show it - already open
  startmenu->UpdateMenu();
  button->showMenu();
}

void LStartButtonPlugin::closeMenu(){
  menu->hide();
}

