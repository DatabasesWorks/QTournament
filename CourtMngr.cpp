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

#include <stdexcept>

#include <SqliteOverlay/ClausesAndQueries.h>

#include "CourtMngr.h"
#include "HelperFunc.h"
#include "CentralSignalEmitter.h"

using namespace SqliteOverlay;

namespace QTournament
{

  CourtMngr::CourtMngr(const TournamentDB& _db)
  : TournamentDatabaseObjectManager(_db, TAB_COURT)
  {
  }

//----------------------------------------------------------------------------

  std::optional<Court> CourtMngr::createNewCourt(const int courtNum, const QString& _name, ERR *err)
  {
    QString name = _name.trimmed();
    
    if (name.length() > MAX_NAME_LEN)
    {
      Sloppy::assignIfNotNull<ERR>(err, ERR::INVALID_NAME);
      return {};
    }
    
    if (hasCourt(courtNum))
    {
      Sloppy::assignIfNotNull<ERR>(err, ERR::COURT_NUMBER_EXISTS);
      return {};
    }
    
    // prepare a new table row
    SqliteOverlay::ColumnValueClause cvc;
    cvc.addCol(CO_NUMBER, courtNum);
    cvc.addCol(GENERIC_NAME_FIELD_NAME, QString2StdString(name));
    cvc.addCol(CO_IS_MANUAL_ASSIGNMENT, 0);
    cvc.addCol(GENERIC_STATE_FIELD_NAME, static_cast<int>(ObjState::CO_Avail));
    
    // create the new court row
    CentralSignalEmitter* cse = CentralSignalEmitter::getInstance();
    cse->beginCreateCourt();
    int newId = tab.insertRow(cvc);
    fixSeqNumberAfterInsert();
    cse->endCreateCourt(tab.length() - 1); // the new sequence number is always the highest
    
    // create a court object for the new court and return a pointer
    // to this new object
    return Court{db, newId};
  }

//----------------------------------------------------------------------------

  bool CourtMngr::hasCourt(const int courtNum)
  {
    return (tab.getMatchCountForColumnValue(CO_NUMBER, courtNum) > 0);
  }

//----------------------------------------------------------------------------

  int CourtMngr::getHighestUnusedCourtNumber() const
  {
    static const std::string sql{"SELECT max(" + std::string{CO_NUMBER} + ") FROM " + std::string{TAB_COURT}};

    try
    {
      return db.get().execScalarQueryIntOrNull(sql).value_or(1);
    }
    catch (NoDataException&) {
      return 1;
    }
  }

//----------------------------------------------------------------------------

  /**
   * Returns a database object for a court identified by its court number
   *
   * @param courtNum is the number of the court
   *
   * @return a unique_ptr to the requested court or nullptr if the court doesn't exits
   */
  std::optional<Court> CourtMngr::getCourt(const int courtNum)
  {
    return getSingleObjectByColumnValue<Court>(CO_NUMBER, courtNum);
  }

//----------------------------------------------------------------------------

  /**
   * Returns a list of all courts
   *
   * @Return QList holding all courts
   */
  std::vector<Court> CourtMngr::getAllCourts()
  {
    return getAllObjects<Court>();
  }

//----------------------------------------------------------------------------

  ERR CourtMngr::renameCourt(Court& c, const QString& _newName)
  {
    QString newName = _newName.trimmed();
    
    // Ensure the new name is valid
    if (newName.length() > MAX_NAME_LEN)
    {
      return ERR::INVALID_NAME;
    }
        
    c.row.update(GENERIC_NAME_FIELD_NAME, QString2StdString(newName));
    
    CentralSignalEmitter::getInstance()->courtRenamed(c);
    
    return ERR::OK;
  }

//----------------------------------------------------------------------------

  /**
   * Returns a database object for a court identified by its sequence number
   *
   * @param seqNum is the sequence number of the court to look up
   *
   * @return a unique_ptr to the requested court or nullptr if the court doesn't exits
   */
  std::optional<Court> CourtMngr::getCourtBySeqNum(int seqNum)
  {
    return getSingleObjectByColumnValue<Court>(GENERIC_SEQNUM_FIELD_NAME, seqNum);
  }


//----------------------------------------------------------------------------

  bool CourtMngr::hasCourtById(int id)
  {
    return (tab.getMatchCountForColumnValue("id", id) != 0);
  }

//----------------------------------------------------------------------------

  std::optional<Court> CourtMngr::getCourtById(int id)
  {
    return getSingleObjectByColumnValue<Court>("id", id);
  }

  //----------------------------------------------------------------------------

  int CourtMngr::getActiveCourtCount()
  {
    int allCourts = tab.length();
    int disabledCourts = tab.getMatchCountForColumnValue(GENERIC_STATE_FIELD_NAME, static_cast<int>(ObjState::CO_Disabled));

    return (allCourts - disabledCourts);
  }

//----------------------------------------------------------------------------

  std::optional<Court> CourtMngr::getNextUnusedCourt(bool includeManual) const
  {
    int reqState = static_cast<int>(ObjState::CO_Avail);
    SqliteOverlay::WhereClause wc;
    wc.addCol(GENERIC_STATE_FIELD_NAME, reqState);

    // further restrict the search criteria if courts for manual
    // match assignment are excluded
    if (!includeManual)
    {
      wc.addCol(CO_IS_MANUAL_ASSIGNMENT, 0);
    }

    // always get the court with the lowest number first
    wc.setOrderColumn_Asc(CO_NUMBER);

    return getSingleObjectByWhereClause<Court>(wc);
  }

//----------------------------------------------------------------------------

