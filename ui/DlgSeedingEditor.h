#ifndef DLGSEEDINGEDITOR_H
#define DLGSEEDINGEDITOR_H

#include <QDialog>

#include "Tournament.h"
#include "PlayerPair.h"

namespace Ui {
  class DlgSeedingEditor;
}

class DlgSeedingEditor : public QDialog
{
  Q_OBJECT

public:
  explicit DlgSeedingEditor(QWidget *parent = 0);
  ~DlgSeedingEditor();
  void initSeedingList(const PlayerPairList& _seed);

public slots:
  void onBtnUpClicked();
  void onBtnDownClicked();
  void onBtnShuffleClicked();
  void onShuffleModeChange();
  void onSelectionChanged();


private:
  Ui::DlgSeedingEditor *ui;
  void updateButtons();
};

#endif // DLGSEEDINGEDITOR_H
