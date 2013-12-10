/*
** g2cachetablemodel.cpp
**
** Copyright © Quazaa Development Team, 2013-2013.
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

#include <QAbstractItemView>

#include "g2cachetablemodel.h"

#include "debug_new.h"

HostData::HostData(SharedG2HostPtr pHost) :
	m_pHost(        pHost                ),
	m_oAddress(     pHost->address()     ),
	m_sAddress(     m_oAddress.toStringWithPort() ),
	m_sCountryCode( m_oAddress.country() ),
	m_sCountry( geoIP.countryNameFromCode( m_sCountryCode ) ),
	m_iCountry( QIcon(":/Resource/Flags/" + m_sCountryCode.toLower() + ".png") ),
	m_nID(          pHost->id()          ),
	m_tLastConnect( pHost->lastConnect() ),
	m_sLastConnect( QDateTime::fromTime_t( m_tLastConnect ).toString() ),
	m_nFailures(    pHost->failures()    ),
	m_sFailures( QString::number( m_nFailures ) )
{
}

/**
 * @brief update refreshes the data within HostData if necessary.
 * Locking: REQUIRES hostCache.m_pSection
 * @param nRow : the row being refreshed
 * @param nSortCol : the currently sorted column
 * @param lToUpdate : the list of indexes that have changed
 * @param pModel : the model
 * @return true if an entry within the column col has been modified
 */
bool HostData::update(int nRow, int nSortCol, QModelIndexList& lToUpdate,
					  G2CacheTableModel* pModel)
{
	Q_ASSERT( !m_pHost.isNull() );

	bool bReturn = false;

	// address and country never change
	if ( m_tLastConnect != m_pHost->lastConnect() )
	{
		lToUpdate.append( pModel->index( nRow, LASTCONNECT ) );
		m_tLastConnect = m_pHost->lastConnect();
		m_sLastConnect = QDateTime::fromTime_t( m_tLastConnect ).toString();

		if ( nSortCol == LASTCONNECT )
			bReturn = true;
	}

	if ( m_nFailures != m_pHost->failures() )
	{
		lToUpdate.append( pModel->index( nRow, FAILURES ) );
		m_nFailures = m_pHost->failures();
		m_sFailures = QString::number( m_nFailures );

		if ( nSortCol == FAILURES )
			bReturn = true;
	}

	return bReturn;
}

/**
 * @brief RuleData::data
 * @param col
 * @return
 */
QVariant HostData::data(int col) const
{
	switch ( col )
	{
	case ADDRESS:
		return m_sAddress;

	case LASTCONNECT:
		return m_sLastConnect;

	case FAILURES:
		return m_sFailures;

	case COUNTRY:
		return m_sCountry;

	default:
		return QVariant();
	}
}

/*Rule* RuleData::rule() const
{
	return m_bRemoving ? NULL : m_pRule;
}*/

bool HostData::lessThan(int col, HostData* pOther) const
{
	if ( !pOther )
		return false;

	switch ( col )
	{
	case ADDRESS:
		return m_sAddress     < pOther->m_sAddress;

	case LASTCONNECT:
		return m_tLastConnect < pOther->m_tLastConnect;

	case FAILURES:
		return m_nFailures    < pOther->m_nFailures;

	case COUNTRY:
		return m_sCountryCode < pOther->m_sCountryCode;

	default:
		return false;
	}
}

G2CacheTableModel::G2CacheTableModel(QObject* parent, QWidget* container) :
	QAbstractTableModel( parent ),
	m_oContainer( container ),
	m_nSortColumn( -1 ),
	m_bNeedSorting( false )
{
	// register necessary meta types before using them
	hostCache.registerMetaTypes();

	connect( &hostCache, SIGNAL( hostAdded( SharedG2HostPtr ) ), this,
			 SLOT( addHost( SharedG2HostPtr ) ), Qt::QueuedConnection );

	connect( &hostCache, SIGNAL( hostRemoved( SharedG2HostPtr ) ), this,
			 SLOT( removeHost( SharedG2HostPtr ) ), Qt::QueuedConnection );

	// This handles GUI updates on rule changes.
	connect( &hostCache, SIGNAL( hostUpdated( quint32 ) ), this,
			 SLOT( updateHost( quint32 ) ), Qt::QueuedConnection );

	// This needs to be called to make sure that all rules added to the host cache before this
	// part of the GUI is loaded are properly added to the model.
	completeRefresh();
}

