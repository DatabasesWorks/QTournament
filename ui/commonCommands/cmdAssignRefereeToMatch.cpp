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

#include <QObject>
#include <QMessageBox>

#include <SimpleReportGeneratorLib/SimpleReportViewer.h>

#include "cmdAssignRefereeToMatch.h"
#include "PlayerMngr.h"
#include "MatchMngr.h"
#include "ui/DlgSelectReferee.h"
#include "reports/ResultSheets.h"

using namespace QTournament;

cmdAssignRefereeToMatch::cmdAssignRefereeToMatch(QWidget* p, const QTournament::Match &_ma, RefereeAction _refAction)
  :AbstractCommand(_ma.getDatabaseHandle(), p), ma(_ma), refAction(_refAction)
{

}

//----------------------------------------------------------------------------

Error cmdAssignRefereeToMatch::exec()
{
  // do we actually need to assign an umpire?
  RefereeMode refMode = (refAction == RefereeAction::Swap) ? ma.get_RAW_RefereeMode() : ma.get_EFFECTIVE_RefereeMode();
  assert(refMode != RefereeMode::UseDefault);
  if ((refMode == RefereeMode::None) || (refMode == RefereeMode::HandWritten))
  {
    return Error::OK;   // nothing to do for us
  }

  // make sure we can assign a referee
  Error err = ma.canAssignReferee(refAction);
  if (err != Error::OK)
  {
    QString msg;

    if (refAction == RefereeAction::MatchCall)
    {
      msg = tr("An unexpected error occured: the match needs an umpire,\n");
      msg += tr("but an umpire can't be assigned right now.\n\n");
      msg += tr("The match cannot be started.");

    } else if (refAction == RefereeAction::PreAssign) {
      msg = tr("It is currently not possible to assign\n");
      msg += tr("an umpire to this match. Maybe the match has\n");
      msg += tr("already been started or finished?\n");
    } else if (refAction == RefereeAction::PreAssign) {
      msg = tr("It is currently not possible to swap\n");
      msg += tr("the assigned umpire for this match.");
    } else {
      msg = tr("Internal note: uncaught error in cmdAssignRefereeToMatch!");
    }

    QMessageBox::warning(parentWidget, tr("Umpire assignment failed"), msg);

    return err;
  }

  // let the user pick the referee
  DlgSelectReferee dlg{db, ma, refAction, parentWidget};
  int result = dlg.exec();
  if (result != QDialog::Accepted)
  {
    return Error::OK;
  }
  auto selPlayer = dlg.getFinalPlayerSelection();

  // if selPlayer is null and we are in a match call, the user decided to
  // continue without referee
  MatchMngr mm{db};
  if (!selPlayer && (refAction == RefereeAction::MatchCall))
  {
    err = mm.setRefereeMode(ma, RefereeMode::None);
    if (err != Error::OK)
    {
      QString msg = tr("Cannot continue without umpire!");
      QMessageBox::warning(parentWidget, tr("Umpire assignment failed"), msg);
      return err;
    }
    err = mm.removeReferee(ma);
    if (err != Error::OK)
    {
      QString msg = tr("Cannot continue without umpire!");
      QMessageBox::warning(parentWidget, tr("Umpire assignment failed"), msg);
      return err;
    }
    return Error::OK;
  }

  // in all other cases, selPlayer shouldn't be null
  assert(selPlayer);

  // actually do the assignment
  err = mm.assignReferee(ma, *selPlayer, refAction);
  if (err != Error::OK)
  {
    QString msg = tr("Could not assign umpire to match.\n");
    msg += tr("Maybe you tried to assign one of the players as umpire?");
    QMessageBox::warning(parentWidget, tr("Umpire assignment failed"), msg);
    return err;
  }

  // if we are calling a match, the user might want to print a
  // new result sheet with the freshly umpire name
  if (refAction == RefereeAction::MatchCall)
  {
    QString msg = tr("Do you want to print a new result sheet with the\n");
    msg += tr("updated umpire name for this match?");
    result = QMessageBox::question(parentWidget, tr("Print result sheet?"), msg);
    if (result == QMessageBox::Yes)
    {
      // create a report instance that is locked to show only one match
      ResultSheets sheet{db, ma};

      // let the report object create the actual output
      upSimpleReport rep = sheet.regenerateReport();

      // create an invisible report viewer and directly trigger
      // the print reaction
      SimpleReportLib::SimpleReportViewer viewer{parentWidget};
      viewer.setReport(rep.get());
      viewer.onBtnPrintClicked();
    }
  }

  return Error::OK;
}

