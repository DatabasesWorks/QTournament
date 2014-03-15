/* 
 * File:   TeamMngr.h
 * Author: nyoknvk1
 *
 * Created on 18. Februar 2014, 14:04
 */

#ifndef PLAYERMNGR_H
#define	PLAYERMNGR_H

#include "TournamentDB.h"
#include "Team.h"
#include "Player.h"
#include "TournamentDataDefs.h"
#include "TournamentErrorCodes.h"
#include "DbTab.h"
#include "GenericObjectManager.h"

#include <QList>

using namespace dbOverlay;

namespace QTournament
{

  class PlayerMngr : public GenericObjectManager
  {
  public:
    PlayerMngr (TournamentDB* _db);
    ERR createNewPlayer (const QString& firstName, const QString& lastName, SEX sex, const QString& teamName);
    bool hasPlayer (const QString& firstName, const QString& lastName);
    Player getPlayer(const QString& firstName, const QString& lastName);
    QList<Player> getAllPlayers();
    ERR renamePlayer (Player& p, const QString& newFirst, const QString& newLast);

  private:
    DbTab playerTab;
  };
}

#endif	/* TEAMMNGR_H */
