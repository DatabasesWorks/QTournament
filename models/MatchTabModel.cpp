/* 
 * File:   PlayerTableModel.cpp
 * Author: volker
 * 
 * Created on March 17, 2014, 7:51 PM
 */

#include "MatchTabModel.h"

#include "Category.h"
#include "Tournament.h"
#include "../ui/GuiHelpers.h"
#include <QDebug>

using namespace QTournament;
using namespace dbOverlay;

MatchTableModel::MatchTableModel(TournamentDB* _db)
:QAbstractTableModel(0), db(_db), matchTab((_db->getTab(TAB_MATCH)))
{
  connect(Tournament::getMatchMngr(), &MatchMngr::beginCreateMatch, this, &MatchTableModel::onBeginCreateMatch, Qt::DirectConnection);
  connect(Tournament::getMatchMngr(), &MatchMngr::endCreateMatch, this, &MatchTableModel::onEndCreateMatch, Qt::DirectConnection);
  connect(Tournament::getMatchMngr(), &MatchMngr::matchStatusChanged, this, &MatchTableModel::onMatchStatusChanged, Qt::DirectConnection);
}

//----------------------------------------------------------------------------

int MatchTableModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid()) return 0;
  return matchTab.length();
}

//----------------------------------------------------------------------------

int MatchTableModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid()) return 0;
  return COLUMN_COUNT;
}

//----------------------------------------------------------------------------

QVariant MatchTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
      //return QVariant();
      return QString("Invalid index");

    if (index.row() >= matchTab.length())
      //return QVariant();
      return QString("Invalid row: " + QString::number(index.row()));

    if (role != Qt::DisplayRole)
      return QVariant();
    
    auto ma = Tournament::getMatchMngr()->getMatchBySeqNum(index.row());
    auto mg = ma->getMatchGroup();
    
    // first column: match num
    if (index.column() == MATCH_NUM_COL_ID)
    {
      return ma->getMatchNumber();
    }

    // second column: match name
    if (index.column() == 1)
    {
      return ma->getDisplayName();
    }

    // third column: category name
    if (index.column() == 2)
    {
      Category c = mg.getCategory();
      return c.getName();
    }

    // fourth column: round
    if (index.column() == 3)
    {
      return mg.getRound();
    }

    // fifth column: players group, if applicable
    if (index.column() == 4)
    {
      return GuiHelpers::groupNumToString(mg.getGroupNumber());
    }

    // sixth column: the match state; this column is used for filtering and
    // needs to be hidden in the view
    if (index.column() == STATE_COL_ID)
    {
      return static_cast<int>(ma->getState());
    }

    return QString("Not Implemented, row=" + QString::number(index.row()) + ", col=" + QString::number(index.row()));
}

//----------------------------------------------------------------------------

QVariant MatchTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
  {
    return QVariant();
  }

  if (section < 0)
  {
    return QVariant();
  }
  
  if (orientation == Qt::Horizontal)
  {
    if (section == MATCH_NUM_COL_ID) {
      return tr("Number");
    }
    if (section == 1) {
      return tr("Match");
    }
    if (section == 2) {
      return tr("Category");
    }
    if (section == 3) {
      return tr("Round");
    }
    if (section == 4) {
      return tr("Group");
    }
    if (section == STATE_COL_ID) {
      return tr("State");
    }

    return QString("Not implemented, section=" + QString::number(section));
  }
  
  return QString::number(section + 1);
}

//----------------------------------------------------------------------------

void MatchTableModel::onBeginCreateMatch()
{
  int newPos = matchTab.length();
  beginInsertRows(QModelIndex(), newPos, newPos);
}
//----------------------------------------------------------------------------

void MatchTableModel::onEndCreateMatch(int newMatchSeqNum)
{
  endInsertRows();

  // tell all associated views to update their filters
  //emit triggerFilterUpdate();
}

//----------------------------------------------------------------------------

void MatchTableModel::onMatchStatusChanged(int matchId, int matchSeqNum)
{
  QModelIndex startIdx = createIndex(matchSeqNum, 0);
  QModelIndex endIdx = createIndex(matchSeqNum, COLUMN_COUNT-1);
  emit dataChanged(startIdx, endIdx);
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