  bool CourtMngr::acquireCourt(const Court &co)
  {
    if (co.getState() != ObjState::CO_Avail)
    {
      return false;
    }

    co.setState(ObjState::CO_Busy);
    CentralSignalEmitter::getInstance()->courtStatusChanged(co.getId(), co.getSeqNum(), ObjState::CO_Avail, ObjState::CO_Busy);
    return true;
  }

//----------------------------------------------------------------------------

  bool CourtMngr::releaseCourt(const Court &co)
  {
    if (co.getState() != ObjState::CO_Busy)
    {
      return false;
    }

    // make sure there is no currently running match
    // assigned to this court
    SqliteOverlay::WhereClause wc;
    wc.addCol(GENERIC_STATE_FIELD_NAME, static_cast<int>(ObjState::MA_Running));
    wc.addCol(MA_COURT_REF, co.getId());
    DbTab matchTab{db, TAB_MATCH, false};
    if (matchTab.getMatchCountForWhereClause(wc) > 0)
    {
      return false;   // there is at least one running match assigned to this court
    }

    // all fine, we can fall back to AVAIL
    co.setState(ObjState::CO_Avail);
    CentralSignalEmitter::getInstance()->courtStatusChanged(co.getId(), co.getSeqNum(), ObjState::CO_Busy, ObjState::CO_Avail);
    return true;
  }

  //----------------------------------------------------------------------------

  ERR CourtMngr::disableCourt(const Court& co)
  {
    ObjState stat = co.getState();

    if (stat == ObjState::CO_Disabled) return ERR::OK;   // nothing to do for us

    // prohibit a state change if the court is in use
    if (stat == ObjState::CO_Busy) return ERR::COURT_BUSY;

    // change the court state and emit a change event
    co.setState(ObjState::CO_Disabled);
    CentralSignalEmitter::getInstance()->courtStatusChanged(co.getId(), co.getSeqNum(), stat, ObjState::CO_Disabled);
    return ERR::OK;
  }

  //----------------------------------------------------------------------------

  ERR CourtMngr::enableCourt(const Court& co)
  {
    ObjState stat = co.getState();

    if (stat != ObjState::CO_Disabled) return ERR::COURT_NOT_DISABLED;

    // change the court state and emit a change event
    co.setState(ObjState::CO_Avail);
    CentralSignalEmitter::getInstance()->courtStatusChanged(co.getId(), co.getSeqNum(), ObjState::CO_Disabled, ObjState::CO_Avail);
    return ERR::OK;
  }

  //----------------------------------------------------------------------------

  ERR CourtMngr::deleteCourt(const Court& co)
  {
    // check if the court has already been used in the past
    DbTab matchTab{db, TAB_MATCH, false};
    if (matchTab.getMatchCountForColumnValue(MA_COURT_REF, co.getId()) > 0)
    {
      return ERR::COURT_ALREADY_USED;
    }

    // after this check it is safe to delete to court because we won't
    // harm the database integrity

    CentralSignalEmitter* cse =CentralSignalEmitter::getInstance();
    int oldSeqNum = co.getSeqNum();
    cse->beginDeleteCourt(oldSeqNum);
    tab.deleteRowsByColumnValue("id", co.getId());
    fixSeqNumberAfterDelete(tab, oldSeqNum);
    cse->endDeleteCourt();

    return ERR::OK;
  }

  //----------------------------------------------------------------------------

  std::string CourtMngr::getSyncString(const std::vector<int>& rows) const
  {
    std::vector<Sloppy::estring> cols = {"id", GENERIC_NAME_FIELD_NAME, CO_NUMBER};

    return db.get().getSyncStringForTable(TAB_COURT, cols, rows);
  }

  //----------------------------------------------------------------------------

  std::optional<Court> CourtMngr::autoSelectNextUnusedCourt(ERR *err, bool includeManual) const
  {
    // find the next free court that is not subject to manual assignment
    auto nextAutoCourt = getNextUnusedCourt(false);
    if (nextAutoCourt)
    {
      // okay, we have a regular court
      Sloppy::assignIfNotNull<ERR>(err, ERR::OK);
      return nextAutoCourt;
    }

    // Damn, no court for automatic assignment available.
    // So let's check for courts with manual assignment, too.
    auto nextManualCourt = getNextUnusedCourt(true);
    if (nextManualCourt)
    {
      // okay, there is court available at all
      Sloppy::assignIfNotNull<ERR>(err, ERR::NO_COURT_AVAIL);
      return {};
    }

    // great, so there is a free court, but it's for
    // manual assigment only. If we were told to include such
    // manual courts, everything is fine
    if (includeManual)
    {
      Sloppy::assignIfNotNull<ERR>(err, ERR::OK);
      return nextManualCourt;
    }

    // indicate to the user that there would be a manual court
    Sloppy::assignIfNotNull<ERR>(err, ERR::ONLY_MANUAL_COURT_AVAIL);
    return {};
  }

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


}
