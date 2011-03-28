//
// datagrams.h
//
// Copyright © Quazaa Development Team, 2009-2010.
// This file is part of QUAZAA (quazaa.sourceforge.net)
//
// Quazaa is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// Quazaa is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Quazaa; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef DATAGRAMS_H
#define DATAGRAMS_H

#include "types.h"
#include <QMutex>
#include <QUdpSocket>
#include <QHash>
#include <QLinkedList>
#include <QTimer>
#include <QTime>

#include "queryhit.h"

class G2Packet;

class DatagramWatcher
{
public:
	virtual      ~DatagramWatcher();
	virtual void OnSuccess(void* pParam) = 0;
	virtual void OnFailure(void* pParam) = 0;
};

class DatagramOut;
class DatagramIn;
class CBuffer;
class QHostAddress;

class CDatagrams : public QObject
{
	Q_OBJECT

public:
	QMutex      m_pSection;
protected:
	quint32 m_nUploadLimit;

	QUdpSocket* m_pSocket;

	bool m_bFirewalled;

	QTimer*     m_tSender;
	QTime       m_tStopWatch;

	QHash<quint16, DatagramOut*>     m_SendCacheMap;    // zeby szybko odszukac pakiety po sekwencji
	QLinkedList<DatagramOut*>		 m_SendCache;		// a lifo queue, last is oldest
	QLinkedList<DatagramOut*>		 m_FreeDGOut;
	quint16                          m_nSequence;

	QHash < QHostAddress,
		  QHash<quint32, DatagramIn*>
		  >                    m_RecvCache;		// for searching by ip & sequence
	QLinkedList<DatagramIn*> m_RecvCacheTime;	// a list ordered by recv time, last is oldest

	QLinkedList<DatagramIn*> m_FreeDGIn;		// a list of free incoming packets
	QLinkedList<CBuffer*>	 m_FreeBuffer;		// a list of free buffers

	CBuffer*    	m_pRecvBuffer;
	QHostAddress*   m_pHostAddress;
	quint16         m_nPort;

	bool            m_bActive;

	quint32         m_nBandwidthIn;
	quint32         m_nAvgBandwidthIn;
	quint32         m_nBandwidthOut;
	quint32         m_nAvgBandwidthOut;
	QTime           m_tMeterTimer;

	quint32 m_nDiscarded;

	QLinkedList<QPair<QHostAddress, G2Packet*> > m_lPendingQKA;
	QLinkedList<QPair<QUuid, G2Packet*> >   m_lPendingQA;
	QLinkedList<QueuedQueryHit>   m_lPendingQH2;

public:
	CDatagrams();
	~CDatagrams();

	void Listen();
	void Disconnect();

	void SendPacket(CEndPoint& oAddr, G2Packet* pPacket, bool bAck = false, DatagramWatcher* pWatcher = 0, void* pParam = 0);

	void RemoveOldIn(bool bForce = false);
	void Remove(DatagramIn* pDG, bool bReclaim = false);
	void Remove(DatagramOut* pDG);
	void OnReceiveGND();
	void OnAcknowledgeGND();

	void OnPacket(CEndPoint addr, G2Packet* pPacket);
	// pierdołki
	void OnPing(CEndPoint& addr, G2Packet* pPacket);
	void OnPong(CEndPoint& addr, G2Packet* pPacket);
	void OnCRAWLR(CEndPoint& addr, G2Packet* pPacket);
	// szukanie
	void OnQKA(CEndPoint& addr, G2Packet* pPacket);
	void OnQA(CEndPoint& addr, G2Packet* pPacket);
	void OnQH2(CEndPoint& addr, G2Packet* pPacket);

	inline void UpdateStats()
	{
		if(m_tMeterTimer.elapsed() < 1000)
		{
			return;
		}

		m_tMeterTimer.start();

		m_nAvgBandwidthIn = (m_nAvgBandwidthIn + m_nBandwidthIn) / 2;
		m_nAvgBandwidthOut = (m_nAvgBandwidthOut + m_nBandwidthOut) / 2;
		m_nBandwidthIn = m_nBandwidthOut = 0;
	}
	inline quint32 DownloadSpeed()
	{
		UpdateStats();
		return m_nAvgBandwidthIn;
	}
	inline quint32 UploadSpeed()
	{
		UpdateStats();
		return m_nAvgBandwidthOut;
	}

	inline bool IsFirewalled()
	{
		return m_bFirewalled;
	}
	inline bool isListening()
	{
		return (m_bActive && m_pSocket && m_pSocket->isValid());
	}

public slots:
	void OnDatagram();
	void FlushSendCache();

	void SetupThread();
	void CleanupThread();

signals:
	void SendQueueUpdated();
	void PacketQueuedForRouting();

	friend class CNetwork;
};

#pragma pack(push, 1)
typedef struct
{
	char     szTag[3];
	quint8   nFlags;
	quint16  nSequence;
	quint8   nPart;
	quint8   nCount;
} GND_HEADER;

#pragma pack(pop)

extern CDatagrams Datagrams;

#endif // DATAGRAMS_H
