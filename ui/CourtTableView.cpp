/*
 *    This is QTournament, a badminton tournament management program.
 *    Copyright (C) 2014 - 2017  Volker Knollmann
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QMessageBox>
#include <QScrollBar>

#include "CourtTableView.h"
#include "MainFrame.h"
#include "CourtMngr.h"
#include "MatchMngr.h"
#include "ui/GuiHelpers.h"
#include "ui/commonCommands/cmdAssignRefereeToMatch.h"
#include "reports/ResultSheets.h"
#include <SimpleReportGeneratorLib/SimpleReportViewer.h>

CourtTableView::CourtTableView(QWidget* parent)
  :GuiHelpers::AutoSizingTableView_WithDatabase<CourtTableModel>{
     GuiHelpers::AutosizeColumnDescrList{
       {"", 1, ABS_COURT_COL_WIDTH, ABS_COURT_COL_WIDTH},
       {"", 1, -1, -1},
       {"", 1, ABS_DURATION_COL_WIDTH, ABS_DURATION_COL_WIDTH}
     }, true, parent}
{
  setRubberBandCol(1);

  // react on selection changes in the court table view
  connect(selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  // handle context menu requests
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(onContextMenuRequested(const QPoint&)));

  // handle double clicks on a column header
  connect(horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(onSectionHeaderDoubleClicked()));

  // setup the context menu and its actions
  initContextMenu();
}

//----------------------------------------------------------------------------
    
CourtTableView::~CourtTableView()
{
}

//----------------------------------------------------------------------------
    
unique_ptr<Court> CourtTableView::getSelectedCourt() const
{
  int srcRow = getSelectedSourceRow();
  if (srcRow < 0) return nullptr;

  CourtMngr cm{db};
  return cm.getCourtBySeqNum(srcRow);
}

//----------------------------------------------------------------------------

unique_ptr<Match> CourtTableView::getSelectedMatch() const
{
  auto co = getSelectedCourt();
  if (co == nullptr) return nullptr;

  return co->getMatch();
}

//----------------------------------------------------------------------------

void CourtTableView::hook_onDatabaseOpened()
{
  AutoSizingTableView_WithDatabase::hook_onDatabaseOpened();

  // set a new delegate
  courtItemDelegate = new CourtItemDelegate(db, this);
  courtItemDelegate->setProxy(sortedModel.get());
  setCustomDelegate(courtItemDelegate);   // Takes ownership
}

//----------------------------------------------------------------------------

void CourtTableView::onSelectionChanged(const QItemSelection& selectedItem, const QItemSelection& deselectedItem)
{
  resizeRowsToContents();
  for (auto item : selectedItem)
  {
    courtItemDelegate->setSelectedRow(item.top());
    resizeRowToContents(item.top());
  }
  for (auto item : deselectedItem)
  {
    resizeRowToContents(item.top());
  }
}

//----------------------------------------------------------------------------
    
void CourtTableView::initContextMenu()
{
  // prepare all actions
  actAddCourt = new QAction(tr("Add court"), this);
  actUndoCall = new QAction(tr("Undo call"), this);
  actFinishMatch = new QAction(tr("Finish match"), this);
  actAddCall = new QAction(tr("Repeat call"), this);
  actSwapReferee = new QAction(tr("Swap umpire"), this);
  actToggleAssignmentMode = new QAction(tr("Only manual match assignment on this court"), this);
  actToggleAssignmentMode->setCheckable(true);
  actToggleEnableState = new QAction(tr("Court disabled"), this);
  actToggleEnableState->setCheckable(true);
  actDelCourt = new QAction(tr("Delete court"), this);
  actReprintResultSheet = new QAction(tr("Re-print result sheet"), this);

  // create sub-actions for the walkover-selection
  actWalkoverP1 = new QAction("P1", this);  // this is just a dummy
  actWalkoverP2 = new QAction("P2", this);  // this is just a dummy

  // link actions to slots
  connect(actAddCourt, SIGNAL(triggered()), this, SLOT(onActionAddCourtTriggered()));
  connect(actWalkoverP1, SIGNAL(triggered(bool)), this, SLOT(onWalkoverP1Triggered()));
  connect(actWalkoverP2, SIGNAL(triggered(bool)), this, SLOT(onWalkoverP2Triggered()));
  connect(actUndoCall, SIGNAL(triggered(bool)), this, SLOT(onActionUndoCallTriggered()));
  connect(actAddCall, SIGNAL(triggered(bool)), this, SLOT(onActionAddCallTriggered()));
  connect(actSwapReferee, SIGNAL(triggered(bool)), this, SLOT(onActionSwapRefereeTriggered()));
  connect(actToggleAssignmentMode, SIGNAL(triggered(bool)), this, SLOT(onActionToggleMatchAssignmentModeTriggered()));
  connect(actToggleEnableState, SIGNAL(triggered(bool)), this, SLOT(onActionToogleEnableStateTriggered()));
  connect(actDelCourt, SIGNAL(triggered()), this, SLOT(onActionDeleteCourtTriggered()));
  connect(actReprintResultSheet, SIGNAL(triggered()), this, SLOT(onReprintResultSheetTriggered()));

  // create the context menu and connect it to the actions
  contextMenu = make_unique<QMenu>();
  contextMenu->addAction(actFinishMatch);
  contextMenu->addAction(actAddCall);
  walkoverSelectionMenu = contextMenu->addMenu(tr("Walkover for..."));
  walkoverSelectionMenu->addAction(actWalkoverP1);
  walkoverSelectionMenu->addAction(actWalkoverP2);
  contextMenu->addAction(actUndoCall);
  contextMenu->addSeparator();
  contextMenu->addAction(actSwapReferee);
  contextMenu->addSeparator();
  contextMenu->addAction(actReprintResultSheet);
  contextMenu->addSeparator();
  contextMenu->addAction(actToggleAssignmentMode);
  contextMenu->addAction(actToggleEnableState);
  contextMenu->addSeparator();
  contextMenu->addAction(actAddCourt);
  contextMenu->addAction(actDelCourt);

  updateContextMenu(false);
}

//----------------------------------------------------------------------------

void CourtTableView::updateContextMenu(bool isRowClicked)
{
  // disable / enable actions that depend on a selected match
  auto ma = getSelectedMatch();
  bool isMatchClicked = (isRowClicked && (ma != nullptr));
  actUndoCall->setEnabled(isMatchClicked);
  walkoverSelectionMenu->setEnabled(isMatchClicked);
  actFinishMatch->setEnabled(isMatchClicked);

  QList<QDateTime> callTimes;
  if (ma != nullptr)
  {
    callTimes = ma->getAdditionalCallTimes();
  }
  actAddCall->setEnabled(isMatchClicked && (callTimes.size() < MAX_NUM_ADD_CALL));

  // enable / disable actions that depend on a selected row
  actAddCourt->setEnabled(!isRowClicked);
  actDelCourt->setEnabled(isRowClicked);

  // update the player pair names for the walkover menu
  if (ma != nullptr)
  {
    actWalkoverP1->setText(ma->getPlayerPair1().getDisplayName());
    actWalkoverP2->setText(ma->getPlayerPair2().getDisplayName());
  }

  // disable "swap umpire" if we have no umpire in the match
  if ((ma != nullptr) && isRowClicked)
  {
    REFEREE_MODE refMode = ma->get_RAW_RefereeMode();
    bool canSwapReferee = ((refMode != REFEREE_MODE::NONE) &&
                          (refMode != REFEREE_MODE::HANDWRITTEN) &&
                          (refMode != REFEREE_MODE::USE_DEFAULT));
    actSwapReferee->setEnabled(canSwapReferee);
  } else {
    actSwapReferee->setEnabled(false);
  }

  // disable result sheet printing if there is no match selected
  actReprintResultSheet->setEnabled(isMatchClicked);

  // show the state of the "manual assignment"
  unique_ptr<Court> co = getSelectedCourt();
  actToggleAssignmentMode->setEnabled(isRowClicked);
  if (co != nullptr)
  {
    actToggleAssignmentMode->setChecked(co->isManualAssignmentOnly());
  }

  // show the enable state
  if ((co != nullptr) && isRowClicked)
  {
    OBJ_STATE coStat = co->getState();
    actToggleEnableState->setEnabled(coStat != STAT_CO_BUSY);
    actToggleEnableState->setChecked(coStat == STAT_CO_DISABLED);
  } else {
    actToggleEnableState->setEnabled(false);
    actToggleEnableState->setChecked(false);
  }
}

//----------------------------------------------------------------------------

void CourtTableView::execWalkover(int playerNum)
{
  auto ma = getSelectedMatch();
  if (ma == nullptr) return;
  if ((playerNum != 1) && (playerNum != 2)) return; // shouldn't happen
  GuiHelpers::execWalkover(this, *ma, playerNum);
}

//----------------------------------------------------------------------------
    
void CourtTableView::onContextMenuRequested(const QPoint& pos)
{
  // map from scroll area coordinates to global widget coordinates
  QPoint globalPos = viewport()->mapToGlobal(pos);

  // resolve the click coordinates to the table row
  int clickedRow = rowAt(pos.y());

  // if no table row is under the cursor, we may only
  // add a court. If a row is under the cursor, we may
  // do everything except for adding courts
  bool isRowClicked = (clickedRow >= 0);
  updateContextMenu(isRowClicked);

  // show the context menu; actions are triggered
  // by the menu itself and we do not need to take
  // further steps here
  contextMenu->exec(globalPos);
}

//----------------------------------------------------------------------------

void CourtTableView::onActionAddCourtTriggered()
{
  CourtMngr cm{db};

  int nextCourtNum = cm.getHighestUnusedCourtNumber();

  ERR err;
  cm.createNewCourt(nextCourtNum, QString::number(nextCourtNum), &err);

  if (err != OK)
  {
    QMessageBox::warning(this, tr("Add court"),
                         tr("Something went wrong, error code = ") + QString::number(static_cast<int>(err)));
  }
}

//----------------------------------------------------------------------------

void CourtTableView::onWalkoverP1Triggered()
{
  execWalkover(1);
}

//----------------------------------------------------------------------------

void CourtTableView::onWalkoverP2Triggered()
{
  execWalkover(2);
}

//----------------------------------------------------------------------------

void CourtTableView::onActionUndoCallTriggered()
{
  auto ma = getSelectedMatch();
  if (ma == nullptr) return;

  MatchMngr mm{db};
  mm.undoMatchCall(*ma);
}

//----------------------------------------------------------------------------

void CourtTableView::onActionAddCallTriggered()
{
  auto ma = getSelectedMatch();
  if (ma == nullptr) return;

  QList<QDateTime> callTimes = ma->getAdditionalCallTimes();
  if (callTimes.size() > MAX_NUM_ADD_CALL) return;

  auto co = ma->getCourt();
  assert(co != nullptr);

  QString callText = GuiHelpers::prepCall(*ma, *co, callTimes.size() + 1);

  int result = QMessageBox::question(this, tr("Repeat call"), callText);

  if (result == QMessageBox::Yes)
  {
    ma->addAddtionalCallTime();
    return;
  }
  QMessageBox::information(this, tr("Repeat call"), tr("Call cancled"));
}

//----------------------------------------------------------------------------

void CourtTableView::onActionSwapRefereeTriggered()
{
  auto ma = getSelectedMatch();
  if (ma == nullptr) return;

  // see if we can assign a new referee
  if (ma->canAssignReferee(REFEREE_ACTION::SWAP) != OK) return;

  // trigger the assign-umpire-procedure
  cmdAssignRefereeToMatch cmd{this, *ma, REFEREE_ACTION::SWAP};
  cmd.exec();
}

//----------------------------------------------------------------------------

void CourtTableView::onSectionHeaderDoubleClicked()
{
  autosizeColumns();
}

//----------------------------------------------------------------------------

void CourtTableView::onActionToggleMatchAssignmentModeTriggered()
{
  unique_ptr<Court> co = getSelectedCourt();
  if (co == nullptr) return;

  bool isManual = co->isManualAssignmentOnly();
  co->setManualAssignment(!isManual);
}

//----------------------------------------------------------------------------

void CourtTableView::onActionToogleEnableStateTriggered()
{
  unique_ptr<Court> co = getSelectedCourt();
  if (co == nullptr) return;

  CourtMngr cm{db};
  if (co->getState() == STAT_CO_DISABLED)
  {
    cm.enableCourt(*co);
  } else {
    cm.disableCourt(*co);
  }
}

//----------------------------------------------------------------------------

void CourtTableView::onActionDeleteCourtTriggered()
{
  unique_ptr<Court> co = getSelectedCourt();
  if (co == nullptr) return;

  // ask a safety question
  QString msg = tr("You are about to delete\n\n");
  msg += tr("     Court %1\n\n");
  msg += tr("Proceed and delete?");
  msg = msg.arg(co->getNumber());
  int result = QMessageBox::question(this, tr("Delete court"), msg);
  if (result !=  QMessageBox::Yes) return;

  CourtMngr cm{db};
  ERR e = cm.deleteCourt(*co);

  if (e == COURT_ALREADY_USED)
  {
    QString msg = tr("The court has already been used for matches and thus\n");
    msg += tr("it can't be deleted for technical reasons.\n\n");
    msg += tr("Please consider disabling the court instead of deleting it.");

    QMessageBox::warning(this, tr("Delete court"), msg);
    return;
  }

  if (e == DATABASE_ERROR)
  {
    QString msg = tr("A database error occured when trying to delete the court.\n");
    msg += tr("The court can't be deleted.\n\n");
    msg += tr("Please consider disabling the court instead of deleting it.");

    QMessageBox::warning(this, tr("Delete court"), msg);
    return;
  }

  if (e != OK)
  {
    QString msg = tr("Some error occured when trying to delete the court.\n\n");
    msg += tr("The court can't be deleted.\n\n");

    QMessageBox::warning(this, tr("Delete court"), msg);
    return;
  }
}

//----------------------------------------------------------------------------

void CourtTableView::onReprintResultSheetTriggered()
{
  auto curMatch = getSelectedMatch();
  if (curMatch == nullptr) return;

  // try to create the result sheet report with the
  // requested number of matches
  unique_ptr<ResultSheets> rep{nullptr};
  try
  {
    rep = make_unique<ResultSheets>(db, *curMatch, 1);
  }
  catch (...) {}
  if (rep == nullptr) return;

  // let the report object create the actual output
  upSimpleReport sr = rep->regenerateReport();

  // create an invisible report viewer and directly trigger
  // the print reaction
  SimpleReportLib::SimpleReportViewer viewer{this};
  viewer.setReport(sr.get());
  viewer.onBtnPrintClicked();
}

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

