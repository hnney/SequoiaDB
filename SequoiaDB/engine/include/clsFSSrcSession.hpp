/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = clsFSSrcSession.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSFSSRCSESSION_HPP_
#define CLSFSSRCSESSION_HPP_

#include "clsDef.hpp"
#include "clsFSDef.hpp"
#include "pmdAsyncSession.hpp"
#include "dms.hpp"
#include "dpsLogDef.hpp"
#include "dpsMessageBlock.hpp"
#include "rtnLobFetcher.hpp"
#include "../bson/bsonobj.h"
#include <map>

using namespace std ;
using namespace bson ;

namespace engine
{
   class _netRouteAgent ;
   class _SDB_DMSCB ;
   class _dmsStorageUnit ;
   class _dpsLogWrapper ;
   class _SDB_RTNCB ;
   class _MsgClsFSNotify ;
   class _clsReplicateSet ;
   class _monIndex ;
   class _clsCatalogAgent ;
   class _clsSplitTask ;
   class _rtnContextData ;

   class _clsDataSrcBaseSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsDataSrcBaseSession ( UINT64 sessionID,
                                  _netRouteAgent *agent ) ;
         virtual ~_clsDataSrcBaseSession () ;

         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;

      public:
         virtual INT32 notifyLSN ( UINT32 suLID, UINT32 clLID,
                                   dmsExtentID extLID,
                                   const DPS_LSN_OFFSET &offset ) = 0 ;

      protected:
         virtual BOOLEAN   _isReady () = 0 ;
         virtual const CHAR* _onObjFilter ( const CHAR* inBuff, INT32 inSize,
                                            INT32 &outSize ) = 0 ;
         virtual INT32     _onFSMeta ( const CHAR *clFullName ) = 0 ;
         virtual INT32     _scanType () const = 0 ;
         virtual BOOLEAN   _canSwitchWhenSyncLog() = 0 ;

         virtual void      _reset () ;
         virtual INT32     _onLobFilter( const bson::OID &oid,
                                         UINT32 sequence,
                                         BOOLEAN &need2Send ) = 0 ;

      protected:
         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

