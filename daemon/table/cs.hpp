/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 *
 *  \brief implements the ContentStore
 *
 *  This ContentStore implementation consists of two data structures,
 *  a Table, and a set of cleanup queues.
 *
 *  The Table is a container (std::set) sorted by full Names of stored Data packets.
 *  Data packets are wrapped in Entry objects.
 *  Each Entry contain the Data packet itself,
 *  and a few addition attributes such as the staleness of the Data packet.
 *
 *  The cleanup queues are three doubly linked lists which stores Table iterators.
 *  The three queues keep track of unsolicited, stale, and fresh Data packet, respectively.
 *  Table iterator is placed into, removed from, and moved between suitable queues
 *  whenever an Entry is added, removed, or has other attribute changes.
 *  The Table iterator of an Entry should be in exactly one queue at any moment.
 *  Within each queue, the iterators are kept in first-in-first-out order.
 *  Eviction procedure exhausts the first queue before moving onto the next queue,
 *  in the order of unsolicited, stale, and fresh queue.
 */

#ifndef NFD_DAEMON_TABLE_CS_HPP
#define NFD_DAEMON_TABLE_CS_HPP

#include "cs-entry-impl.hpp"
#include <boost/iterator/transform_iterator.hpp>

namespace nfd {
namespace cs {

/** \brief represents the ContentStore
 */
class Cs : noncopyable
{
public:
  explicit
  Cs(size_t nMaxPackets = 10);

  /** \brief inserts a Data packet
   *  \return true
   */
  bool
  insert(const Data& data, bool isUnsolicited = false);

  bool
  insert(const Data& data, const Link& link, bool isUnsolicited = false);

  /** \brief finds the best matching Data packet
   */
  const Data*
  find(const Interest& interest) const;

  void
  erase(const Name& exactName)
  {
    BOOST_ASSERT_MSG(false, "not implemented");
  }

  /** \brief changes capacity (in number of packets)
   */
  void
  setLimit(size_t nMaxPackets);

  /** \return capacity (in number of packets)
   */
  size_t
  getLimit() const
  {
    return m_limit;
  }

  /** \return number of stored packets
   */
  size_t
  size() const
  {
    return m_table.size();
  }

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  dump();

public: // enumeration
  struct EntryFromEntryImpl
  {
    typedef const Entry& result_type;

    const Entry&
    operator()(const EntryImpl& entry) const
    {
      return entry;
    }
  };

  /** \brief ContentStore iterator (public API)
   */
  typedef boost::transform_iterator<EntryFromEntryImpl, TableIt, const Entry&> const_iterator;

  const_iterator
  begin() const
  {
    return boost::make_transform_iterator(m_table.begin(), EntryFromEntryImpl());
  }

  const_iterator
  end() const
  {
    return boost::make_transform_iterator(m_table.end(), EntryFromEntryImpl());
  }

private: // find
  /** \brief find leftmost match in [first,last)
   *  \return the leftmost match, or last if not found
   */
  TableIt
  findLeftmost(const Interest& interest, TableIt left, TableIt right) const;

  /** \brief find rightmost match in [first,last)
   *  \return the rightmost match, or last if not found
   */
  TableIt
  findRightmost(const Interest& interest, TableIt first, TableIt last) const;

  /** \brief find rightmost match among entries with exact Names in [first,last)
   *  \return the rightmost match, or last if not found
   */
  TableIt
  findRightmostAmongExact(const Interest& interest, TableIt first, TableIt last) const;

private: // cleanup
  /** \brief attaches an entry to a suitable cleanup queue
   *  \note if the entry is already attached to a queue, it's automatically detached
   */
  void
  attachQueue(TableIt it);

  /** \brief detaches an entry from its current cleanup queue
   *  \warning if the entry is unattached, this will cause undefined behavior
   */
  void
  detachQueue(TableIt it);

  /** \brief moves an entry from FIFO queue to STALE queue
   */
  void
  moveToStaleQueue(TableIt it);

  /** \brief picks an entry for eviction
   */
  std::tuple<TableIt, std::string/*reason*/>
  evictPick();

  /** \brief evicts zero or more entries so that number of stored entries
   *         is not greater than capacity
   */
  void
  evict();

private:
  size_t m_limit;
  Table m_table;
  Queue m_queues[QUEUE_MAX];
};

} // namespace cs

using cs::Cs;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_HPP
