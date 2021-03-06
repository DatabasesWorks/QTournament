/*
 * Copyright © 2014 Volker Knollmann
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file or visit
 * http://www.wtfpl.net/ for more details.
 * 
 * This program comes without any warranty. Use it at your own risk or
 * don't use it at all.
 */

#ifndef DATABASETESTSCENARIO_H
#define	DATABASETESTSCENARIO_H

#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <QtSql/QSqlDatabase>

#include "BasicTestClass.h"
#include "GenericDatabase.h"
#include "TournamentDB.h"
#include "Category.h"

class DatabaseTestScenario : public BasicTestClass
{
  //class Category;

public:

protected:
  static const QString DB_CONNECTION_NAME;
  static const QString DB_USER;
  static const QString DB_PASSWD;
  static const QString MYSQL_HOST;
  static const QString MYSQL_DB;
  static const int MYSQL_PORT;

  static const QString SQLITE_DB;

  QSqlDatabase getDbConn(dbOverlay::GenericDatabase::DB_ENGINE t);
  QSqlDatabase getDbConn();
  void removeDbConn();
  void cleanupMysql();
  
  void prepScenario01(bool useTeams = true);
  QTournament::TournamentDB* getScenario01(bool useTeams = true);
  void prepScenario02(bool useTeams = true);
  QTournament::TournamentDB* getScenario02(bool useTeams = true);
  void prepScenario03(bool useTeams = true);
  QTournament::TournamentDB* getScenario03(bool useTeams = true);
  void prepScenario04(bool useTeams = true);
  QTournament::TournamentDB* getScenario04(bool useTeams = true);
  void prepScenario05(bool useTeams = true);
  QTournament::TournamentDB* getScenario05(bool useTeams = true);
  void prepScenario06(bool useTeams = true);
  QTournament::TournamentDB* getScenario06(bool useTeams = true);

  void execQueryAndDumpError(QSqlQuery& qry, const QString& sqlStatement="");

  bool playRound(const QTournament::Category& cat, int round, int expectedMatchCount);

public:
  virtual void tearDown ();
  QString getSqliteFileName();
  bool sqliteFileExists();
};

#endif	/* DATABASETESTSCENARIO_H */

