/*****************************************************************************

Copyright (c) 1995, 2015, Oracle and/or its affiliates. All rights reserved.
Copyright (c) 2019, 2021, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/buf0types.h
The database buffer pool global types for the directory

Created 11/17/1995 Heikki Tuuri
*******************************************************/

#pragma once
#include "univ.i"

/** Buffer page (uncompressed or compressed) */
class buf_page_t;
/** Buffer block for which an uncompressed page exists */
struct buf_block_t;
/** Buffer pool statistics struct */
struct buf_pool_stat_t;
/** Buffer pool buddy statistics struct */
struct buf_buddy_stat_t;

/** A buffer frame. @see page_t */
typedef	byte	buf_frame_t;

/** Flags for io_fix types */
enum buf_io_fix {
	BUF_IO_NONE = 0,		/**< no pending I/O */
	BUF_IO_READ,			/**< read pending */
	BUF_IO_WRITE,			/**< write pending */
	BUF_IO_PIN			/**< disallow relocation of
					block and its removal of from
					the flush_list */
};

/** Alternatives for srv_checksum_algorithm, which can be changed by
setting innodb_checksum_algorithm */
enum srv_checksum_algorithm_t {
  /** Write crc32; allow full_crc32,crc32,innodb,none when reading */
  SRV_CHECKSUM_ALGORITHM_CRC32,
  /** Write crc32; allow full_crc23,crc32 when reading */
  SRV_CHECKSUM_ALGORITHM_STRICT_CRC32,
  /** For new files, always compute CRC-32C for the whole page.
  For old files, allow crc32, innodb or none when reading. */
  SRV_CHECKSUM_ALGORITHM_FULL_CRC32,
  /** For new files, always compute CRC-32C for the whole page.
  For old files, allow crc32 when reading. */
  SRV_CHECKSUM_ALGORITHM_STRICT_FULL_CRC32
};

inline bool is_checksum_strict(srv_checksum_algorithm_t algo)
{
  return algo == SRV_CHECKSUM_ALGORITHM_STRICT_CRC32;
}

inline bool is_checksum_strict(ulint algo)
{
  return algo == SRV_CHECKSUM_ALGORITHM_STRICT_CRC32;
}

/** Parameters of binary buddy system for compressed pages (buf0buddy.h) */
/* @{ */
/** Zip shift value for the smallest page size */
#define BUF_BUDDY_LOW_SHIFT	UNIV_ZIP_SIZE_SHIFT_MIN

/** Smallest buddy page size */
#define BUF_BUDDY_LOW		(1U << BUF_BUDDY_LOW_SHIFT)

/** Actual number of buddy sizes based on current page size */
#define BUF_BUDDY_SIZES		(srv_page_size_shift - BUF_BUDDY_LOW_SHIFT)

/** Maximum number of buddy sizes based on the max page size */
#define BUF_BUDDY_SIZES_MAX	(UNIV_PAGE_SIZE_SHIFT_MAX	\
				- BUF_BUDDY_LOW_SHIFT)

/** twice the maximum block size of the buddy system;
the underlying memory is aligned by this amount:
this must be equal to srv_page_size */
#define BUF_BUDDY_HIGH	(BUF_BUDDY_LOW << BUF_BUDDY_SIZES)
/* @} */

/** Page identifier. */
class page_id_t
{
public:

  /** Constructor from (space, page_no).
  @param[in]	space	tablespace id
  @param[in]	page_no	page number */
  page_id_t(ulint space, uint32_t page_no) : m_id(uint64_t{space} << 32 | page_no)
  {
    ut_ad(space <= 0xFFFFFFFFU);
  }

  page_id_t(uint64_t id) : m_id(id) {}
  bool operator==(const page_id_t& rhs) const { return m_id == rhs.m_id; }
  bool operator!=(const page_id_t& rhs) const { return m_id != rhs.m_id; }
  bool operator<(const page_id_t& rhs) const { return m_id < rhs.m_id; }
  bool operator>(const page_id_t& rhs) const { return m_id > rhs.m_id; }
  bool operator<=(const page_id_t& rhs) const { return m_id <= rhs.m_id; }
  bool operator>=(const page_id_t& rhs) const { return m_id >= rhs.m_id; }
  page_id_t &operator--() { ut_ad(page_no()); m_id--; return *this; }
  page_id_t &operator++()
  {
    ut_ad(page_no() < 0xFFFFFFFFU);
    m_id++;
    return *this;
  }
  page_id_t operator-(uint32_t i) const
  {
    ut_ad(page_no() >= i);
    return page_id_t(m_id - i);
  }
  page_id_t operator+(uint32_t i) const
  {
    ut_ad(page_no() < ~i);
    return page_id_t(m_id + i);
  }

  /** Retrieve the tablespace id.
  @return tablespace id */
  uint32_t space() const { return static_cast<uint32_t>(m_id >> 32); }

  /** Retrieve the page number.
  @return page number */
  uint32_t page_no() const { return static_cast<uint32_t>(m_id); }

  /** Retrieve the fold value.
  @return fold value */
  ulint fold() const { return (space() << 20) + space() + page_no(); }

  /** Reset the page number only.
  @param[in]	page_no	page number */
  void set_page_no(uint32_t page_no)
  {
    m_id= (m_id & ~uint64_t{0} << 32) | page_no;
  }

  ulonglong raw() { return m_id; }

private:
  /** The page identifier */
  uint64_t m_id;
};

/** A field reference full of zero, for use in assertions and checks,
and dummy default values of instantly dropped columns.
Initially, BLOB field references are set to zero, in
dtuple_convert_big_rec(). */
extern const byte field_ref_zero[UNIV_PAGE_SIZE_MAX];

#ifndef UNIV_INNOCHECKSUM

/** Latch types */
enum rw_lock_type_t
{
  RW_S_LATCH= 1 << 0,
  RW_X_LATCH= 1 << 1,
  RW_SX_LATCH= 1 << 2,
  RW_NO_LATCH= 1 << 3
};

#include "sux_lock.h"

class page_hash_latch : public rw_lock
{
public:
  /** Wait for a shared lock */
  void read_lock_wait();
  /** Wait for an exclusive lock */
  void write_lock_wait();

  /** Acquire a shared lock */
  inline void read_lock();
  /** Acquire an exclusive lock */
  inline void write_lock();

  /** Acquire a lock */
  template<bool exclusive> void acquire()
  {
    if (exclusive)
      write_lock();
    else
      read_lock();
  }
  /** Release a lock */
  template<bool exclusive> void release()
  {
    if (exclusive)
      write_unlock();
    else
      read_unlock();
  }
};

#endif /* !UNIV_INNOCHECKSUM */
