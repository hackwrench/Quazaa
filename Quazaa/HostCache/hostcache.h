/*
** hostcache.h
**
** Copyright © Quazaa Development Team, 2009-2013.
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

#ifndef HOSTCACHE_H
#define HOSTCACHE_H

#include <QMutex>
#include <QAtomicInt>

#include "hostcachehost.h"

#define ENABLE_HOST_CACHE_DEBUGGING    0
#define ENABLE_HOST_CACHE_BENCHMARKING 0

// Increment this if there have been made changes to the way of storing Host Cache Hosts.
#define HOST_CACHE_CODE_VERSION	7
// History:
// 4 - Initial implementation.
// 6 - Fixed Hosts having an early date and changed time storage from QDateTime to quint32.
// 7 - Changed host failure counter from quint32 to quint8.

// TODO: purge QMetaObject::invokeMethod
// TODO: in canQuery there are calls to .isNull
// TODO: things to test
//  12:58:39 brov: 1. does it use hosts from host cache b4 GWC query succeeds
//  12:59:25 brov: 2. search manager must request query keys directly from other nodes (use a random search string)

class QFile;

class G2HostCache : public QObject
{
	Q_OBJECT

public:
	// allows access to 0 .. m_nMaxFailures + 1
	G2HostCacheIterator*    m_pFailures; // = new G2HostCacheIterator[m_nMaxFailures + 2];
	G2HostCacheList         m_lHosts;

	CEndPoint               m_oLokalAddress;

#if ENABLE_HOST_CACHE_BENCHMARKING
	QAtomicInt              m_nLockWaitTime;
	QAtomicInt              m_nWorkTime;
#endif

	mutable QMutex          m_pSection;
	mutable quint32         m_tLastSave; //TODO: use qatomicint

	quint8                  m_nMaxFailures;
	QAtomicInt              m_nSizeAtomic;

	// Thread used by the Host Cache
	SharedThreadPtr         m_pHostCacheDiscoveryThread;

public:
	G2HostCache();
	~G2HostCache();

	void start();

	void add(const CEndPoint host, const quint32 tTimeStamp);
	void addKey(const CEndPoint host, const quint32 tTimeStamp,
				CEndPoint* pKeyHost, const quint32 nKey, const quint32 tNow);
	void addAck(const CEndPoint host, const quint32 tTimeStamp,
				const quint32 tAck, const quint32 tNow);

	SharedG2HostPtr get(const CEndPoint& oHost);
	bool check(const SharedG2HostPtr pHost) const;

	void updateFailures(const CEndPoint& oAddress, const quint32 nFailures);

private: // remove this private if this is ever required...
//	SharedG2HostPtr update(const CEndPoint& oHost,     const quint32 tTimeStamp);
	SharedG2HostPtr update(G2HostCacheIterator& itHost, const quint32 tTimeStamp,
						   const quint32 nFailures);

public:
	void remove(const CEndPoint& oHost);
	void remove(SharedG2HostPtr pHost);

	void addXTry(QString sHeader);
	QString getXTry() const;

	void onFailure(const CEndPoint& addr);
	SharedG2HostPtr getConnectable(const QSet<SharedG2HostPtr>& oExcept = QSet<SharedG2HostPtr>(),
								   QString sCountry = QString("ZZ"));
	bool hasConnectable();

	void clear();
	void save(const quint32 tNow) const;

private:
	void pruneOldHosts(const quint32 tNow);

public:
	void pruneByQueryAck(const quint32 tNow);

	static quint32 writeToFile(const void* const pManager, QFile& oFile);

	quint32 count() const;
	bool isEmpty() const;

	quint32 requestHostInfo();

	/**
	 * @brief Manager::registerMetaTypes registers the necessary meta types for signals and slots.
	 */
	void            registerMetaTypes();

signals:
	/**
	 * @brief ruleAdded informs about a new rule having been added.
	 * @param pRule : the rule
	 */
	void            hostAdded(SharedG2HostPtr pHost);

	/**
	 * @brief ruleRemoved informs about a rule having been removed.
	 * @param pRule : the rule
	 */
	void            hostRemoved(SharedG2HostPtr pHost);

	/**
	 * @brief ruleInfo info signal to get informed about all rules within the manager.
	 * See Manager::requestRuleList() for more information.
	 * @param pRule : the rule
	 */
	void            hostInfo(SharedG2HostPtr pHost);

	/**
	 * @brief ruleUpdated informs about a rule having been updated.
	 * @param nID : the GUI ID of the updated rule
	 */
	void            hostUpdated(quint32 nID);

public slots:
	void localAddressChanged();

	SharedG2HostPtr addSync(CEndPoint host, quint32 tTimeStamp, bool bLock);
	SharedG2HostPtr addSyncKey(CEndPoint host, quint32 tTimeStamp, CEndPoint* pKeyHost,
							   const quint32 nKey, const quint32 tNow, bool bLock);
	SharedG2HostPtr addSyncAck(CEndPoint host, quint32 tTimeStamp, const quint32 tAck,
							   const quint32 tNow, bool bLock);

	void removeSync(CEndPoint oHost);

	void sanityCheck();
	void maintain();

private:
	SharedG2HostPtr addSyncHelper(const CEndPoint& oHostIP, quint32 tTimeStamp,
									const quint32 tNow, quint32 nNewFailures = 0);
	void insert(SharedG2HostPtr pNew, G2HostCacheIterator& it);

	G2HostCacheIterator remove(G2HostCacheIterator& itHost);
	void removeWorst(quint8& nFailures);

	G2HostCacheIterator      find(const CEndPoint& oHost);
	G2HostCacheIterator      find(const SharedG2HostPtr pHost);
//	THostCacheConstIterator find(const CHostCacheHost* const pHost) const;

	void load();

private slots:
	void asyncStartUpHelper();
	void asyncUpdateFailures(CEndPoint oAddress, quint32 nNewFailures);
	void asyncAddXTry(QString sHeader);
	void asyncOnFailure(CEndPoint addr);
};

extern G2HostCache hostCache;

#endif // HOSTCACHE_H