G2CacheTableModel::~G2CacheTableModel()
{
	m_vHosts.clear();
}

int G2CacheTableModel::rowCount(const QModelIndex& parent) const
{
	if ( parent.isValid() )
	{
		return 0;
	}
	else
	{
		return (int)m_vHosts.size();
	}
}

int G2CacheTableModel::columnCount(const QModelIndex& parent) const
{
	if ( parent.isValid() )
	{
		return 0;
	}
	else
	{
		return _NO_OF_COLUMNS;
	}
}

QVariant G2CacheTableModel::data(const QModelIndex& index, int nRole) const
{
	if ( !index.isValid() || index.row() > m_vHosts.size() || index.row() < 0 )
	{
		Q_ASSERT( false );
		return QVariant();
	}

	const HostData* pData = m_vHosts[ index.row() ];

	switch ( nRole )
	{
	case Qt::DisplayRole:
		return pData->data( index.column() );

	case Qt::DecorationRole:
		if ( index.column() == COUNTRY )
		{
			return pData->m_iCountry;
		}
		break;

	// TODO: Reimplement formatting options in models.
	/*case Qt::ForegroundRole:
		switch ( nbr->nState )
		{
		case nsConnected:
			//return skinSettings.listsColorSpecial;
			break;
		case nsConnecting:
			//return skinSettings.listsColorActive;
			break;
		default:
			//return skinSettings.listsColorNormal;
			break;
		}
		return QVariant();*/

	/*case Qt::FontRole:
		QFont font = qApp->font( m_oContainer );
		switch ( nbr->nState )
		{
		case nsConnected:
			//font.setWeight(skinSettings.listsWeightSpecial);
			return font;
			break;
		case nsConnecting:
			//font.setWeight(skinSettings.listsWeightActive);
			return font;
			break;
		default:
			//font.setWeight(skinSettings.listsWeightNormal);
			return font;
			break;
		}*/

	default:
		break;
	}

	return QVariant();
}

QVariant G2CacheTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ( orientation != Qt::Horizontal )
		return QVariant();

	if ( role == Qt::DisplayRole )
	{
		switch ( section )
		{
		case ADDRESS:
			return tr( "IP Address" );

		case LASTCONNECT:
			return tr( "Last Connected" );

		case FAILURES:
			return tr( "Failure Count" );

		case COUNTRY:
			return tr( "Country" );
		}
	}
	else if ( role == Qt::ToolTipRole )
	{
		switch(section)
		{
		case ADDRESS:
			return tr( "The Host IP Address" );

		case LASTCONNECT:
			return tr( "Last time this host has been connected" );

		case FAILURES:
			return tr( "The number of unsuccessful connections in a row" );

		case COUNTRY:
			return tr( "The host country" );
		}
	}

	return QVariant();
}

QModelIndex G2CacheTableModel::index(int row, int column, const QModelIndex& parent) const
{
	if ( parent.isValid() || row < 0 || row >= m_vHosts.size() )
		return QModelIndex();
	else
		return createIndex( row, column, m_vHosts[row] );
}

class G2CacheTableModelCmp
{
public:
	G2CacheTableModelCmp(int nCol, Qt::SortOrder nOrder) :
		m_nColumn( nCol ),
		m_nOrder( nOrder )
	{
	}

	bool operator()(HostData* a, HostData* b)
	{
		if ( m_nOrder == Qt::AscendingOrder )
		{
			return a->lessThan( m_nColumn, b );
		}
		else
		{
			return b->lessThan( m_nColumn, a );
		}
	}

