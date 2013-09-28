/*
** $Id$
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

#include "datagrams.h"
#include "network.h"
#include "neighbours.h"
#include "datagramfrags.h"
#include "g2node.h"
#include "g2packet.h"
#include "searchmanager.h"
#include "Hashes/hash.h"
#include "queryhit.h"
#include "systemlog.h"
#include "querykeys.h"
#include "query.h"

#include "HostCache/hostcache.h"

#include "quazaaglobals.h"
#include "quazaasettings.h"

#include "thread.h"
#include "buffer.h"

#include "debug_new.h"

CDatagrams Datagrams;

CDatagrams::CDatagrams()
{
    m_nUploadLimit = 32768; // TODO it

	m_pRecvBuffer = new CBuffer();
	m_pHostAddress = new QHostAddress();
	m_nSequence = 0;

	m_bActive = false;

	m_pSocket = 0;
	m_tSender = 0;
	m_bFirewalled = true;

	m_nInFrags = 0;
	m_nOutFrags = 0;

}
CDatagrams::~CDatagrams()
{
	if(m_bActive)
	{
		Disconnect();
	}

	if(m_pSocket)
	{
		delete m_pSocket;
	}

	if(m_tSender)
	{
		delete m_tSender;
	}

	if(m_pRecvBuffer)
	{
		delete m_pRecvBuffer;
	}
	if(m_pHostAddress)
	{
		delete m_pHostAddress;
	}
}

void CDatagrams::Listen()
{
	QMutexLocker l(&m_pSection);

	if(m_bActive)
	{
		systemLog.postLog(LogSeverity::Debug, QString("CDatagrams::Listen - already listening"));
		return;
	}

	Q_ASSERT(m_pSocket == 0);

	m_pSocket = new QUdpSocket(this);

	CEndPoint addr = Network.GetLocalAddress();
	if(m_pSocket->bind(addr.port()))
	{
		systemLog.postLog(LogSeverity::Debug, QString("Datagrams listening on %1").arg(m_pSocket->localPort()));
		m_nDiscarded = 0;

		for(int i = 0; i < quazaaSettings.Gnutella2.UdpBuffers; i++)
		{
			m_FreeBuffer.append(new CBuffer(1024));
		}

		for(int i = 0; i < quazaaSettings.Gnutella2.UdpInFrames; i++)
		{
			m_FreeDGIn.append(new DatagramIn);
		}

		for(int i = 0; i < quazaaSettings.Gnutella2.UdpOutFrames; i++)
		{
			m_FreeDGOut.append(new DatagramOut);
		}

		connect(this, SIGNAL(SendQueueUpdated()), this, SLOT(FlushSendCache()), Qt::QueuedConnection);
		connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(OnDatagram()), Qt::QueuedConnection);

		m_bActive = true;
	}
	else
	{
		systemLog.postLog(LogSeverity::Debug, QString("Can't bind UDP socket! UDP communication disabled!"));
		Disconnect();
	}

	m_bFirewalled = true;

}

void CDatagrams::Disconnect()
{
	QMutexLocker l(&m_pSection);

	m_bActive = false;

	if(m_pSocket)
	{
		m_pSocket->close();
		delete m_pSocket;
		m_pSocket = 0;
	}

	disconnect(SIGNAL(SendQueueUpdated()));

    while(!m_AckCache.isEmpty())
    {
        QPair<CEndPoint, char*> oAck = m_AckCache.takeFirst();
        delete [] oAck.second;
    }

	while(!m_SendCache.isEmpty())
	{
		Remove(m_SendCache.first());
	}

	while(!m_RecvCacheTime.isEmpty())
	{
		Remove(m_RecvCacheTime.first());
	}

	while(!m_FreeDGIn.isEmpty())
	{
		delete m_FreeDGIn.takeFirst();
	}

	while(!m_FreeDGOut.isEmpty())
	{
		delete m_FreeDGOut.takeFirst();
	}

	while(!m_FreeBuffer.isEmpty())
	{
		delete m_FreeBuffer.takeFirst();
	}
}

void CDatagrams::OnDatagram()
{
	if(!m_bActive)
	{
		return;
	}

	//while(m_pSocket->hasPendingDatagrams())
	{
		qint64 nSize = m_pSocket->pendingDatagramSize();
		m_pRecvBuffer->resize(nSize);
		qint64 nReadSize = m_pSocket->readDatagram(m_pRecvBuffer->data(), nSize, m_pHostAddress, &m_nPort);

		m_mInput.Add(nReadSize);

		if(nReadSize < 8)
		{
		//	continue;
			return;
		}

		m_nInFrags++;

		GND_HEADER* pHeader = (GND_HEADER*)m_pRecvBuffer->data();
		if(strncmp((char*)&pHeader->szTag, "GND", 3) == 0 && pHeader->nPart > 0 && (pHeader->nCount == 0 || pHeader->nPart <= pHeader->nCount))
		{
			if(pHeader->nCount == 0)
			{
				// ACK
				OnAcknowledgeGND();
			}
			else
			{
				// DG
				OnReceiveGND();
			}
		}
	}
}

void CDatagrams::OnReceiveGND()
{

	GND_HEADER* pHeader = (GND_HEADER*)m_pRecvBuffer->data();
	QHostAddress nIp = *m_pHostAddress;
	quint32 nSeq = ((pHeader->nSequence << 16) & 0xFFFF0000) + (m_nPort & 0x0000FFFF);

#ifdef DEBUG_UDP
	systemLog.postLog(LogSeverity::Debug, "Received GND from %s:%u nSequence = %u nPart = %u nCount = %u", m_pHostAddress->toString().toLocal8Bit().constData(), m_nPort, pHeader->nSequence, pHeader->nPart, pHeader->nCount);
#endif

	DatagramIn* pDG = 0;

	if(m_RecvCache.contains(nIp) && m_RecvCache[nIp].contains(nSeq))
	{
		pDG = m_RecvCache[nIp][nSeq];

		// Zeby dac szanse wiekszym pakietom ;)
		if(pDG->m_nLeft)
		{
			pDG->m_tStarted = time(0);
		}
	}
	else
	{
		QMutexLocker l(&m_pSection);

		if(!m_FreeDGIn.isEmpty())
		{
			pDG = m_FreeDGIn.takeFirst();
		}
		else
		{
			if(m_FreeDGIn.isEmpty())
			{
				RemoveOldIn(true);
				if(m_FreeDGIn.isEmpty())
				{
#ifdef DEBUG_UDP
					systemLog.postLog(LogSeverity::Debug, QString("UDP in frames exhausted"));
#endif
					m_nDiscarded++;
					return;
				}
			}

			pDG = m_FreeDGIn.takeFirst();
		}

		if(m_FreeBuffer.size() < pHeader->nCount)
		{
			m_nDiscarded++;
			m_FreeDGIn.append(pDG);
			RemoveOldIn(false);
			return;
		}

		pDG->Create(CEndPoint(*m_pHostAddress, m_nPort), pHeader->nFlags, pHeader->nSequence, pHeader->nCount);

		for(int i = 0; i < pHeader->nCount; i++)
		{
			Q_ASSERT(pDG->m_pBuffer[i] == 0);
			pDG->m_pBuffer[i] = m_FreeBuffer.takeFirst();
		}

		m_RecvCache[nIp][nSeq] = pDG;
		m_RecvCacheTime.prepend(pDG);
	}

    // It is here, in case if we did not have free datagrams
    // ACK = I've received a datagram, and if you have received and rejected it, do not send ACK-a
	if(pHeader->nFlags & 0x02)
	{
        GND_HEADER* pAck = new GND_HEADER;

        memcpy(pAck, pHeader, sizeof(GND_HEADER));
        pAck->nCount = 0;
        pAck->nFlags = 0;

#ifdef DEBUG_UDP
		systemLog.postLog(LogSeverity::Debug, "Sending UDP ACK to %s:%u", m_pHostAddress->toString().toLocal8Bit().constData(), m_nPort);
#endif

        //m_pSocket->writeDatagram((char*)&oAck, sizeof(GND_HEADER), *m_pHostAddress, m_nPort);
        //m_mOutput.Add(sizeof(GND_HEADER));
        m_AckCache.append(qMakePair(CEndPoint(*m_pHostAddress, m_nPort), reinterpret_cast<char*>(pAck)));
        if( m_AckCache.count() == 1 )
            QMetaObject::invokeMethod(this, "FlushSendCache", Qt::QueuedConnection);
	}

	if(pDG->Add(pHeader->nPart, m_pRecvBuffer->data() + sizeof(GND_HEADER), m_pRecvBuffer->size() - sizeof(GND_HEADER)))
	{

		G2Packet* pPacket = 0;
		try
		{
			CEndPoint addr(*m_pHostAddress, m_nPort);
			pPacket = pDG->ToG2Packet();
			if(pPacket)
			{
				OnPacket(addr, pPacket);
			}

		}
		catch(...)
		{

		}
		if(pPacket)
		{
			pPacket->Release();
		}

		m_pSection.lock();
		Remove(pDG, true);
		m_pSection.unlock();
	}

}

void CDatagrams::OnAcknowledgeGND()
{
	GND_HEADER* pHeader = (GND_HEADER*)m_pRecvBuffer->data();

#ifdef DEBUG_UDP
	systemLog.postLog(LogSeverity::Debug, "UDP received GND ACK from %s seq %u part %u", m_pHostAddress->toString().toLocal8Bit().constData(), pHeader->nSequence, pHeader->nPart);
#endif

	if(!m_SendCacheMap.contains(pHeader->nSequence))
	{
		return;
	}

	DatagramOut* pDG = m_SendCacheMap.value(pHeader->nSequence);

	if(pDG->Acknowledge(pHeader->nPart))
	{
		Remove(pDG);
	}
}

void CDatagrams::Remove(DatagramIn* pDG, bool bReclaim)
{
	for(int i = 0; i < pDG->m_nCount; i++)
	{
		if(pDG->m_pBuffer[i])
		{
			m_FreeBuffer.append(pDG->m_pBuffer[i]);
			pDG->m_pBuffer[i]->clear();
			pDG->m_pBuffer[i] = 0;
		}
	}

	if(bReclaim)
	{
		return;
	}

	if(m_RecvCache.contains(pDG->m_oAddress))
	{
		quint32 nSeq = ((pDG->m_nSequence << 16) & 0xFFFF0000) + (pDG->m_oAddress.port() & 0x0000FFFF);
		Q_ASSERT(pDG == m_RecvCache[pDG->m_oAddress][nSeq]);
		m_RecvCache[pDG->m_oAddress].remove(nSeq);
		if(m_RecvCache[pDG->m_oAddress].isEmpty())
		{
			m_RecvCache.remove(pDG->m_oAddress);
		}

		QLinkedList<DatagramIn*>::iterator itFrame = m_RecvCacheTime.end();
		while( itFrame != m_RecvCacheTime.begin() )
		{
			--itFrame;

			if( *itFrame == pDG )
			{
				m_RecvCacheTime.erase(itFrame);
				break;
			}
		}

		m_FreeDGIn.append(pDG);
	}
}

// Usuwa jeden pakiet z cache'u odbioru
void CDatagrams::RemoveOldIn(bool bForce)
{
	quint32 tNow = time(0);
	bool bRemoved = false;

	while(!m_RecvCacheTime.isEmpty() && (tNow - m_RecvCacheTime.back()->m_tStarted > quazaaSettings.Gnutella2.UdpInExpire || m_RecvCacheTime.back()->m_nLeft == 0))
	{
		Remove(m_RecvCacheTime.back());
		bRemoved = true;
	}

	if(bForce && !bRemoved && !m_RecvCacheTime.isEmpty())
	{
		for(QLinkedList<DatagramIn*>::iterator itPacket = m_RecvCacheTime.begin(); itPacket != m_RecvCacheTime.end(); ++itPacket)
		{
			if((*itPacket)->m_nLeft == 0)
			{
				Remove(*itPacket);
				break;
			}
		}
	}
}

void CDatagrams::Remove(DatagramOut* pDG)
{
	m_SendCacheMap.remove(pDG->m_nSequence);

	QLinkedList<DatagramOut*>::iterator itFrame = m_SendCache.end();
	while( itFrame != m_SendCache.begin() )
	{
		--itFrame;

		if( *itFrame == pDG )
		{
			m_SendCache.erase(itFrame);
			break;
		}
	}

	m_FreeDGOut.append(pDG);

	if(pDG->m_pBuffer)
	{
		m_FreeBuffer.append(pDG->m_pBuffer);
		pDG->m_pBuffer->clear();
		pDG->m_pBuffer = 0;
	}
}

void CDatagrams::FlushSendCache()
{
	QMutexLocker l(&m_pSection);

	__FlushSendCache();
}

void CDatagrams::__FlushSendCache()
{
	if(!m_bActive)
	{
		return;
	}

	//QMutexLocker l(&m_pSection);
	ASSUME_LOCK(Datagrams.m_pSection);

	quint32 tNow = time(0);

	qint64 nToWrite = qint64(m_nUploadLimit) - qint64(m_mOutput.Usage());

    static TCPBandwidthMeter meter;

    // Maybe make it dynamic? So bad routers are automatically detected and settings adjusted?
    qint64 nMaxPPS = quazaaSettings.Connection.UDPOutLimitPPS - meter.Usage();

    if( nMaxPPS <= 0 )
    {
        systemLog.postLog( LogSeverity::Debug, Components::Network,
		                   "UDP: PPS limit reached, ACKS: %d, Packets: %d, Average PPS: %u / %u",
		                   m_AckCache.size(), m_SendCache.size(), meter.AvgUsage(), meter.Usage() );
        return;
    }

    while( nToWrite > 0 && !m_AckCache.isEmpty() && nMaxPPS > 0)
    {
        QPair< CEndPoint, char* > oAck = m_AckCache.takeFirst();
        m_pSocket->writeDatagram(oAck.second, sizeof(GND_HEADER), oAck.first, oAck.first.port());
        m_mOutput.Add(sizeof(GND_HEADER));
        nToWrite -= sizeof(GND_HEADER);
		delete (GND_HEADER*)oAck.second;
        nMaxPPS--;
        meter.Add(1);
    }

	QHostAddress nLastHost;

	// it can write slightly more than limit allows... that's ok
    while(nToWrite > 0 && !m_SendCache.isEmpty() && nMaxPPS > 0)
	{
		bool bSent = false;

		char* pPacket;
		quint32 nPacket;

		for(QLinkedList<DatagramOut*>::iterator itPacket = m_SendCache.begin(); itPacket != m_SendCache.end(); ++itPacket)
		{
			DatagramOut* pDG = *itPacket;

			if(pDG->m_oAddress == nLastHost)
			{
				continue;
			}

			// TODO: sprawdzenie UDP na firewallu - mog? by? 3 stany udp
			if(pDG->GetPacket(tNow, &pPacket, &nPacket, pDG->m_bAck && m_nInFrags > 0))
			{
#ifdef DEBUG_UDP
				systemLog.postLog(LogSeverity::Debug, "UDP sending to %s seq %u part %u count %u", pDG->m_oAddress.toString().toLocal8Bit().constData(), pDG->m_nSequence, ((GND_HEADER*)pPacket)->nPart, pDG->m_nCount);
#endif

				m_pSocket->writeDatagram(pPacket, nPacket, pDG->m_oAddress, pDG->m_oAddress.port());
				m_nOutFrags++;

				nLastHost = pDG->m_oAddress;

				if(nToWrite >= nPacket)
				{
					nToWrite -= nPacket;
				}
				else
				{
					nToWrite = 0;
				}

				m_mOutput.Add(nPacket);

				if(!pDG->m_bAck)
				{
					Remove(pDG);
				}

                nMaxPPS--;
                meter.Add(1);

				bSent = true;

				break;
			}
		}

		if(m_SendCache.isEmpty() || !bSent)
		{
			break;
		}
	}

	while(!m_SendCache.isEmpty() && tNow - m_SendCache.back()->m_tSent > quazaaSettings.Gnutella2.UdpOutExpire)
	{
		Remove(m_SendCache.back());
	}

}

void CDatagrams::SendPacket(CEndPoint& oAddr, G2Packet* pPacket, bool bAck, DatagramWatcher* pWatcher, void* pParam)
{
	if(!m_bActive)
	{
		return;
	}

	QMutexLocker l(&m_pSection);

	Q_UNUSED(pWatcher);
	Q_UNUSED(pParam);

	if(m_FreeDGOut.isEmpty())
	{
		systemLog.postLog(LogSeverity::Debug, QString("UDP out frames exhausted"));

		if( !bAck ) // if caller does not want ACK, drop the packet here
			return; // TODO: needs more testing

		Remove(m_SendCache.last());

	}

	if(m_FreeBuffer.isEmpty())
	{
		RemoveOldIn(false);

		if(m_FreeBuffer.isEmpty())
		{
			systemLog.postLog(LogSeverity::Debug, QString("UDP out discarded, out of buffers"));
			//qDebug() << "UDP out discarded, out of buffers";
			return;
		}
	}

	DatagramOut* pDG = m_FreeDGOut.takeFirst();
	pDG->Create(oAddr, pPacket, m_nSequence++, m_FreeBuffer.takeFirst(), (bAck && (m_nInFrags > 0))); // to prevent net spam when unable to receive datagrams

	m_SendCache.prepend(pDG);
	m_SendCacheMap[pDG->m_nSequence] = pDG;

	// TODO: Powiadomienia do obiektow nasluchujacych, jesli podano

#ifdef DEBUG_UDP
	systemLog.postLog(LogSeverity::Debug, "UDP queued for %s seq %u parts %u", oAddr.toString().toLocal8Bit().constData(), pDG->m_nSequence, pDG->m_nCount);
#endif

	//emit SendQueueUpdated();
	__FlushSendCache();

}

void CDatagrams::OnPacket(CEndPoint addr, G2Packet* pPacket)
{
	try
	{
		if(pPacket->IsType("PI"))
		{
			OnPing(addr, pPacket);
		}
		else if(pPacket->IsType("PO"))
		{
			OnPong(addr, pPacket);
		}
		else if(pPacket->IsType("CRAWLR"))
		{
			OnCRAWLR(addr, pPacket);
		}
		else if(pPacket->IsType("QKR"))
		{
			OnQKR(addr, pPacket);
		}
		else if(pPacket->IsType("QKA"))
		{
			OnQKA(addr, pPacket);
		}
		else if(pPacket->IsType("QA"))
		{
			OnQA(addr, pPacket);
		}
		else if(pPacket->IsType("QH2"))
		{
			OnQH2(addr, pPacket);
		}
		else if(pPacket->IsType("Q2"))
		{
			OnQuery(addr, pPacket);
		}
		else
		{
			//systemLog.postLog(LogSeverity::Debug, QString("G2 UDP recieved unknown packet %1").arg(pPacket->GetType()));
			//qDebug() << "UDP RECEIVED unknown packet " << pPacket->GetType();
		}
	}
	catch(...)
	{
		systemLog.postLog(LogSeverity::Debug, QString("malformed packet"));
		//qDebug() << "malformed packet";
	}
}

void CDatagrams::OnPing(CEndPoint& addr, G2Packet* pPacket)
{
	Q_UNUSED(pPacket);

	G2Packet* pNew = G2Packet::New("PO", false);
	SendPacket(addr, pNew, false);
	pNew->Release();
}

void CDatagrams::OnPong(CEndPoint& addr, G2Packet* pPacket)
{
	if(pPacket->m_bCompound)
	{
		char szType[9];
		quint32 nLength = 0, nNext = 0;

		while(pPacket->ReadPacket(&szType[0], nLength))
		{
			nNext = pPacket->m_nPosition + nLength;

			if(strcmp("RELAY", szType) == 0)
			{
				if(!Network.IsConnectedTo(addr))
				{
					m_bFirewalled = false;
				}
			}

			pPacket->m_nPosition = nNext;
		}
	}
}
void CDatagrams::OnCRAWLR(CEndPoint& addr, G2Packet* pPacket)
{
	QMutexLocker l2(&Neighbours.m_pSection);

//	bool bRLeaf = false;
//	bool bRNick = false;
//	bool bRGPS = false;
//	bool bRExt = false;

	if(!pPacket->m_bCompound)
	{
		return;
	}

	char szType[9];
	quint32 nLength = 0, nNext = 0;

	while(pPacket->ReadPacket(&szType[0], nLength))
	{
		nNext = pPacket->m_nPosition + nLength;

		if(strcmp("RLEAF", szType) == 0)
		{
//			bRLeaf = true;
		}
		else if(strcmp("RNAME", szType) == 0)
		{
//			bRNick = true;
		}
		else if(strcmp("RGPS", szType) == 0)
		{
//			bRGPS = true;
		}
		else if(strcmp("REXT", szType) == 0)
		{
//			bRExt = true;
		}

		pPacket->m_nPosition = nNext;
	}

	G2Packet* pCA = G2Packet::New("CRAWLA", true);

	G2Packet* pTmp = G2Packet::New("SELF", true);
	if(Neighbours.IsG2Hub())
	{
		pTmp->WritePacket("HUB", 0);
	}
	else
	{
		pTmp->WritePacket("LEAF", 0);
	}
	pTmp->WritePacket("NA", ((Network.m_oAddress.protocol() == 0) ? 6 : 18))->WriteHostAddress(&Network.m_oAddress);
	pTmp->WritePacket("CV", QuazaaGlobals::USER_AGENT_STRING().toUtf8().size())->WriteString(QuazaaGlobals::USER_AGENT_STRING(), false);
	pTmp->WritePacket("V", 4)->WriteString(QuazaaGlobals::VENDOR_CODE(), false);;
	quint16 nLeaves = Neighbours.m_nLeavesConnectedG2;
	pTmp->WritePacket("HS", 2)->WriteIntLE(nLeaves);
	if(!quazaaSettings.Profile.GnutellaScreenName.isEmpty())
	{
		pTmp->WritePacket("NAME", quazaaSettings.Profile.GnutellaScreenName.left(255).toUtf8().size())->WriteString(quazaaSettings.Profile.GnutellaScreenName.left(255));
	}

	pCA->WritePacket(pTmp);
	pTmp->Release();

	for(QList<CNeighbour*>::iterator itNode = Neighbours.begin(); itNode != Neighbours.end(); ++itNode)
	{
		if((*itNode)->m_nProtocol != dpG2)
		{
			continue;
		}

		CG2Node* pNode = (CG2Node*) * itNode;
		if(pNode->m_nState == nsConnected)
		{
			if(pNode->m_nType == G2_HUB)
			{
				G2Packet* pNH = G2Packet::New("NH");
				pNH->WritePacket("NA", ((pNode->m_oAddress.protocol() == 0) ? 6 : 18))->WriteHostAddress(&pNode->m_oAddress);
				pNH->WritePacket("HS", 2)->WriteIntLE(pNode->m_nLeafCount);
				pCA->WritePacket(pNH);
				pNH->Release();
			}
			else if(pNode->m_nType == G2_LEAF)
			{
				G2Packet* pNL = G2Packet::New("NL");
				pNL->WritePacket("NA", ((pNode->m_oAddress.protocol() == 0) ? 6 : 18))->WriteHostAddress(&pNode->m_oAddress);
				pCA->WritePacket(pNL);
				pNL->Release();
			}
		}
	}

	SendPacket(addr, pCA, true);

	pCA->Release();
}
void CDatagrams::OnQKR(CEndPoint& addr, G2Packet* pPacket)
{
	if(!Neighbours.IsG2Hub())
	{
		return;
	}

	CEndPoint oRequestedAddress = addr;
	CEndPoint oSendingAddress = addr;

	if(pPacket->m_bCompound)
	{
		char szType[9];
		quint32 nLength = 0, nNext = 0;

		while(pPacket->ReadPacket(&szType[0], nLength))
		{
			nNext = pPacket->m_nPosition + nLength;

			if(strcmp("SNA", szType) == 0 && nLength >= 4)
			{
				if(nLength >= 16)
				{
					Q_IPV6ADDR ip;
					pPacket->Read(&ip, 16);
					oSendingAddress.setAddress(ip);
				}
				else
				{
					quint32	nIp = pPacket->ReadIntBE<quint32>();
					oSendingAddress.setAddress(nIp);
				}
			}
			else if(strcmp("RNA", szType) == 0 && nLength >= 6)
			{
				if(nLength >= 18)
				{
					pPacket->ReadHostAddress(&oRequestedAddress, false);
				}
				else
				{
					pPacket->ReadHostAddress(&oRequestedAddress);
				}
			}
			pPacket->m_nPosition = nNext;
		}
	}

	if(!oRequestedAddress.port() || oRequestedAddress.isFirewalled())
	{
		return;
	}

	G2Packet* pAns = G2Packet::New("QKA", true);
	quint32 nKey = QueryKeys.Create(oRequestedAddress);
	pAns->WritePacket("QK", 4);
	pAns->WriteIntLE<quint32>(nKey);
	G2Packet* pSNA = G2Packet::New("SNA");
	pSNA->WriteHostAddress(&oSendingAddress);
	pAns->WritePacket(pSNA);
	pSNA->Release();

	SendPacket(oRequestedAddress, pAns, false);
	pAns->Release();

#if LOG_QUERY_HANDLING
	systemLog.postLog(LogSeverity::Debug, "Node %s asked for a query key (0x%08x) for node %s", qPrintable(addr.toStringWithPort()), nKey, qPrintable(oRequestedAddress.toStringWithPort()));
#endif // LOG_QUERY_HANDLING
}

void CDatagrams::OnQKA(CEndPoint& addr, G2Packet* pPacket)
{
	if ( !pPacket->m_bCompound )
	{
		return;
	}

	quint32 nKey = 0;
	QHostAddress nKeyHost;

	char szType[9];
	quint32 nLength = 0, nNext = 0;

	while(pPacket->ReadPacket(&szType[0], nLength))
	{
		nNext = pPacket->m_nPosition + nLength;

		if(strcmp("QK", szType) == 0 && nLength >= 4)
		{
			nKey = pPacket->ReadIntLE<quint32>();
		}
		else if(strcmp("SNA", szType) == 0 && nLength >= 4)
		{
			if(nLength >= 16)
			{
				Q_IPV6ADDR ip;
				pPacket->Read(&ip, 16);
				nKeyHost.setAddress(ip);
			}
			else
			{
				quint32 nIp = pPacket->ReadIntBE<quint32>();
				nKeyHost.setAddress(nIp);
			}
		}
		pPacket->m_nPosition = nNext;
	}

	if ( nKey ) // null QK means a hub does not want to be queried or just downgraded
	{
		hostCache.m_pSection.lock();
		CHostCacheHost* pCache = hostCache.addSync( addr, common::getTNowUTC(), false );
		if ( pCache )
		{
			pCache->setKey( nKey );
		}
		hostCache.m_pSection.unlock();
	}

#if LOG_QUERY_HANDLING
	systemLog.postLog(LogSeverity::Debug, QString("Got a query key for %1 = 0x%2").arg(addr.toString().toLocal8Bit().constData()).arg(nKey));
	//qDebug("Got a query key for %s = 0x%x", addr.toString().toLocal8Bit().constData(), nKey);
#endif // LOG_QUERY_HANDLING

	if(Neighbours.IsG2Hub() && !nKeyHost.isNull() && nKeyHost != ((QHostAddress)Network.m_oAddress))
	{
		G2Packet* pQNA = G2Packet::New("QNA");
		pQNA->WriteHostAddress(&addr);
		pPacket->PrependPacket(pQNA);

		Neighbours.m_pSection.lock();
		CNeighbour* pNode = Neighbours.Find(nKeyHost, dpG2);
		if( pNode )
		{
			((CG2Node*)pNode)->SendPacket(pPacket, true, false);
		}
		Neighbours.m_pSection.unlock();
	}

}
void CDatagrams::OnQA(CEndPoint& addr, G2Packet* pPacket)
{
	hostCache.m_pSection.lock();

	CHostCacheHost* pHost = hostCache.addSync( addr, common::getTNowUTC(), false );
	if ( pHost )
	{
		pHost->setAck( 0 );
	}

	hostCache.m_pSection.unlock();

	QUuid oGuid;

	// Hubs are only supposed to route UDP /QA - we'll drop it if we're in leaf mode
	if ( SearchManager.OnQueryAcknowledge( pPacket, addr, oGuid ) && Neighbours.IsG2Hub() )
	{
		// Add from address
		G2Packet* pFR = G2Packet::New( "FR" );
		pFR->WriteHostAddress( &addr );
		pPacket->AddOrReplaceChild( "FR", pFR );

		Network.m_pSection.lock();
		Network.RoutePacket( oGuid, pPacket, true, false );
		Network.m_pSection.unlock();
	}
}

void CDatagrams::OnQH2(CEndPoint& addr, G2Packet* pPacket)
{
	if(!pPacket->m_bCompound)
	{
		return;
	}

	QueryHitInfo* pInfo = CQueryHit::ReadInfo(pPacket, &addr);

	if(pInfo)
	{
		if(SearchManager.OnQueryHit(pPacket, pInfo) && Neighbours.IsG2Hub() && pInfo->m_nHops < 7)
		{
			pPacket->m_pBuffer[pPacket->m_nLength - 17]++;

			Network.m_pSection.lock();

			if(pInfo->m_oNodeAddress == pInfo->m_oSenderAddress)
			{
				// hits node address matches sender address
				Network.m_oRoutingTable.Add(pInfo->m_oNodeGUID, pInfo->m_oSenderAddress);
			}
			else if(!pInfo->m_lNeighbouringHubs.isEmpty())
			{
				// hits address does not match sender address (probably forwarded by a hub)
				// and there are neighbouring hubs available, use them instead (sender address can be used instead...)
				Network.m_oRoutingTable.Add(pInfo->m_oNodeGUID, pInfo->m_lNeighbouringHubs[0], false);
			}

			Network.RoutePacket(pInfo->m_oGUID, pPacket, true);

			Network.m_pSection.unlock();
		}
	}
}

void CDatagrams::OnQuery(CEndPoint &addr, G2Packet *pPacket)
{
	CQueryPtr pQuery = CQuery::FromPacket(pPacket, &addr);

	if(pQuery.isNull())
	{
#if LOG_QUERY_HANDLING
		qDebug() << "Received malformed query from" << qPrintable(addr.toStringWithPort());
#endif // LOG_QUERY_HANDLING
		return;
	}

	if( !Neighbours.IsG2Hub() )
	{
		// Stop receiving queries from others
		// We are here because we just downgraded to leaf mode
		// Shareaza should not retry with QK == 0
		// TODO: test this
#if LOG_QUERY_HANDLING
		systemLog.postLog(LogSeverity::Debug, "Sending null query key to %s because we're not a hub.", qPrintable(addr.toStringWithPort()));
#endif // LOG_QUERY_HANDLING

		G2Packet* pQKA = G2Packet::New("QKA", true);
		pQKA->WritePacket("QK", 4)->WriteIntLE<quint32>(0);

		if( addr != pQuery->m_oEndpoint )
		{
			pQKA->WritePacket("SNA", (pQuery->m_oEndpoint.protocol() == QAbstractSocket::IPv6Protocol ? 18 : 6))->WriteHostAddress(&pQuery->m_oEndpoint);
		}

		SendPacket(pQuery->m_oEndpoint, pQKA);
		pQKA->Release();

		return;
	}

	if(!QueryKeys.Check(pQuery->m_oEndpoint, pQuery->m_nQueryKey))
	{
#if LOG_QUERY_HANDLING
		systemLog.postLog(LogSeverity::Debug, "Issuing query key correction for %s.", qPrintable(addr.toStringWithPort()));
#endif // LOG_QUERY_HANDLING

		G2Packet* pQKA = G2Packet::New("QKA", true);
		pQKA->WritePacket("QK", 4)->WriteIntLE<quint32>(QueryKeys.Create(pQuery->m_oEndpoint));

		if( addr != pQuery->m_oEndpoint )
		{
			pQKA->WritePacket("SNA", (pQuery->m_oEndpoint.protocol() == QAbstractSocket::IPv6Protocol ? 18 : 6))->WriteHostAddress(&pQuery->m_oEndpoint);
		}
		SendPacket(addr, pPacket);
		pQKA->Release();

		return;
	}

	if( !Network.m_oRoutingTable.Add(pQuery->m_oGUID, pQuery->m_oEndpoint) )
	{
#if LOG_QUERY_HANDLING
		qDebug() << "Query already processed, ignoring";
#endif // LOG_QUERY_HANDLING
		G2Packet* pQA = Neighbours.CreateQueryAck(pQuery->m_oGUID, false, 0, false);
		SendPacket(pQuery->m_oEndpoint, pQA, true);
		pQA->Release();
		return;
	}

#if LOG_QUERY_HANDLING
	qDebug() << "Processing query from: " << qPrintable(addr.toStringWithPort());
#endif // LOG_QUERY_HANDLING

	// just in case
	if( pQuery->m_oEndpoint == Network.m_oAddress )
	{
		systemLog.postLog( LogSeverity::Error, Components::Network,
		                   "Q2 received via UDP and return address points to us, changing return address to source %s",
		                   qPrintable( addr.toStringWithPort() ) );
		G2Packet* pUDP = G2Packet::New("UDP");
		pUDP->WriteHostAddress(&addr);
		pUDP->WriteIntLE<quint32>(0);
		pPacket->AddOrReplaceChild("UDP", pUDP);
	}

	Neighbours.m_pSection.lock();
	G2Packet* pQA = Neighbours.CreateQueryAck(pQuery->m_oGUID);
	SendPacket(pQuery->m_oEndpoint, pQA, true);
	pQA->Release();

	Neighbours.RouteQuery(pQuery, pPacket);
	Neighbours.m_pSection.unlock();

	// local search
}

