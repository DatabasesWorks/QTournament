/* 
 * File:   GenericDatabaseObject.cpp
 * Author: nyoknvk1
 * 
 * Created on 18. Februar 2014, 14:25
 */

#include <stdexcept>

#include "GenericDatabaseObject.h"

namespace QTournament
{

GenericDatabaseObject::GenericDatabaseObject(const QString& _tabName, int _id)
: tabName(_tabName), id(_id)
{
    if (id < 1)
    {
        throw std::invalid_argument("GenericDatabaseObject ctor: invalid id");
    }
    
    if (tabName.length() == 0)
    {
        throw std::invalid_argument("GenericDatabaseObject ctor: invalid, zero length table name");
    }
}

}