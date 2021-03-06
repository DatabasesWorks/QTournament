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

#include <vector>

#include "PlayerProfile.h"
#include "Player.h"
#include "PlayerMngr.h"
#include "PlayerPair.h"
#include "Match.h"
#include "MatchMngr.h"

using namespace SqliteOverlay;

namespace QTournament
{

  PlayerProfile::PlayerProfile(const Player& _p)
    :db{_p.getDatabaseHandle()}, p{_p}, lastPlayedMatchId{-1},
      currentMatchId{-1}, nextMatchId{-1}, lastUmpireMatchId{-1},
      currentUmpireMatchId{-1}, nextUmpireMatchId{-1},
      finishCount{0}, walkoverCount{0}, scheduledCount{0},
      umpireFinishedCount{0}
  {
    initMatchLists();
    initMatchIds();
  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::getLastPlayedMatch() const
  {
    return returnMatchOrNullptr(lastPlayedMatchId);
  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::getCurrentMatch() const
  {
    return returnMatchOrNullptr(currentMatchId);
  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::getNextMatch() const
  {
    return returnMatchOrNullptr(nextMatchId);
  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::getLastUmpireMatch() const
  {
    return returnMatchOrNullptr(lastUmpireMatchId);
  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::getCurrentUmpireMatch() const
  {
    return returnMatchOrNullptr(currentUmpireMatchId);
  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::getNextUmpireMatch() const
  {
    return returnMatchOrNullptr(nextUmpireMatchId);
  }

  //----------------------------------------------------------------------------

  void PlayerProfile::initMatchIds()
  {
    MatchMngr mm{db};

    // loop over all umpire matches and find the next scheduled match,
    // the currently running match and the last finished match
    QDateTime lastFinishTime;
    int nextMatchNum = -1;
    for (const Match& ma : matchesAsUmpire)
    {
      OBJ_STATE stat = ma.getState();

      if (stat == STAT_MA_FINISHED) ++umpireFinishedCount;

      if (ma.getState() == STAT_MA_RUNNING)
      {
        currentUmpireMatchId = ma.getId();
        continue;
      }

      if (stat == STAT_MA_FINISHED)
      {
        QDateTime fTime = ma.getFinishTime();
        if (fTime.isValid())
        {
          if ((!(lastFinishTime.isValid())) || (fTime > lastFinishTime))  // this does not include walkovers
          {
            lastFinishTime = fTime;
            lastUmpireMatchId = ma.getId();
            continue;
          }
        }
      }

      int maNum = ma.getMatchNumber();
      if ((maNum != MATCH_NUM_NOT_ASSIGNED) && ((maNum < nextMatchNum) || (nextMatchNum < 0)))
      {
        nextMatchNum = maNum;
        nextUmpireMatchId = ma.getId();
      }
    }


    // loop over all player matches and find the next scheduled match,
    // the currently running match and the last finished match
    lastFinishTime = QDateTime{};   // set to "invalid"
    nextMatchNum = -1;
    for (const Match& ma : matchesAsPlayer)
    {
      OBJ_STATE stat = ma.getState();
      int maNum = ma.getMatchNumber();

      // count all scheduled matches
      if (maNum != MATCH_NUM_NOT_ASSIGNED) ++scheduledCount;

      if (stat == STAT_MA_RUNNING)
      {
        currentMatchId = ma.getId();
        continue;
      }

      if (stat == STAT_MA_FINISHED)
      {
        ++finishCount;
        QDateTime fTime = ma.getFinishTime();
        if (fTime.isValid())
        {
          if ((!(lastFinishTime.isValid())) || (fTime > lastFinishTime))
          {
            lastFinishTime = fTime;
            lastPlayedMatchId = ma.getId();
          }
        } else {
          // invalid finish time indicates a walkover
          ++walkoverCount;
        }
        continue;
      }

      if ((maNum != MATCH_NUM_NOT_ASSIGNED) && ((maNum < nextMatchNum) || (nextMatchNum < 0)))
      {
        nextMatchNum = maNum;
        nextMatchId = ma.getId();
      }
    }
  }

  //----------------------------------------------------------------------------

  void PlayerProfile::initMatchLists()
  {
    //
    // find all matches involving the participant as a PLAYER
    //

    // step 1: search via PlayerPairs
    QString where = "%1=%2 OR %3=%2";
    where = where.arg(PAIRS_PLAYER1_REF);
    where = where.arg(p.getId());
    where = where.arg(PAIRS_PLAYER2_REF);
    DbTab* pairsTab = db->getTab(TAB_PAIRS);
    auto it = pairsTab->getRowsByWhereClause(where.toUtf8().constData());
    vector<int> matchIdList;

    DbTab* matchTab = db->getTab(TAB_MATCH);
    MatchMngr mm{db};
    while (!(it.isEnd()))
    {
      TabRow r = *it;
      int ppId = r.getId();

      // search for all matches involving this player pair
      where = "%1=%2 OR %3=%2";
      where = where.arg(MA_PAIR1_REF);
      where = where.arg(ppId);
      where = where.arg(MA_PAIR2_REF);
      auto matchIt = matchTab->getRowsByWhereClause(where.toUtf8().constData());
      while (!(matchIt.isEnd()))
      {
        int maId = (*matchIt).getId();

        if (!(Sloppy::isInVector<int>(matchIdList, maId))) matchIdList.push_back(maId);

        ++matchIt;
      }

      ++it;
    }

    // step 2: search via ACTUAL_PLAYER
    where = "%1=%2 OR %3=%2 OR %4=%2 OR %5=%2";
    where = where.arg(MA_ACTUAL_PLAYER1A_REF);
    where = where.arg(p.getId());
    where = where.arg(MA_ACTUAL_PLAYER1B_REF);
    where = where.arg(MA_ACTUAL_PLAYER2A_REF);
    where = where.arg(MA_ACTUAL_PLAYER2B_REF);
    it = matchTab->getRowsByWhereClause(where.toUtf8().constData());
    while (!(it.isEnd()))
    {
      int maId = (*it).getId();

      if (!(Sloppy::isInVector<int>(matchIdList, maId))) matchIdList.push_back(maId);

      ++it;
    }

    // copy the results to the matchesAsPlayer-list
    for (int maId : matchIdList)
    {
      auto ma = mm.getMatch(maId);
      matchesAsPlayer.push_back(*ma);
    }

    // sort by match number
    std::sort(matchesAsPlayer.begin(), matchesAsPlayer.end(), [](const Match& ma1, const Match& ma2)
    {
      return ma1.getMatchNumber() < ma2.getMatchNumber();
    });


    //
    // find all matches involving the participant as an UMPIRE
    //
    matchIdList.clear();
    it = matchTab->getRowsByColumnValue(MA_REFEREE_REF, p.getId());
    while (!(it.isEnd()))
    {
      int maId = (*it).getId();

      auto ma = mm.getMatch(maId);
      matchesAsUmpire.push_back(*ma);

      ++it;
    }

    // sort by match number
    std::sort(matchesAsUmpire.begin(), matchesAsUmpire.end(), [](const Match& ma1, const Match& ma2)
    {
      return ma1.getMatchNumber() < ma2.getMatchNumber();
    });

  }

  //----------------------------------------------------------------------------

  unique_ptr<Match> PlayerProfile::returnMatchOrNullptr(int maId) const
  {
    if (maId < 1) return nullptr;

    MatchMngr mm{db};
    return mm.getMatch(maId);
  }

  //----------------------------------------------------------------------------

}