	int m_nColumn;
	Qt::SortOrder m_nOrder;
};

void G2CacheTableModel::sort(int column, Qt::SortOrder order)
{
	m_nSortColumn = column;
	m_nSortOrder  = order;

	emit layoutAboutToBeChanged();

	// I hope this is the correct way to convince Qt...
	QModelIndexList oldIdx = persistentIndexList();
	QModelIndexList newIdx = oldIdx;

	qStableSort( m_vHosts.begin(), m_vHosts.end(), G2CacheTableModelCmp( column, order ) );

	for ( int i = 0; i < oldIdx.size(); ++i ) // For each persistent index
	{
		int oldRow = oldIdx.at(i).row();

		// if oldRow is outside range
		if ( oldRow > m_vHosts.size()
				// or the index points to another item
				|| oldIdx.at(i).internalPointer() != m_vHosts[oldRow] )
		{
			// find the correct item and update persistent index
			for ( int j = 0; j < m_vHosts.size(); ++j )
			{
				if ( oldIdx.at(i).internalPointer() == m_vHosts[j] )
				{
					newIdx[i] = createIndex( j, oldIdx.at(i).column(),
											 oldIdx.at(i).internalPointer() );
					break;
				}
			}
		}
	}

	changePersistentIndexList( oldIdx, newIdx );
	emit layoutChanged();

	m_bNeedSorting = false;
}

int G2CacheTableModel::find(quint32 nRuleID)
{
	const int nSize = (int)m_vHosts.size();

	for ( int nPos = 0; nPos < nSize; ++nPos )
	{
		if ( m_vHosts[nPos]->m_nID == nRuleID )
			return nPos;
	}

	return -1;
}

G2CacheTableModel::HostData* G2CacheTableModel::dataFromRow(int nRow) const
{
	if ( nRow < m_vHosts.size() && nRow >= 0 )
		return m_vHosts[nRow];
	else
		return NULL;
}

/*Rule* SecurityTableModel::ruleFromIndex(const QModelIndex &index) const
{
	if ( index.isValid() && index.row() < m_lNodes.count() && index.row() >= 0 )
		return m_lNodes[ index.row() ]->m_pRule;
	else
		return NULL;
}*/

void G2CacheTableModel::completeRefresh()
{
	// Remove all rules.
	if ( m_vHosts.size() )
	{
		beginRemoveRows( QModelIndex(), 0, (int)m_vHosts.size() - 1 );
		for ( uint i = 0; i < m_vHosts.size(); ++i )
			delete m_vHosts[i];
		m_vHosts.clear();
		endRemoveRows();
	}

	// Note that this slot is automatically disconnected once all rules have been recieved once.
	connect( &hostCache, SIGNAL( hostInfo( SharedG2HostPtr ) ), this,
			 SLOT( recieveHostInfo( SharedG2HostPtr ) ), Qt::QueuedConnection );

	// Request getting them back from the Security Manager.
	m_nHostInfo = hostCache.requestHostInfo();
}

/**
 * @brief SecurityTableModel::triggerRuleRemoval
 * @param nIndex
 */
void G2CacheTableModel::triggerHostRemoval(int nIndex)
{
	Q_ASSERT( nIndex >= 0 && nIndex < m_vHosts.size() );

	hostCache.remove( m_vHosts[nIndex]->m_oAddress );
}

void G2CacheTableModel::recieveHostInfo(SharedG2HostPtr pHost)
{
	--m_nHostInfo;

	// This handles disconnecting the ruleInfo signal after a completeRefresh() has been finished.
	if ( !m_nHostInfo )
	{
		// Make sure we don't recieve any signals we don't want once we got all hosts once.
		disconnect( &hostCache, SIGNAL( hostInfo( SharedG2HostPtr ) ),
					this, SLOT( recieveHostInfo( SharedG2HostPtr ) ) );
	}

	addHost( pHost );
}

