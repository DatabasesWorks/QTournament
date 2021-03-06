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

#include "ThreadSafeQueue.h"


ProgressQueue::ProgressQueue(int _maxVal)
{
  if (_maxVal <= 0)
  {
    throw std::invalid_argument("Ctor: can't have a max progress value less or equal to zero");
  }

  maxVal = _maxVal;

  scaleFac = 100.0 / maxVal;
}

void ProgressQueue::step(int numSteps)
{
  if (numSteps <= 0) return;

  // acquire a lock on the mutex
  lock_guard<mutex> lk{queueMutex};

  // calculate the new value
  counter += numSteps;
  if (counter > maxVal) counter = maxVal;

  // map the new value to a 0...100 range and store the it
  q.push((int)(counter * scaleFac));

  // notify potentially waiting threads that
  // the queue is not empty anymore
  emptyCondition.notify_one();

  // lk is automatically unlocked by the
  // destructor of the lock_guard
}

void ProgressQueue::reset(int _maxVal)
{
  if (_maxVal <= 0)
  {
    throw std::invalid_argument("Reset: Can't have a max progress value less or equal to zero");
  }

  // acquire a lock on the mutex
  lock_guard<mutex> lk{queueMutex};

  // clear all old items from the list and
  // reinitialize members
  queue<int> empty;
  q.swap(empty);
  maxVal = _maxVal;
  scaleFac = 100.0 / maxVal;
  counter = 0;
}
