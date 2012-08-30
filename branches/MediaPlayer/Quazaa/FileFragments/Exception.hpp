/*
** $Id: Exception.hpp 858 2012-01-14 23:34:50Z brov $
**
** Copyright © Quazaa Development Team, 2009-2012.
** This file is part of QUAZAA (quazaa.sourceforge.net)
**
** Quazaa is free software; this file may be used under the terms of the GNU
** General Public License version 3.0 or later as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Quazaa is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** Please review the following information to ensure the GNU General Public
** License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** You should have received a copy of the GNU General Public License version
** 3.0 along with Quazaa; if not, write to the Free Software Foundation,
** Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// FileFragments code borrowed from Shareaza
// http://shareaza.sourceforge.net/

#ifndef FILEFRAGMENTS_EXCEPTION_HPP_INCLUDED
#define FILEFRAGMENTS_EXCEPTION_HPP_INCLUDED

namespace Ranges
{

// @class exception
//       This is the general exception class used in this subsystem.
//       All exceptions generated by any facility of this subsystem
//       are guarantied to be derived from it, except for ::std::bad_alloc.
//       This allows exception handling on per subsystem base if desired.
class Exception : public std::exception
{
public:
	Exception(const char* msg) throw() : m_msg( msg ) { }
	~Exception() throw() { }
	char const* what() const throw() { return m_msg; }
private:
	const char* m_msg;
};

template< class RangeT >
class RangeError : public Exception
{
public:
	typedef RangeT range_type;
	typedef typename range_type::size_type range_size_type;
	typedef typename range_type::payload_type payload_type;
	RangeError(range_size_type begin, range_size_type end, const payload_type& payload) throw()
	: Exception( "invalid range" ), m_begin( begin ), m_end( end ), m_payload( payload )
	{ }
	~RangeError() throw() { }
	range_size_type begin() const throw() { return m_begin; }
	range_size_type end() const throw() { return m_end; }
private:
	range_size_type m_begin;
	range_size_type m_end;
	payload_type m_payload;
};

template< class RangeT >
class ListError : public Exception
{
public:
	typedef RangeT range_type;
	typedef typename range_type::size_type range_size_type;
	typedef typename range_type::payload_type payload_type;
	ListError(const range_type& range, range_size_type limit) throw()
	: Exception( "range exceeds list limit" ), m_range( range ), m_limit( limit )
	{ }
	~ListError() throw() { }
	range_size_type begin() const throw() { return m_range.begin(); }
	range_size_type end() const throw() { return m_range.end(); }
	range_size_type length() const throw()
	{
		return m_range.end() - m_range.begin();
	}
	range_size_type limit() const throw() { return m_limit; }
	const range_type& value() const throw() { return m_range; }
private:
	range_type m_range;
	range_size_type m_limit;
};

} // namespace Ranges

#endif // #ifndef FILEFRAGMENTS_EXCEPTION_HPP_INCLUDED