      protected:
         INT32 handleFSMeta( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleFSIndex( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleFSNotify( NET_HANDLE handle, MsgHeader* header ) ;

      protected:
         void              _resend( const NET_HANDLE &handle,
                                    const _MsgClsFSNotify *req ) ;
         void              _eraseDefaultIndex() ;
         BOOLEAN           _existIndex( const CHAR *indexName ) ;
         INT32             _openContext( CHAR *cs, CHAR *collection ) ;
         void              _constructIndex( BSONObj &obj ) ;
         INT32             _constructFullNames( BSONObj &obj ) ;
         void              _constructMeta( BSONObj &obj, const CHAR *cs,
                                           const CHAR *collection,
                                           _dmsStorageUnit *su ) ;
         INT32             _getCSName( const BSONObj &obj, CHAR *cs, UINT32 len ) ;
         INT32             _getCollection( const BSONObj &obj, CHAR *collection,
                                           UINT32 len ) ;
         INT32             _getRangeKey( const BSONObj &obj ) ;
         void              _disconnect() ;

         INT32             _syncLog( const NET_HANDLE &handle,
                                     SINT64 packet,
                                     const MsgRouteID &routeID,
                                     UINT32 TID, UINT64 requestID ) ;
         INT32             _syncRecord( const NET_HANDLE &handle,
                                        SINT64 packet,
                                        const MsgRouteID &routeID,
                                        UINT32 TID, UINT64 requestID ) ;

         INT32             _syncLob( const NET_HANDLE &handle,
                                     SINT64 packet,
                                     const MsgRouteID &routeID,
                                     UINT32 TID, UINT64 requestID ) ;
      protected:
         BSONObj                          _rangeKeyObj ;
         BSONObj                          _rangeEndKeyObj ;
         vector<_monIndex>                _indexs ;
         _netRouteAgent                   *_agent ;
         DPS_LSN                          _lsn ;
         DPS_LSN_OFFSET                   _beginLSNOffset ;
         SINT64                           _contextID ;
         _rtnContextData                  *_context ;
         BOOLEAN                          _findEnd ;
         const CHAR                       *_query ;
         SINT32                           _queryLen ;
         _dpsMessageBlock                 _mb ;
         SINT64                           _packetID ;
         INT32                            _dataType ;
         BOOLEAN                          _canResend ;
         BOOLEAN                          _quit ;
         BOOLEAN                          _hasMeta ;
         INT32                            _needData ;
         BOOLEAN                          _init ;
         std::string                      _curCollecitonName ;

         _clsReplicateSet                 *_pRepl ;
         MsgHeader                        _disconnectMsg ;
         UINT32                           _timeCounter ;

         map<UINT64, UINT32>              _mapOveredCLs ;
         UINT64                           _curCollection ; // suLID+clLID
         dmsExtentID                      _curExtID ;
         BSONObj                          _curScanKeyObj ;
         deque<DPS_LSN_OFFSET>            _deqLSN ;
         ossSpinXLatch                    _LSNlatch ;
         _rtnLobFetcher                   _lobFetcher ;
   };

   class _clsFSSrcSession : public _clsDataSrcBaseSession
   {
   DECLARE_OBJ_MSG_MAP()
   public:
      _clsFSSrcSession( UINT64 sessionID,
                        _netRouteAgent *agent ) ;
      virtual ~_clsFSSrcSession() ;

   public:
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual EDU_TYPES eduType () const ;

   public:
      virtual INT32 notifyLSN ( UINT32 suLID, UINT32 clLID, dmsExtentID extLID,
                                const DPS_LSN_OFFSET &offset ) ;

   protected:
      INT32 handleBegin( NET_HANDLE handle, MsgHeader* header ) ;
      INT32 handleEnd( NET_HANDLE handle, MsgHeader* header ) ;
      INT32 handleSyncTransReq( NET_HANDLE handle, MsgHeader* header ) ;

   protected:
      virtual BOOLEAN _isReady() ;
      virtual const CHAR* _onObjFilter ( const CHAR* inBuff, INT32 inSize,
                                         INT32 &outSize ) ;
      virtual INT32 _onLobFilter( const bson::OID &oid,
                                  UINT32 sequence,
                                  BOOLEAN &need2Send ) ;
      virtual INT32   _onFSMeta ( const CHAR *clFullName ) ;
      virtual INT32   _scanType () const ;
      virtual BOOLEAN _canSwitchWhenSyncLog() ;

   private:
      _dpsMessageBlock     _lsnSearchMB ;
   } ;
   typedef class _clsFSSrcSession clsFSSrcSession ;

   class _clsSplitSrcSession : public _clsDataSrcBaseSession
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsSplitSrcSession( UINT64 sessionID, _netRouteAgent *agent ) ;
         virtual ~_clsSplitSrcSession () ;

         INT32 notifyLSN ( UINT32 suLID, UINT32 clLID, dmsExtentID extLID,
                           const DPS_LSN_OFFSET &offset ) ;

      public:
         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual EDU_TYPES eduType () const ;

      protected:
         virtual BOOLEAN _isReady() ;
         virtual const CHAR* _onObjFilter ( const CHAR* inBuff, INT32 inSize,
                                            INT32 &outSize ) ;
         virtual INT32   _onFSMeta ( const CHAR *clFullName ) ;
         virtual INT32   _scanType () const ;
         virtual BOOLEAN _canSwitchWhenSyncLog() ;
         virtual INT32   _onLobFilter( const bson::OID &oid,
                                       UINT32 sequence,
                                       BOOLEAN &need2Send ) ;

      protected:
         INT32   _genKeyObj ( const BSONObj &obj, BSONObj &keyObj ) ;
         BOOLEAN _GEThanRangeKey ( const BSONObj &keyObj ) ;
         BOOLEAN _LThanRangeEndKey( const BSONObj &keyObj ) ;
         BOOLEAN _LEThanScanObj ( const BSONObj &keyObj ) ;
         INT32   _addToFilterMB ( const CHAR *data, UINT32 size ) ;

         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

      protected:
         INT32 handleBegin( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleEnd( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleLEnd ( NET_HANDLE handle, MsgHeader* header ) ;

      protected:
         _dpsMessageBlock                 _filterMB ;
         _dpsMessageBlock                 _lsnSearchMB ;
         BSONObj                          _shardingKey ;
         _clsCatalogAgent                 *_pCatAgent ;
         EDUID                            _cleanupJobID ;

         BOOLEAN                          _hasShardingIndex ;
         BOOLEAN                          _hashShard ;
         BOOLEAN                          _hasEndRange ;
         UINT32                           _partitionBit ;

         UINT64                           _taskID ;
         UINT64                           _updateMetaTime ;
         UINT64                           _lastEndNtyTime ;
   };
}

#endif