/**
 * @brief addHost adds a rule to the GUI.
 * @param pHost : the host
 */
void G2CacheTableModel::addHost(SharedG2HostPtr pHost)
{
	hostCache.m_pSection.lock();
	Q_ASSERT( hostCache.check( pHost ) );
	insert( new HostData( pHost ) );
	hostCache.m_pSection.unlock();
}

/**
 * @brief removeHost removes a host from the table model.
 * This is to be triggered from the host cache AFTER the host has been removed.
 * @param pHost : the host
 */
void G2CacheTableModel::removeHost(SharedG2HostPtr pHost)
{
	for ( int i = 0; i < m_vHosts.size(); ++i )
	{
		if ( m_vHosts[i]->m_pHost == pHost.data() )
		{
			erase( i );

			// m_bNeedSorting needs not to be set to true here as sorting is not required on removal
			break;
		}
	}
}

/**
 * @brief updateHost updates the GUI for a specified host.
 * @param nHostID : the ID of the host
 */
void G2CacheTableModel::updateHost(quint32 nHostID)
{
	QModelIndexList uplist;
	bool bSort = m_bNeedSorting;

	const int nHostRowPos = find( nHostID );

	Q_ASSERT( nHostRowPos != -1 );

	hostCache.m_pSection.lock();
	if ( m_vHosts[nHostRowPos]->update( nHostRowPos, m_nSortColumn, uplist, this ) )
	{
		bSort = true;
	}
	hostCache.m_pSection.unlock();

	// if necessary adjust container order (also updates view)
	if ( bSort )
	{
		sort( m_nSortColumn, m_nSortOrder );
		m_bNeedSorting = false;
	}
	// update view for all changed model indexes
	else if ( !uplist.isEmpty() )
	{
		QAbstractItemView* pView = qobject_cast< QAbstractItemView* >( m_oContainer );

		if ( pView )
		{
			foreach ( QModelIndex index, uplist )
			{
				pView->update( index );
			}
		}
	}
}

/**
 * @brief updateAll updates all hosts in the GUI.
 */
void G2CacheTableModel::updateAll()
{
	QModelIndexList uplist;
	bool bSort = m_bNeedSorting;

	hostCache.m_pSection.lock();
	for ( int i = 0, max = (int)m_vHosts.size(); i < max; ++i )
	{
		if ( m_vHosts[i]->update( i, m_nSortColumn, uplist, this ) )
		{
			bSort = true;
		}
	}
	hostCache.m_pSection.unlock();

	if ( bSort )
	{
		sort( m_nSortColumn, m_nSortOrder );
		m_bNeedSorting = false;
	}
	else
	{
		if ( !uplist.isEmpty() )
		{
			QAbstractItemView* pView = qobject_cast< QAbstractItemView* >( m_oContainer );

			if( pView )
			{
				foreach ( QModelIndex index, uplist )
				{
					pView->update( index );
				}
			}
		}
	}
}

void G2CacheTableModel::insert(HostData* pData)
{
	if ( m_bNeedSorting )
	{
		beginInsertRows( QModelIndex(), (int)m_vHosts.size(), (int)m_vHosts.size() );

		m_vHosts.push_back( pData );

		endInsertRows();

		sort( m_nSortColumn, m_nSortOrder );
	}
	else // TODO: improve efficiency later
	{
		beginInsertRows( QModelIndex(), (int)m_vHosts.size(), (int)m_vHosts.size() );

		m_vHosts.push_back( pData );

		endInsertRows();

		sort( m_nSortColumn, m_nSortOrder );
	}
}

void G2CacheTableModel::erase(int nPos)
{
	beginRemoveRows( QModelIndex(), nPos, nPos );
	delete m_vHosts[nPos];

	for ( int i = nPos; i < m_vHosts.size() - 1; ++i )
	{
		m_vHosts[i] = m_vHosts[i+1];
	}
	m_vHosts.pop_back();

	endRemoveRows();
}
