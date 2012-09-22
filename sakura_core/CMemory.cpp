/*!	@file
	�������o�b�t�@�N���X

	@author Norio Nakatani
	@date 1998/03/06 �V�K�쐬
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000-2001, jepro, genta
	Copyright (C) 2001, mik, misaka, Stonee, hor
	Copyright (C) 2002, Moca, sui, aroka, genta
	Copyright (C) 2003, genta, Moca, �����
	Copyright (C) 2004, Moca
	Copyright (C) 2005, Moca, D.S.Koba
	Copyright (C) 2006, rastiv

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/

#include "StdAfx.h"
#include <stdio.h>
#include <windows.h>
#include <mbstring.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>
#include "CMemory.h"
#include "etc_uty.h"
#include "CEol.h"// 2002/2/3 aroka
#include "charcode.h"
#include "CESI.h"  // 2006.12.16  rastiv
#include "my_icmp.h" // Nov. 29, 2002 genta/moca

//#ifdef _DEBUG
#include "global.h"
#include "Debug.h"
#include "CRunningTimer.h"
//#endif



#define ESC_JIS		"\x01b$B"
#define ESC_ASCII	"\x01b(B"
#define ESC_8BIT	"\x01b(I"

#define MIME_BASE64	1
#define MIME_QUOTED	2

// #define UUDECODE_CHAR(c) ((((unsigned char)c) - ((unsigned char)32)) & (unsigned char)0x3f)

/* ������� */
#define CHAR_ASCII		0	/* ASCII���� */
#define CHAR_8BITCODE	1	/* 8�r�b�g�R�[�h(���p�J�^�J�i�Ȃ�) */
#define CHAR_ZENKAKU	2	/* �S�p���� */
#define CHAR_NULL		3	/* �Ȃɂ��Ȃ� */



/*///////////////////////////////////////////////////////////////////////////
//
//	CMemory::CMemory
//	CMemory()
//
//	����
//		CMemory�N���X �R���X�g���N�^
//
///////////////////////////////////////////////////////////////////////////*/
CMemory::CMemory()
{
	m_nDataBufSize = 0;
	m_pData = NULL;
	m_nDataLen = 0;
	return;
}



/*///////////////////////////////////////////////////////////////////////////
//
//	CMemory::CMemory
//	CMemory( const char* pData, int nDataLen )
//
//	����
//	  pData		�i�[�f�[�^�A�h���X
//	  nDataLen	�i�[�f�[�^�̗L����
//
//	����
//		CMemory�N���X  �R���X�g���N�^
//
//
//	�ߒl
//		�Ȃ�
//
//	���l
//		�i�[�f�[�^�ɂ�NULL���܂ނ��Ƃ��ł���
//
///////////////////////////////////////////////////////////////////////////*/
CMemory::CMemory( const char* pData, int nDataLen )
{
	m_nDataBufSize = 0;
	m_pData = NULL;
	m_nDataLen = 0;
	SetString( pData, nDataLen );
	return;
}


CMemory::~CMemory()
{
	Empty();
	return;
}




const CMemory& CMemory::operator = ( char cChar )
{
	char pszChar[2];
	pszChar[0] = cChar;
	pszChar[1] = '\0';
	SetString( pszChar, 1 );
	return *this;
}


//const CMemory& CMemory::operator = ( const char* pszStr )
//{
//	SetData( pszStr, strlen( pszStr ) );
//	return *this;
//}

/******
const CMemory& CMemory::operator=( const char* pData, int nDataLen )
{
	SetData( pData, nDataLen );
	return *this;
}
*******/

const CMemory& CMemory::operator = ( const CMemory& cMemory )
{
	if( this != &cMemory ){
		SetNativeData( (CMemory*)&(cMemory) );
	}
	return *this;
}

const CMemory& CMemory::operator += ( const CMemory& cMemory )
{
	int			nDataLen;
	const char*	pData;
	if( this == &cMemory ){
		CMemory cm = cMemory;
		pData = cm.GetStringPtr( &nDataLen );
		AllocStringBuffer( m_nDataLen + nDataLen );
		AddData( pData, nDataLen );
	}else{
		pData = cMemory.GetStringPtr( &nDataLen );
		AllocStringBuffer( m_nDataLen + nDataLen );
		AddData( (const char*)pData, nDataLen );
	}
	return *this;
}


const CMemory& CMemory::operator += ( char ch )
{
	char szChar[2];
	szChar[0] = ch;
	szChar[1] = '\0';
	AllocStringBuffer( m_nDataLen + sizeof( ch ) );
	AddData( szChar, sizeof( ch ) );
	return *this;
}



//CMemory::operator char*() const
//{
//	return (char*)m_pData;
//}
//CMemory::operator const char*() const
//{
//	return (const char*)m_pData;
//}
//CMemory::operator unsigned char*() const
//{
//	return (unsigned char*)m_pData;
//}
//CMemory::operator const unsigned char*() const
//{
//	return (const unsigned char*)m_pData;
//}
//
//CMemory::operator void*() const
//{
//	return (void*)m_pData;
//}
//CMemory::operator const void*() const
//{
//	return (const void*)m_pData;
//}


/*
||  GetAt()�Ɠ��@�\
*/
const char CMemory::operator[](int nIndex) const
{
	if( nIndex < m_nDataLen ){
		return m_pData[nIndex];
	}else{
		return 0;
	}
}








/*
|| �o�b�t�@�̍Ō�Ƀf�[�^��ǉ�����iprotect�����o
*/
void CMemory::AddData( const char* pData, int nDataLen )
{
	if( NULL == m_pData ){
		return;
	}
	memcpy( &m_pData[m_nDataLen], pData, nDataLen );
	m_nDataLen += nDataLen;
	m_pData[m_nDataLen] = '\0';
	return;
}

/*!
	@brief �g���� SJIS��JIS�ϊ�

	SJIS�R�[�h��JIS�ɕϊ�����D���̍ہCJIS�ɑΉ��̈�̂Ȃ�IBM�g��������
	NEC�I��IBM�g�������ɕϊ�����D

	Shift_JIS fa40�`fc4b �͈̔͂̕����� 8754�`879a �܂��� ed40�`eefc ��
	�U�݂��镶���ɕϊ����ꂽ��ɁCJIS�ɕϊ�����܂��D
	
	@param pszSrc [in] �ϊ����镶����ւ̃|�C���^ (Shift JIS)
	
	@author ����
	@date 2002.10.03 1�����݈̂����C�ϊ��܂ōs���悤�ɕύX genta
*/
unsigned short CMemory::_mbcjmstojis_ex( unsigned char* pszSrc )
{
	unsigned int	tmpw;	/* �� int �� 16 bit �ȏ�ł��鎖�����҂��Ă��܂��B */
	
	if(	_IS_SJIS_1(* pszSrc    ) &&	/* Shift_JIS �S�p������ 1�o�C�g�� */
		_IS_SJIS_2(*(pszSrc+1) )	/* Shift_JIS �S�p������ 2�o�C�g�� */
	){	/* Shift_JIS�S�p�����ł��� */
		tmpw = ( ((unsigned int)*pszSrc) << 8 ) | ( (unsigned int)*(pszSrc + 1) );
		if(
			( *pszSrc == 0x0fa ) ||
			( *pszSrc == 0x0fb ) ||
			( ( *pszSrc == 0x0fc ) && ( *(pszSrc+1) <= 0x04b ) )
		) {		/* fa40�`fc4b �̕����ł���B */
			/* �����R�[�h�ϊ����� */
			if		  ( tmpw <= 0xfa49 ) {	tmpw -= 0x0b51;	}	/* fa40�`fa49 �� eeef�`eef8 (�@�`�I) */
			else	if( tmpw <= 0xfa53 ) {	tmpw -= 0x72f6;	}	/* fa4a�`fa53 �� 8754�`875d (�T�`�]) */
			else	if( tmpw <= 0xfa57 ) {	tmpw -= 0x0b5b;	}	/* fa54�`fa57 �� eef9�`eefc (�ʁ`�W) */
			else	if( tmpw == 0xfa58 ) {	tmpw  = 0x878a;	}	/* �� */
			else	if( tmpw == 0xfa59 ) {	tmpw  = 0x8782;	}	/* �� */
			else	if( tmpw == 0xfa5a ) {	tmpw  = 0x8784;	}	/* �� */
			else	if( tmpw == 0xfa5b ) {	tmpw  = 0x879a;	}	/* �� */
			else	if( tmpw <= 0xfa7e ) {	tmpw -= 0x0d1c;	}	/* fa5c�`fa7e �� ed40�`ed62 (�\�`�~) */
			else	if( tmpw <= 0xfa9b ) {	tmpw -= 0x0d1d;	}	/* fa80�`fa9b �� ed63�`ed7e (���`��) */
			else	if( tmpw <= 0xfafc ) {	tmpw -= 0x0d1c;	}	/* fa9c�`fafc �� ed80�`ede0 (���`��) */
			else	if( tmpw <= 0xfb5b ) {	tmpw -= 0x0d5f;	}	/* fb40�`fb5b �� ede1�`edfc (�@�`�[) */
			else	if( tmpw <= 0xfb7e ) {	tmpw -= 0x0d1c;	}	/* fb5c�`fb7e �� ee40�`ee62 (�\�`�~) */
			else	if( tmpw <= 0xfb9b ) {	tmpw -= 0x0d1d;	}	/* fb80�`fb9b �� ee63�`ee7e (���`��) */
			else	if( tmpw <= 0xfbfc ) {	tmpw -= 0x0d1c;	}	/* fb9c�`fbfc �� ee80�`eee0 (���`��) */
			else{							tmpw -= 0x0d5f;	}	/* fc40�`fc4b �� eee1�`eeec (�@�`�K) */
		}
		return (unsigned short) _mbcjmstojis( tmpw );
	}
	return 0;
}


/* �R�[�h�ϊ� SJIS��JIS */
void CMemory::SJIStoJIS( void )
{
	char*	pBufJIS;
	int		nBufJISLen;
	CMemory	cMem;

	/* SJIS��JIS */
	StrSJIStoJIS( &cMem, (unsigned char *)m_pData, m_nDataLen );
	pBufJIS = cMem.GetStringPtr( &nBufJISLen );
	SetString( pBufJIS, nBufJISLen );
	return;
}


/*!	SJIS��JIS

	@date 2003.09.07 genta �s�v�ȃL���X�g����
*/
int CMemory::StrSJIStoJIS( CMemory* pcmemDes, unsigned char* pszSrc, int nSrcLen )
{
//	BOOL bSJISKAN	= FALSE;
//	BOOL b8BITCODE	= FALSE;

	long			nWorkBgn;
	long			nWorkLen;
	int				i;
	int				j;
	unsigned char *	pszWork;
	int				nCharKindOld;
	int				nCharKind;
	int				bChange;
	nCharKind = CHAR_ASCII;		/* ASCII���� */
	nCharKindOld = nCharKind;
	bChange = FALSE;
//	/* ������� */
//	#define CHAR_ASCII		0	/* ASCII���� */
//	#define CHAR_8BITCODE	1	/* 8�r�b�g�R�[�h(���p�J�^�J�i�Ȃ�) */
//	#define CHAR_ZENKAKU	2	/* �S�p���� */

	pcmemDes->SetString( "" );
	pcmemDes->AllocStringBuffer( nSrcLen );
//	bSJISKAN  = FALSE;
	nWorkBgn = 0;
	for( i = 0;; i++ ){
		/* �������I������ */
		if( i >= nSrcLen ){
			nCharKind = CHAR_NULL;	/* �Ȃɂ��Ȃ� */
		}else
		// �������H
		if( ( i < nSrcLen - 1) &&
			/* SJIS�S�p�R�[�h��1�o�C�g�ڂ� */	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
			_IS_SJIS_1( pszSrc[i + 0] ) &&
			/* SJIS�S�p�R�[�h��2�o�C�g�ڂ� */
			_IS_SJIS_2( pszSrc[i + 1] )
		){
			nCharKind = CHAR_ZENKAKU;	/* �S�p���� */
//			++i;
		}else
		if( pszSrc[i] & (unsigned char)0x80 ){
			nCharKind = CHAR_8BITCODE;	/* 8�r�b�g�R�[�h(���p�J�^�J�i�Ȃ�) */
		}else{
			nCharKind = CHAR_ASCII;		/* ASCII���� */
		}
		/* ������ނ��ω����� */
		if( nCharKindOld != nCharKind ){
			if( CHAR_NULL != nCharKind ){
				bChange = TRUE;
			}

			nWorkLen = i - nWorkBgn;
			/* �ȑO�̕������ */
			switch( nCharKindOld ){
			case CHAR_ASCII:	/* ASCII���� */
				if( 0 < nWorkLen ){
					pcmemDes->AppendString( (char *)&(pszSrc[nWorkBgn]), nWorkLen );
				}
				break;
			case CHAR_8BITCODE:	/* 8�r�b�g�R�[�h(���p�J�^�J�i�Ȃ�) */
				if( 0 < nWorkLen ){
					pszWork = new unsigned char[nWorkLen + 1];
					memcpy( pszWork, &pszSrc[nWorkBgn], nWorkLen );
					pszWork[ nWorkLen ] = '\0';
					for( j = 0; j < nWorkLen; ++j ){
						pszWork[j] -= (unsigned char)0x80;
					}
					pcmemDes->AppendString( (char *)pszWork, nWorkLen );
					delete [] pszWork;
				}
				break;
			case CHAR_ZENKAKU:	/* �S�p���� */
				if( 0 < nWorkLen ){
					pszWork = new unsigned char[nWorkLen + 1];
					memcpy( pszWork, &pszSrc[nWorkBgn], nWorkLen );
					pszWork[ nWorkLen ] = '\0';
					// SJIS��JIS�ϊ�
					nWorkLen = MemSJIStoJIS( pszWork, nWorkLen );
					pcmemDes->AppendString( (char *)pszWork, nWorkLen );
					delete [] pszWork;
				}
				break;
			}
			/* �V����������� */
			switch( nCharKind ){
			case CHAR_ASCII:	/* ASCII���� */
				pcmemDes->AppendString( ESC_ASCII );
				break;
			case CHAR_NULL:		/* �Ȃɂ��Ȃ� */
				if( bChange &&					/* ���͕����킪�ω����� */
					nCharKindOld != CHAR_ASCII	/* ���O��ASCII�����ł͂Ȃ� */
				){
					pcmemDes->AppendString( ESC_ASCII );
				}
				break;
			case CHAR_8BITCODE:	/* 8�r�b�g�R�[�h(���p�J�^�J�i�Ȃ�) */
				pcmemDes->AppendString( ESC_8BIT );
				break;
			case CHAR_ZENKAKU:	/* �S�p���� */
				pcmemDes->AppendString( ESC_JIS );
				break;
			}
			nCharKindOld = nCharKind;
			nWorkBgn = i;
			if( nCharKind == CHAR_NULL ){	/* �Ȃɂ��Ȃ� */
				break;
			}
		}
		if( nCharKind == CHAR_ZENKAKU ){	/* �S�p���� */
			++i;
		}
	}
	return pcmemDes->m_nDataLen;
}



/* SJIS��JIS�ϊ� */
long CMemory::MemSJIStoJIS( unsigned char* pszSrc, long nSrcLen )
{
	int				i, j;
	char *			pszDes;
	unsigned short	sCode;

	pszDes = new char[nSrcLen + 1];
	memset( pszDes, 0, nSrcLen + 1 );
	j = 0;
	for( i = 0; i < nSrcLen - 1; i++ ){
		//	Oct. 3, 2002 genta IBM�g�������Ή�
		sCode = _mbcjmstojis_ex( pszSrc + i );
		if( sCode != 0 ){
			pszDes[j	] = (unsigned char)(sCode >> 8);
			pszDes[j + 1] = (unsigned char)(sCode);
			++i;
			j += 2;
		}else{
			pszDes[j] = pszSrc[i];
			j++;
		}
	}
	pszDes[j] = 0;
	memcpy( (char *)pszSrc, (const char *)pszDes, j );
	delete [] pszDes;
	return j;
}


/* �R�[�h�ϊ� JIS��SJIS */
void CMemory::JIStoSJIS( bool bMIMEdecode )
{
	int				i;
	int				j;
	unsigned char*	pszDes;
	unsigned char*	pszSrc = (unsigned char*)m_pData;
	long			nSrcLen = m_nDataLen;
	BOOL			bMIME = FALSE;
	int				nMEME_Selected;
	long			nWorkBgn;
	long			nWorkLen;
	unsigned char*	pszWork;

	int				nISOCode = CHAR_ASCII;
	int				nOldISOCode = nISOCode;
	BOOL			bFindESCSeq = FALSE;
	int				nESCSeqLen  = - 1; // �G�X�P�[�v�V�[�P���X�� - 1

	pszDes = new unsigned char [nSrcLen + 1];
	memset( pszDes, 0, nSrcLen + 1 );
	j = 0;
	if( false != bMIMEdecode ){
		for( i = 0; i < nSrcLen; i++ ){
			if( i <= nSrcLen - 16 && '=' == pszSrc[i] ){
				if( 0 == my_memicmp( "=?ISO-2022-JP?B?", &pszSrc[i], 16 ) ){
					nMEME_Selected = MIME_BASE64;
					bMIME = TRUE;
					i += 15;
					nWorkBgn = i + 1;
					continue;
				}
				if( 0 == my_memicmp( "=?ISO-2022-JP?Q?", &pszSrc[i], 16 ) ){
					nMEME_Selected = MIME_QUOTED;
					bMIME = TRUE;
					i += 15;
					nWorkBgn = i + 1;
					continue;
				}
			}
			if( bMIME == TRUE ){
				if( i <= nSrcLen - 2  &&
					0 == memcmp( "?=", &pszSrc[i], 2 ) ){
					nWorkLen = i - nWorkBgn;
					pszWork = new unsigned char [nWorkLen + 1];
//					memset( pszWork, 0, nWorkLen + 1 );
					memcpy( pszWork, &pszSrc[nWorkBgn], nWorkLen );
					pszWork[nWorkLen] = '\0';
					switch( nMEME_Selected ){
					  case MIME_BASE64:
						// Base64�f�R�[�h
						nWorkLen = MemBASE64_Decode(pszWork, nWorkLen);
						break;
					  case MIME_QUOTED:
						// Quoted-Printable�f�R�[�h
						nWorkLen = QuotedPrintable_Decode( (char*)pszWork, nWorkLen );
						break;
					}
					memcpy( &pszDes[j], pszWork, nWorkLen );
					bMIME = FALSE;
					j += nWorkLen;
					++i;
					delete [] pszWork;
					continue;
				}else{
					continue;
				}
			}
			pszDes[j] = pszSrc[i];
			j++;
		}
		if( bMIME ){
			nWorkBgn -= 16; // MIME�w�b�_�����̂܂܃R�s�[
			nWorkLen = i - nWorkBgn;
			memcpy( &pszDes[j], &pszSrc[nWorkBgn], nWorkLen );
			j += nWorkLen;
		}

		// ��ASCII�e�L�X�g�Ή����b�Z�[�W�w�b�_��MIME�R�[�h
		memcpy( (char *)pszSrc, (const char *)pszDes, j );

		nSrcLen = j;
	}

	nWorkBgn = 0;
	j = 0;
	for( i = 0; i < nSrcLen; i++ ){
		if( i <= nSrcLen - 3		&&
			pszSrc[i + 0] == 0x1b	&&
			pszSrc[i + 1] == '$'	&&
		   (pszSrc[i + 2] == 'B' || pszSrc[i + 2] == '@') ){

			bFindESCSeq = TRUE;
			nOldISOCode = nISOCode;
			nISOCode = CHAR_ZENKAKU;
			nESCSeqLen = 2;
		}else
		if( i <= nSrcLen - 3		&&
			pszSrc[i + 0] == 0x1b	&&
			pszSrc[i + 1] == '('	&&
			pszSrc[i + 2] == 'I' ){

			bFindESCSeq = TRUE;
			nOldISOCode = nISOCode;
			nISOCode = CHAR_8BITCODE;
			nESCSeqLen = 2;
		}else
		if( i <= nSrcLen - 3		&&
			pszSrc[i + 0] == 0x1b	&&
			pszSrc[i + 1] == '('	&&
		   (pszSrc[i + 2] == 'B' || pszSrc[i + 2] == 'J') ){
			
			bFindESCSeq = TRUE;
			nOldISOCode = nISOCode;
			nISOCode = CHAR_ASCII;
			nESCSeqLen = 2;
		}
		// else{}

		if( bFindESCSeq ){
			if( 0 < i - nWorkBgn ){
				if( CHAR_ZENKAKU == nOldISOCode ){
					nWorkLen = i - nWorkBgn;
					pszWork = new unsigned char [nWorkLen + 1];
					memcpy( pszWork, &pszSrc[nWorkBgn], nWorkLen );
					pszWork[nWorkLen] = '\0';
					// JIS��SJIS�ϊ�
					nWorkLen = MemJIStoSJIS( (unsigned char*)pszWork, nWorkLen );
					memcpy( &pszDes[j], pszWork, nWorkLen );
					j += nWorkLen;
					delete [] pszWork;
				}
			}
			i += nESCSeqLen;
			nWorkBgn = i + 1;
			bFindESCSeq = FALSE;
			continue;
		}else
		if( CHAR_ASCII == nISOCode ){
			pszDes[j] = pszSrc[i];
			j++;
			continue;
		}else
		if( CHAR_8BITCODE == nISOCode ){
			pszDes[j] = (unsigned char)0x80 | pszSrc[i];
			j++;
			continue;
		}
	}

	// ESCSeq��ASCII�ɖ߂�Ȃ������Ƃ��ɁC�f�[�^������Ȃ��悤��
	if( CHAR_ZENKAKU == nISOCode ){
		if( 0 < i - nWorkBgn ){
			nWorkBgn -= nESCSeqLen + 1; // ESCSeq���c���Ă�������
			nWorkLen = i - nWorkBgn;
			memcpy( &pszDes[j], &pszSrc[nWorkBgn], nWorkLen );
			j += nWorkLen;
		}
	}

	memcpy( pszSrc, pszDes, j );
 	m_nDataLen = j;
	delete [] pszDes;
	return;
}





/* ������Base64�̃f�[�^�� */
int CMemory::IsBASE64Char( char cData )
{
	int nret = -1;
	if( (unsigned char)cData < 0x80 ){
		nret = Charcode::BASE64VAL[cData];
		if( nret == 0xff ){
			nret = -1;
		}
	}
	return nret;
}

// BASE64 => �f�R�[�h��
// 4����  => 3����

// Base64�f�R�[�h
long CMemory::MemBASE64_Decode( unsigned char* pszSrc, long nSrcLen )
{
	int				i, j, k;
	long			nSrcSize;
	long			lData;
	unsigned char*	pszDes;
	long			nDesLen;
	long			lDesSize;
	int				sMax;

	//Src�̗L�����̎Z�o
	for( i = 0; i < nSrcLen; i++ ){
		if( pszSrc[i] == '=' ){
			break;
		}
	}
	nSrcSize = i;
	nDesLen = (nSrcSize / 4) * 3;
	nDesLen += ((nSrcSize % 4) * 6) / 8;
	pszDes = new unsigned char[nDesLen + 1];
	memset( pszDes, 0, nDesLen + 1 );

	lDesSize = 0;
	for( i = 0; i < nSrcSize; i++ ){
		if( i < nSrcSize - (nSrcSize % 4) ){
			sMax = 4;
		}else{
			sMax = (nSrcSize % 4);
		}
		lData = 0;
		for( j = 0; j < sMax; j++ ){
			k = IsBASE64Char( pszSrc[i + j] );
			if( k >= 0 ){
				lData |= (((long)k) << ((4 - j - 1) * 6));
			}else{
			}
		}
		for( j = 0; j < (sMax * 6)/ 8 ; j++ ){
			pszDes[lDesSize] = (unsigned char)((lData >> (8 * (2 - j))) & 0x0000ff);
			lDesSize++;
		}
		i+= 3;
	}
	pszDes[lDesSize] = 0;
	memcpy( (char *)pszSrc, (const char *)pszDes, lDesSize );
	delete [] pszDes;
	return lDesSize;
}


/*
* Base64�G���R�[�h
*
*	�����������f�[�^�́A�V���Ɋm�ۂ����������Ɋi�[����܂�
*	�I�����ɁA���̃������n���h�����w�肳�ꂽ�A�h���X�Ɋi�[���܂�
*	���������ꂽ�f�[�^��́A�ꉞNULL�I�[������ɂȂ��Ă��܂�
*
*/
int CMemory::MemBASE64_Encode(
	const char*	pszSrc		,	// �G���R�[�h�Ώۃf�[�^
	int			nSrcLen		,	// �G���R�[�h�Ώۃf�[�^��
	char**		ppszDes		,	// ���ʃf�[�^�i�[�������|�C���^�̃A�h���X
//	HGLOBAL*	phgDes		,	// ���ʃf�[�^�̃������n���h���i�[�ꏊ
//	long*		pnDesLen	,	// ���ʃf�[�^�̃f�[�^���i�[�ꏊ
	int			nWrap		,	// �G���R�[�h��̃f�[�^�������I��CRLF�Ő܂�Ԃ��ꍇ�̂P�s�ő啶���� (-1���w�肳�ꂽ�ꍇ�͐܂�Ԃ��Ȃ�)
	int			bPadding		// �p�f�B���O���邩
)
{
	int			i, j, k, m, n;
	long		nDesIdx;
//	char*		pszDes;
	long		nDataDes;
	long		nDataSrc;
	long		nLineLen;
	char		cw;
	const char	szBASE64CODE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int			nBASE64CODE_Num = sizeof( szBASE64CODE ) / sizeof( szBASE64CODE[0] );
	int			nDesLen;

	// ��������̒������Z�o�i�w�b�_�[�E�t�b�^�[�������j
	//=?ISO-2022-JP?B? GyRC QXc/ Lkgi JE4/ NiRq Siwk MRso Qg== ?=
//	if( bPadding ){
		nDesLen = ((nSrcLen / 3) + ((nSrcLen % 3)? 1:0)) * 4;
		if( -1 != nWrap ){
			nDesLen += 2 * ( nDesLen / nWrap + ((nDesLen % nWrap)? 1:0 ));
		}
//	}else{
//		nDesLen = ?????????????????????????????????????????;
//	}

//	*phgDes = GlobalAlloc( GHND, nDesLen + 1 );
//	pszDes = (char *)GlobalLock( *phgDes );
	(*ppszDes) = new char[nDesLen + 1];
	memset( (*ppszDes), 0, nDesLen + 1 );

	nDesIdx = 0;
	nLineLen = 0;
	for( i = 0; i < nSrcLen; i += 3 ){
		memcpy( (void *)&nDataDes, "====", 4 );
		nDataSrc = 0;
		if( nSrcLen - i < 3 ){
			k = (nSrcLen % 3) * 8 / 6 + 1;
			for( m = 0; m < nSrcLen % 3; m++ ){
				((char*)&nDataSrc)[3 - m] = pszSrc[i + m];
			}
		}else{
			k = 4;
			for( m = 0; m < 3; m++ ){
				((char*)&nDataSrc)[3 - m] = pszSrc[i + m];
			}
		}
		for( j = 0; j < k; j++ ){
			cw = (char)((nDataSrc >> (6 * (3 - j) + 8)) & 0x0000003f);
			((char*)&nDataDes)[j] = szBASE64CODE[(int)cw];
		}
		if( bPadding ){		// �p�f�B���O���邩
			k = 4;
		}else{
			nDesLen -= (4 - k);
		}
		for( n = 0; n < k; n++ ){
			// �����܂�Ԃ��̏���
			if( nWrap != -1 && nLineLen == nWrap ){
				(*ppszDes)[nDesIdx + 0] = CR;
				(*ppszDes)[nDesIdx + 1] = LF;
				nDesIdx += 2;
				nLineLen = 0;
			}
			(*ppszDes)[nDesIdx] = ((char*)&nDataDes)[n];
			nDesIdx++;
			nLineLen++;
		}
	}
//	GlobalUnlock( *phgDes );
	return nDesLen;
}



/* Base64�f�R�[�h */
void CMemory::BASE64Decode( void )
{
	int		nBgn;
	int		nPos;
	char*	pszSrc = m_pData;
	long	nSrcLen = m_nDataLen;
	CMemory	cmemBuf;

	nBgn = 0;
	nPos = 0;
	while( nBgn < nSrcLen ){
		while( nBgn < nSrcLen && 0 > IsBASE64Char( pszSrc[nBgn] ) ){
			nBgn++;
		}
		nPos = nBgn + 1;
		while( nPos < nSrcLen && 0 <= IsBASE64Char( pszSrc[nPos] ) ){
			nPos++;
		}
		if( nBgn < nSrcLen && nPos - nBgn > 0 ){
			char*	pData;
			int		nDesLen;
			pData = new char[nPos - nBgn];
			memcpy( pData, &pszSrc[nBgn], nPos - nBgn );

			// Base64�f�R�[�h
			nDesLen = MemBASE64_Decode( (unsigned char *)pData, nPos - nBgn );
			cmemBuf.AppendString( pData, nDesLen );
			delete [] pData;
		}
		nBgn = nPos;
	}
	SetString( cmemBuf.GetStringPtr(), cmemBuf.m_nDataLen );
	return;
}




/* Uudecode (�f�R�[�h�j*/
void CMemory::UUDECODE( char* pszFileName )
{
	unsigned char*	pszSrc = (unsigned char *)m_pData;
	long			nSrcLen = m_nDataLen;
	CMemory			cmemBuf;
	int				nBgn;
	int				nPos;
	BOOL			bBEGIN;
	unsigned char	uchDecode[64];
	bBEGIN = FALSE;
	nBgn = 0;
	while( nBgn < nSrcLen ){
		nPos = nBgn;
		while( nPos < nSrcLen && pszSrc[nPos] != CR && pszSrc[nPos] != LF ){
			nPos++;
		}
		if( nBgn < nSrcLen && nPos - nBgn > 0 ){
			int				nBufLen = nPos - nBgn;
			unsigned char*	pBuf = new unsigned char[ nBufLen + 1];
			memset( pBuf, 0, nBufLen + 1 );
			memcpy( pBuf, &pszSrc[nBgn], nBufLen );
			if( !bBEGIN	){
				if( 0 == memcmp( pBuf, "begin ", 6 ) ){
					bBEGIN = TRUE;
					char*	pTok;
					/* �ŏ��̃g�[�N�����擾���܂��B*/
					pTok = strtok( (char*)pBuf, " " );
					/* ���̃g�[�N�����擾���܂��B*/
					pTok = strtok( NULL, " " );
					/* �X�Ɏ��̃g�[�N�����擾���܂��B*/
					pTok = strtok( NULL, "\0" );
					if( NULL != pszFileName ){
						strcpy( pszFileName, pTok );
					}
				}
			}else{
				if( 0 == memcmp( pBuf, "end", 3 ) ){
					break;
				}else{
					int		nBytes;
					nBytes = UUDECODE_CHAR( pBuf[0] );
					if( 0 == nBytes ){
						break;
					}
					int		iByteIndex = 0;
					int		iCharIndex = 1;
					while ( iByteIndex < nBytes ) {
						// Note that nBytes may not be a multiple of 3, but the
						// number of characters on the line should be a multiple of 4.
						// If the number of encoded bytes is not a multiple of 3 then
						// extra pad characters should have been added to the text
						// by the encoder to make the character count a multiple of 4.
//						if( pBuf[iCharIndex] == '\0' ||
//							pBuf[iCharIndex + 1] == '\0' ||
//							pBuf[iCharIndex + 2] == '\0' ||
//							pBuf[iCharIndex + 3] == '\0') {
//							//break;
//						}
						// Decode the line in 3-byte (=4-character) jumps.
						if( iByteIndex < nBytes ){
							uchDecode[iByteIndex] =
								(unsigned char)((UUDECODE_CHAR(pBuf[iCharIndex]) << 2) |
								(UUDECODE_CHAR(pBuf[iCharIndex + 1]) >> 4));
							iByteIndex++;
						}
						if( iByteIndex < nBytes ){
							uchDecode[iByteIndex] =
								(unsigned char)((UUDECODE_CHAR(pBuf[iCharIndex + 1]) << 4) |
								(UUDECODE_CHAR(pBuf[iCharIndex + 2]) >> 2));
							iByteIndex++;
						}
						if( iByteIndex < nBytes ){
							uchDecode[iByteIndex] =
								(unsigned char)((UUDECODE_CHAR(pBuf[iCharIndex + 2]) << 6) |
								UUDECODE_CHAR(pBuf[iCharIndex + 3]));
							iByteIndex++;
						}
						iCharIndex += 4;
					}
					cmemBuf.AppendString( (char*)uchDecode, iByteIndex );
				}
			}
			delete [] pBuf;
		}
		nBgn = nPos;
		while( nBgn < nSrcLen && ( pszSrc[nBgn] == CR || pszSrc[nBgn] == LF ) ){
			nBgn++;
		}
	}
	SetString( cmemBuf.GetStringPtr(), cmemBuf.m_nDataLen );
	return;
}



/* Quoted-Printable�f�R�[�h */
long CMemory::QuotedPrintable_Decode( char* pszSrc, long nSrcLen )
{
	int			i;
	char*		pszDes;
	long		lDesSize;
	char		szHex[3];
	int			nHex;

	memset( szHex, 0, 3 );
	pszDes = new char [nSrcLen + 1];
	memset( pszDes, 0, nSrcLen + 1 );
	lDesSize = 0;
	for( i = 0; i < nSrcLen; i++ ){
		if( pszSrc[i] == '=' ){
			szHex[0] = pszSrc[i + 1];
			szHex[1] = pszSrc[i + 2];
			sscanf( szHex, "%x", &nHex );
			pszDes[lDesSize] = (char)nHex;
			lDesSize++;
			i += 2;
		}else{
			pszDes[lDesSize] = pszSrc[i];
			lDesSize++;
		}
	}
	pszDes[lDesSize] = 0;
	memcpy( (char *)pszSrc, (const char *)pszDes, lDesSize );
	delete [] pszDes;
	return lDesSize;
}





/* JIS��SJIS�ϊ� */
long CMemory::MemJIStoSJIS( unsigned char* pszSrc, long nSrcLen )
{
	int				i, j;
	char*			pszDes;
	unsigned short	sCode;

	pszDes = new char [nSrcLen + 1];
	memset( pszDes, 0, nSrcLen + 1 );
	j = 0;
	for( i = 0; i < nSrcLen - 1; i++ ){
		sCode = (unsigned short)_mbcjistojms(
			(unsigned int)
			(((unsigned short)pszSrc[i	  ] << 8) |
			 ((unsigned short)pszSrc[i + 1]))
		);
		if( sCode != 0 ){
			pszDes[j	] = (unsigned char)(sCode >> 8);
			pszDes[j + 1] = (unsigned char)(sCode);
			++i;
			j+=2;
		}else{
			pszDes[j] = pszSrc[i];
			j++;
		}
	}
	pszDes[j] = 0;
	memcpy( (char *)pszSrc, (const char *)pszDes, j );
	delete [] pszDes;
	return j;
}



/************************************************************************
*
*�y�֐����z
*	EUCToSJIS
*
*�y�@�\�z
*	�w��͈͂̃o�b�t�@����EUC�����R�[�h
*	�������SJIS�S�p�R�[�h�ɕϊ�����B	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
*	���p�����͕ϊ������ɂ��̂܂܎c���B
*
*	���䕶��CRLF�ȊO�̃o�C�i���R�[�h���������Ă���ꍇ�ɂ͌��ʂ��s���
*	�Ȃ邱�Ƃ�����̂Œ��ӁB
*	�o�b�t�@�̍Ō�Ɋ����R�[�h��1�o�C�g�ڂ���������ƍ���
*
*�y���́z	�Ȃ�
*
*�y�߂�l�z	�Ȃ�
*
************************************************************************/
/* EUC��SJIS�R�[�h�ϊ� */
void CMemory::EUCToSJIS( void )
{
	char*			pBuf = m_pData;
	int				nBufLen = m_nDataLen;
	int				nPtr = 0L;
	int				nPtrDes = 0L;
	char*			pszDes = new char[nBufLen];
	unsigned int	sCode;

	while( nPtr < nBufLen ){
		if( (unsigned char)pBuf[nPtr] == (unsigned char)0x8e && nPtr < nBufLen - 1 ){
			/* ���p�J�^�J�i */
			pszDes[nPtrDes] = pBuf[nPtr + 1];
			nPtrDes++;
			nPtr += 2;
		}else
		/* EUC�����R�[�h��? */
		if( nPtr < nBufLen - 1 && Charcode::IsEucKan1(pBuf[nPtr]) && Charcode::IsEucKan2(pBuf[nPtr + 1L]) ){
			/* �ʏ��JIS�R�[�h�ɕϊ� */
			pBuf[nPtr	  ] &= 0x7f;
			pBuf[nPtr + 1L] &= 0x7f;

			/* SJIS�R�[�h�ɕϊ� */	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
			sCode = (unsigned short)_mbcjistojms(
				(unsigned int)
				(((unsigned short)pBuf[nPtr] << 8) |
				 ((unsigned short)pBuf[nPtr + 1]))
			);
			if( sCode != 0 ){
				pszDes[nPtrDes	  ] = (unsigned char)(sCode >> 8);
				pszDes[nPtrDes + 1] = (unsigned char)(sCode);
				nPtrDes += 2;;
				nPtr += 2;
			}else{
				pszDes[nPtrDes] = pBuf[nPtr];
				nPtrDes++;
				nPtr++;
			}
		}else{
			pszDes[nPtrDes] = pBuf[nPtr];
			nPtrDes++;
			nPtr++;
		}
	}
	SetString( pszDes, nPtrDes );
	delete [] pszDes;
	return;
}




/* SJIS��EUC�R�[�h�ϊ� */
void CMemory::SJISToEUC( void )
{
	unsigned char*	pBuf = (unsigned char*)m_pData;
	int				nBufLen = m_nDataLen;
	int				i;
	int				nCharChars;
	unsigned char*	pDes;
	int				nDesIdx;
	unsigned short	sCode;

	pDes = new unsigned char[nBufLen * 2];
	nDesIdx = 0;
	for( i = 0; i < nBufLen; ++i ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pBuf, nBufLen, i );
		if( nCharChars == 1 ){
			if( pBuf[i] >= (unsigned char)0x80 ){
				/* ���p�J�^�J�i */
				pDes[nDesIdx	] = (unsigned char)0x8e;
				pDes[nDesIdx + 1] = pBuf[i];
				nDesIdx += 2;
			}else{
				pDes[nDesIdx] = pBuf[i];
				nDesIdx++;
			}
		}else
		if( nCharChars == 2 ){
			/* �S�p���� */
			//	Oct. 3, 2002 genta IBM�g�������Ή�
			sCode =	(unsigned short)_mbcjmstojis_ex( pBuf + i );
			if(sCode != 0){
				pDes[nDesIdx	] = (unsigned char)0x80 | (unsigned char)(sCode >> 8);
				pDes[nDesIdx + 1] = (unsigned char)0x80 | (unsigned char)(sCode);
				nDesIdx += 2;
				++i;
			}else{
				pDes[nDesIdx	] = pBuf[i];
				pDes[nDesIdx + 1] = pBuf[i + 1];
				nDesIdx += 2;
				++i;
			}
		}else
		if( nCharChars > 0 ){
			i += nCharChars - 1;
		}
	}
	SetString( (const char *)pDes, nDesIdx );
	delete [] pDes;
	return;
}

/****
UTF-8�̃R�[�h
	�r�b�g��		���e
	0xxx xxxx	1�o�C�g�R�[�h
	10xx xxxx	2�o�C�g�R�[�h�A3�o�C�g�R�[�h��2, 3������
	110x xxxx	2�o�C�g�R�[�h�̐擪�o�C�g
	1110 xxxx	3�o�C�g�R�[�h�̐擪�o�C�g

UTF-8�̃G���R�[�f�B���O
	�t�H�[�}�b�g	Unicode			Unicode�r�b�g��						UTF�r�b�g��				���l
	1�o�C�g�R�[�h	\u0�`\u7F		0000 0000 0aaa bbbb					0aaa bbbb
	2�o�C�g�R�[�h	\u80�`\u7FF		0000 0aaa bbbb cccc					110a aabb 10bb cccc
	3�o�C�g�R�[�h	\u800�`\uFFFF	aaaa bbbb cccc dddd					1110 aaaa 10bb bbcc 10cc dddd
	4�o�C�g�R�[�h	Surrogate		110110wwwwzzzzyy + 110111yyyyxxxxxx 1111 0uuu 10uu zzzz 10yy yyyy 10xx xxxx Java�ł͖��g�p
***/

/* UTF-8�̕����� */
int CMemory::IsUTF8( const unsigned char* pData, int nDataLen )
{
	if( 3 <= nDataLen
	 && (unsigned char)0xe0 == (unsigned char)(pData[0] & (unsigned char)0xf0)
	 && (unsigned char)0x80 == (unsigned char)(pData[1] & (unsigned char)0xc0)
	 && (unsigned char)0x80 == (unsigned char)(pData[2] & (unsigned char)0xc0)
	){
		return 3;
	}
	if( 2 <= nDataLen
	 && (unsigned char)0xc0 == (unsigned char)(pData[0] & (unsigned char)0xe0)
	 && (unsigned char)0x80 == (unsigned char)(pData[1] & (unsigned char)0xc0)
	){
		return 2;
	}
//	if( 1 <= nDataLen
//	 && (unsigned char)0 == (unsigned char)(pData[0] & (unsigned char)0x80)
//	){
//		return 1;
//	}
	return 0;

}

int CMemory::DecodeUTF8toUnicode( const unsigned char* pData, int nUTF8Bytes, unsigned char* pDes )
{
	switch( nUTF8Bytes ){
	case 1:
		pDes[1] = 0x00;
		pDes[0] = pData[0];
		return 2;
	case 2:
		pDes[1] = ( pData[0] & 0x1c ) >> 2;
		pDes[0] = ( pData[0] << 6 ) | ( pData[1] & 0x3f );
		return 2;
	case 3:
		pDes[1] =  ( ( pData[0] & 0x0f ) << 4 )
				 | ( ( pData[1] & 0x3c ) >> 2 );
		pDes[0] =  (   pData[2] & 0x3f )
				 | ( ( pData[1] & 0x03 ) << 6 );
		return 2;
	}
	return 0;
}

/* UTF-8��SJIS�R�[�h�ϊ� */
void CMemory::UTF8ToSJIS( void )
{
	unsigned char*	pBuf = (unsigned char*)m_pData;
	int				nBufLen = m_nDataLen;
	int				i;
	int				j;
//	int				nCharChars;
	unsigned char*	pDes;
	int				k;
//	unsigned short	sCode;
	int				nUTF8Bytes;
	int				nUNICODEBytes;
	unsigned char	pWork[100];

	setlocale( LC_ALL, "Japanese" );
	pDes = new unsigned char[nBufLen * 2];
	k = 0;
	for( i = 0; i < nBufLen; ){
		/* UTF-8�̕����� */
		nUTF8Bytes = IsUTF8( &pBuf[i], nBufLen - i );
		switch( nUTF8Bytes ){
		case 1:
		case 2:
		case 3:
			nUNICODEBytes = DecodeUTF8toUnicode( &pBuf[i], nUTF8Bytes, pWork );
//			memcpy( &pDes[k], pWork, nUNICODEBytes );
//			k += nUNICODEBytes;

			if( 2 == nUNICODEBytes ){
				j = wctomb( (char*)&(pDes[k]), ((wchar_t*)pWork)[0] );
				if( -1 == j ){
					memcpy( &pDes[k], &pBuf[i], nUTF8Bytes );
					k += nUTF8Bytes;
				}else{
					k += j;
				}
			}else{
				memcpy( &pDes[k], &pBuf[i], nUTF8Bytes );
				k += nUTF8Bytes;
			}

			i += nUTF8Bytes;
			break;
		default:
			pDes[k] = pBuf[i];
			++i;
			k++;
			break;
		}
	}
	SetString( (const char *)pDes, k );
	delete [] pDes;
	return;

}





/* �R�[�h�ϊ� SJIS��UTF-8 */
void CMemory::SJISToUTF8( void )
{
	/* �R�[�h�ϊ� SJIS��Unicode */
	SJISToUnicode();

	/* �R�[�h�ϊ� Unicode��UTF-8 */
	UnicodeToUTF8();
	return;
}

/* �R�[�h�ϊ� SJIS��UTF-7 */
void CMemory::SJISToUTF7( void )
{
	/* �R�[�h�ϊ� SJIS��Unicode */
	SJISToUnicode();

	/* �R�[�h�ϊ� Unicode��UTF-7 */
	UnicodeToUTF7();
	return;
}




/* �R�[�h�ϊ� Unicode��UTF-8 */
void CMemory::UnicodeToUTF8( void )
{
	wchar_t*		pUniBuf = (wchar_t*)m_pData;
//	char*			pBuf = (char*)m_pData;
	int				nBufLen = m_nDataLen;
	int				i;
//	int				j;
//	int				nCharChars;
	unsigned char*	pWork;
	unsigned char*	pDes;
	int				k;
//	unsigned short	sCode;
//	int				nUTF8Bytes;
//	int				nUNICODEBytes;
//	unsigned char	pWork[100];
//	unsigned char*	pBufUTF8;
//	wchar_t*		pUniBuf;


	setlocale( LC_ALL, "Japanese" );
	k = 0;
	for( i = 0; i < (int)(nBufLen / sizeof( wchar_t )); ++i ){
		if( 0x0000 <= pUniBuf[i] && 0x007f >= pUniBuf[i] ){
			k += 1;
		}else
		if( 0x0080 <= pUniBuf[i] && 0x07ff >= pUniBuf[i] ){
			k += 2;
		}else
		if( 0x0800 <= pUniBuf[i] && 0xffff >= pUniBuf[i] ){
			k += 3;
		}
	}
	pDes = new unsigned char[k + 1];
	memset( pDes, 0, k + 1 );
	k = 0;
	for( i = 0; i < (int)(nBufLen / sizeof( wchar_t )); ++i ){
		pWork = (unsigned char*)&((pUniBuf[i]));
//		pWork = (unsigned char*)pUniBuf;
//		pWork += sizeof( wchar_t ) * i;

		if( 0x0000 <= pUniBuf[i] && 0x007f >= pUniBuf[i] ){
			pDes[k] = pWork[0];
			k += 1;
		}else
		if( 0x0080 <= pUniBuf[i] && 0x07ff >= pUniBuf[i] ){
			pDes[k] = ( ( pWork[0] & 0xc0 ) >> 6 )
					| ( ( pWork[1] & 0x07 ) << 2 )
					| 0xc0;
			pDes[k + 1] = ( pWork[0] & 0x3f ) | 0x80;
			k += 2;
		}else
		if( 0x0800 <= pUniBuf[i] && 0xffff >= pUniBuf[i] ){
			pDes[k] = ( ( pWork[1] & 0xf0 ) >> 4 )
						| 0xe0;
			pDes[k + 1] = 0x80
						| ( ( pWork[1] & 0x0f ) << 2 )
						| ( ( pWork[0] & 0xc0 ) >> 6 );
			pDes[k + 2] = 0x80
						| ( pWork[0] & 0x3f );

			k += 3;
		}else{
		}
	}
	SetString( (const char *)pDes, k );
	delete [] pDes;
	return;
}

/* �R�[�h�ϊ� UTF-7��SJIS

	UTF-7�ɂ��Ă�RFC 2152�Q�ƁD
*/
void CMemory::UTF7ToSJIS( void )
{
	char*		pBuf = (char*)m_pData;
	int			nBufLen = m_nDataLen;
	int			i;
	char*		pszWork;
	int			nWorkLen;
	char*		pDes;
	int			k;
	BOOL		bBASE64;
	int			nBgn;
//	CMemory		cmemWork;
	CMemory*	pcmemWork;

	pcmemWork = new CMemory;

	pDes = new char[nBufLen + 1];
	setlocale( LC_ALL, "Japanese" );
	k = 0;
	bBASE64 = FALSE;
	// 2002.10.05 Moca < ��<= �ɕύX �������CpBuf[nBufLen]�͑��݂��Ȃ����Ƃɒ���
	for( i = 0; i <= nBufLen; ++i ){
		if( !bBASE64 ){
			if( i < nBufLen - 1
			  &&  '+' == pBuf[i]
			  &&  '-' == pBuf[i + 1]
			){
				pDes[k] = '+';
				k++;
				++i;
			}else
			if( i < nBufLen - 2
			  &&  '+' == pBuf[i]
			  &&  -1 != IsBASE64Char( pBuf[i + 1] ) /* ������Base64�̃f�[�^�� */
			){
				nBgn = i + 1;
				bBASE64 = TRUE;
			}else
			if( i < nBufLen ){
				pDes[k] = pBuf[i];
				k++;
			}
		}else{
			if( i == nBufLen ||				//	2002.10.25 Moca �f�[�^�I�[�����f�R�[�h
				-1 == IsBASE64Char( pBuf[i] ) ){	//	Oct. 10, 2000 genta
				nWorkLen = i - nBgn;
				BOOL bSuccess;
				bSuccess = TRUE;

				if( 3 <= nWorkLen ){
					pszWork = new char [nWorkLen + 1];
//					memset( pszWork, 0, nWorkLen + 1 );
					memcpy( pszWork, &pBuf[nBgn], nWorkLen );
					pszWork[nWorkLen] = '\0';
					// Base64�f�R�[�h
					nWorkLen = MemBASE64_Decode( (unsigned char *)pszWork, nWorkLen );
					pszWork[nWorkLen] = '\0';
					if( 0 == nWorkLen % 2 ){
						/* Unicode��2�o�C�g�P�� */
						pcmemWork->SetString( pszWork, nWorkLen );
						/* �R�[�h�ϊ� UnicodeBE��SJIS */
						pcmemWork->UnicodeBEToSJIS();
						memcpy( &pDes[k], pcmemWork->m_pData, pcmemWork->m_nDataLen );
						k += pcmemWork->m_nDataLen;	//	Oct. 10, 2000 genta

						//	Oct. 10, 2000 genta
						//	'-'��Base64���̏I���������L��
						// ����ȊO��Base64�̏I���������Ɠ����ɗL�ӂȕ�����
						if( i < nBufLen && '-' != pBuf[i] )
							--i;
					}else{
						bSuccess = FALSE;
					}
					delete [] pszWork;
					pszWork = NULL;
				}else{
					bSuccess = FALSE;

				}
				if( FALSE == bSuccess ){
					if( i < nBufLen ){
						nWorkLen = i - nBgn + 2;
					}else{ // pBuf[nBufLen] �͖���
						nWorkLen = i - nBgn + 1;
					}
					memcpy( &pDes[k],  &pBuf[nBgn - 1], nWorkLen );
					k += nWorkLen;
				}
				bBASE64 = FALSE;
			}
		}
	}
	pDes[k] = '\0';
	SetString( (const char *)pDes, k );
	delete [] pDes;
	delete pcmemWork;
	return;
}

/*!
	Unicode�̕�����UTF-7�Œ��ڃG���R�[�h�ł��邩���ׂ�
	@author Moca
	@date 2002.10.25 �V�K�쐬

	TAB SP CR LF �� ���ڃG���R�[�h�\
	��{�Z�b�g
	         '(),-./012...789:?ABC...XYZabc...xyz
	�ȉ��̓I�v�V�����Ń��[���ł͎x����������ꍇ������
	         !"#$%&*;<=>@[\]^_`{|}
	�Ƃ肠�����������ŃI�v�V�����͒��ڕϊ��ł��Ȃ��Ɣ��f����
*/
int CMemory::IsUTF7Direct( wchar_t wc )
{
	int nret = 0;
	if( (wc & 0xff00) == 0 ){
		nret = Charcode::IsUtf7SetDChar( (unsigned char)wc );
	}
	return nret;
}



/*! �R�[�h�ϊ� Unicode��UTF-7
	@date 2002.10.25 Moca UTF-7�Œ��ڃG���R�[�h�ł��镶����RFC�ɍ��킹�Đ�������
*/
void CMemory::UnicodeToUTF7( void )
{
	wchar_t*		pUniBuf;
//	int				nBufLen = m_nDataLen;
	int				nUniBufLen = m_nDataLen / sizeof(wchar_t);
	int				i;
	int				j;
	unsigned char*	pDes;
	int				k;
	BOOL			bBASE64;
//	char			mbchar[4];
	int				nBgn;
	char*			pszBase64Buf;
	int				nBase64BufLen;
	char*			pszWork;
	char			cWork;

//	setlocale( LC_ALL, "Japanese" ); // wctomb ���g��Ȃ��Ȃ������߃R�����g�A�E�g
	k = 0;
	bBASE64 = FALSE;
	nBgn = 0;
	pUniBuf = new wchar_t[nUniBufLen + 1];
//	memset( pUniBuf, 0, (nUniBufLen + 1) * sizeof( wchar_t ) );
	memcpy( pUniBuf, m_pData, nUniBufLen * sizeof( wchar_t ) );
	pUniBuf[nUniBufLen] = L'\0';
	for( i = 0; i < nUniBufLen; ++i ){
		j = IsUTF7Direct( pUniBuf[i]);
		if( !bBASE64 ){
			if( 1 == j ){
				k++;
			}else
			if( L'+' == pUniBuf[i] ){
				k += 2;
			}else{
				bBASE64 = TRUE;
				nBgn = i;
			}
		}else{
			if( 1 == j ){
				/* 2�o�C�g��Unicode������Ƃ����O���LO/HI�o�C�g������ */
				pszWork = (char*)(char*)&pUniBuf[nBgn];
				for( j = 0; j < (int)((i - nBgn) * sizeof( wchar_t )); j += 2 ){
					cWork = pszWork[j + 1];
					pszWork[j + 1] = pszWork[j];
					pszWork[j] = cWork;
				}
				/* Base64�G���R�[�h */
				pszBase64Buf = NULL;
				nBase64BufLen = 0;
				nBase64BufLen = MemBASE64_Encode(
					(char*)&pUniBuf[nBgn],			// �G���R�[�h�Ώۃf�[�^
					(i - nBgn) * sizeof( wchar_t ), // �G���R�[�h�Ώۃf�[�^��
					&pszBase64Buf,					// ���ʃf�[�^�i�[�������|�C���^�̃A�h���X
					-1,		// �G���R�[�h��̃f�[�^�������I��CRLF�Ő܂�Ԃ��ꍇ�̂P�s�ő啶���� (-1���w�肳�ꂽ�ꍇ�͐܂�Ԃ��Ȃ�)
					FALSE	// �p�f�B���O���邩
				);
//				MYTRACE_A( "pszBase64Buf=[%s]\n", pszBase64Buf );
				//////////////
				k++;
				k += nBase64BufLen;
				k++;
				//////////////
				delete [] pszBase64Buf;
				pszBase64Buf = NULL;
				nBase64BufLen = 0;
				bBASE64 = FALSE;
				i--;
			}else{
			}
		}
	}
	if( bBASE64 && 0 < (i - nBgn) ){
		/* 2�o�C�g��Unicode������Ƃ����O���LO/HI�o�C�g������ */
		pszWork = (char*)(char*)&pUniBuf[nBgn];
		for( j = 0; j < (int)((i - nBgn) * sizeof( wchar_t )); j += 2 ){
			cWork = pszWork[j + 1];
			pszWork[j + 1] = pszWork[j];
			pszWork[j] = cWork;
		}
		/* Base64�G���R�[�h */
		pszBase64Buf = NULL;
		nBase64BufLen = 0;
		nBase64BufLen = MemBASE64_Encode(
			(char*)&pUniBuf[nBgn],			// �G���R�[�h�Ώۃf�[�^
			(i - nBgn) * sizeof( wchar_t ), // �G���R�[�h�Ώۃf�[�^��
			&pszBase64Buf,					// ���ʃf�[�^�i�[�������|�C���^�̃A�h���X
			-1,		// �G���R�[�h��̃f�[�^�������I��CRLF�Ő܂�Ԃ��ꍇ�̂P�s�ő啶���� (-1���w�肳�ꂽ�ꍇ�͐܂�Ԃ��Ȃ�)
			FALSE	// �p�f�B���O���邩
		);
//		MYTRACE_A( "pszBase64Buf=[%s]\n", pszBase64Buf );
		//////////////
		k++;
		k += nBase64BufLen;
		k++;
		//////////////
		delete [] pszBase64Buf;
		pszBase64Buf = NULL;
		nBase64BufLen = 0;
		bBASE64 = FALSE;
	}
	delete [] pUniBuf;

	pDes = new unsigned char[k + 1];
	memset( pDes, 0, k + 1 );
	k = 0;
	bBASE64 = FALSE;
	nBgn = 0;
	pUniBuf = (wchar_t*)m_pData;
	for( i = 0; i < nUniBufLen; ++i ){
		j = IsUTF7Direct( pUniBuf[i] );
		if( !bBASE64 ){
			if( 1 == j ){
				pDes[k] = (unsigned char)(pUniBuf[i] & 0x007f);
				k++;
			}else
			if( L'+' == pUniBuf[i] ){
				pDes[k    ] = '+';
				pDes[k + 1] = '-';
				k += 2;
			}else{
				bBASE64 = TRUE;
				nBgn = i;
			}
		}else{
			if( 1 == j ){
				/* 2�o�C�g��Unicode������Ƃ����O���LO/HI�o�C�g������ */
				pszWork = (char*)(char*)&pUniBuf[nBgn];
				for( j = 0; j < (int)((i - nBgn) * sizeof( wchar_t )); j += 2 ){
					cWork = pszWork[j + 1];
					pszWork[j + 1] = pszWork[j];
					pszWork[j] = cWork;
				}
				char*	pszBase64Buf;
				int		nBase64BufLen;
				/* Base64�G���R�[�h */
				nBase64BufLen = MemBASE64_Encode(
					(char*)&pUniBuf[nBgn],			// �G���R�[�h�Ώۃf�[�^
					(i - nBgn) * sizeof( wchar_t ), // �G���R�[�h�Ώۃf�[�^��
					&pszBase64Buf,					// ���ʃf�[�^�i�[�������|�C���^�̃A�h���X
					-1,		// �G���R�[�h��̃f�[�^�������I��CRLF�Ő܂�Ԃ��ꍇ�̂P�s�ő啶���� (-1���w�肳�ꂽ�ꍇ�͐܂�Ԃ��Ȃ�)
					FALSE	// �p�f�B���O���邩
				);
//				MYTRACE_A( "pszBase64Buf=[%s]\n", pszBase64Buf );
				//////////////
				pDes[k] = '+';
				k++;
				memcpy( &pDes[k], pszBase64Buf, nBase64BufLen );
				k += nBase64BufLen;
				pDes[k] = '-';
				k++;
				//////////////
				delete [] pszBase64Buf;
				bBASE64 = FALSE;
				i--;
			}else{
			}
		}
	}
	if( bBASE64 && 0 < (i - nBgn) ){
		/* 2�o�C�g��Unicode������Ƃ����O���LO/HI�o�C�g������ */
		pszWork = (char*)(char*)&pUniBuf[nBgn];
		for( j = 0; j < (int)((i - nBgn) * sizeof( wchar_t )); j += 2 ){
			cWork = pszWork[j + 1];
			pszWork[j + 1] = pszWork[j];
			pszWork[j] = cWork;
		}
		/* Base64�G���R�[�h */
		pszBase64Buf = NULL;
		nBase64BufLen = 0;
		nBase64BufLen = MemBASE64_Encode(
			(char*)&pUniBuf[nBgn],			// �G���R�[�h�Ώۃf�[�^
			(i - nBgn) * sizeof( wchar_t ), // �G���R�[�h�Ώۃf�[�^��
			&pszBase64Buf,					// ���ʃf�[�^�i�[�������|�C���^�̃A�h���X
			-1,		// �G���R�[�h��̃f�[�^�������I��CRLF�Ő܂�Ԃ��ꍇ�̂P�s�ő啶���� (-1���w�肳�ꂽ�ꍇ�͐܂�Ԃ��Ȃ�)
			FALSE	// �p�f�B���O���邩
		);
//		MYTRACE_A( "pszBase64Buf=[%s]\n", pszBase64Buf );
		//////////////
		pDes[k] = '+';
		k++;
		memcpy( &pDes[k], pszBase64Buf, nBase64BufLen );
		k += nBase64BufLen;
		pDes[k] = '-';
		k++;
		//////////////
		delete [] pszBase64Buf;
		pszBase64Buf = NULL;
		nBase64BufLen = 0;
		bBASE64 = FALSE;
	}
	SetString( (const char *)pDes, k );
	delete [] pDes;
	return;
}





/* ������ */
void CMemory::ToLower( void )
{
	unsigned char*	pBuf = (unsigned char*)m_pData;
	int				nBufLen = m_nDataLen;
	int				i;
	int				nCharChars;
	unsigned char	uc;
	for( i = 0; i < nBufLen; ++i ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pBuf, nBufLen, i );
		if( nCharChars == 1 ){
			uc = (unsigned char)tolower( pBuf[i] );
			pBuf[i] = uc;
		}else
		if( nCharChars == 2 ){
			/* �S�p�p�啶�����S�p�p������ */
			if( pBuf[i] == 0x82 && pBuf[i + 1] >= 0x60 && pBuf[i + 1] <= 0x79 ){
				pBuf[i] = pBuf[i];
				pBuf[i + 1] = pBuf[i + 1] + 0x21;
//@@@ 2001.02.03 Start by MIK: �M���V�������ϊ�
			//�啶��:0x839f�`0x83b6
			//������:0x83bf�`0x83d6
			}else if( pBuf[i] == 0x83 && pBuf[i + 1] >= 0x9f && pBuf[i + 1] <= 0xb6 ){
				pBuf[i] = pBuf[i];
				pBuf[i + 1] = pBuf[i + 1] + 0x20;
//@@@ 2001.02.03 End
//@@@ 2001.02.03 Start by MIK: ���V�A�����ϊ�
			//�啶��:0x8440�`0x8460
			//������:0x8470�`0x8491 0x847f���Ȃ��I
			}else if( pBuf[i] == 0x84 && pBuf[i + 1] >= 0x40 && pBuf[i + 1] <= 0x60 ){
				pBuf[i] = pBuf[i];
				if( pBuf[i + 1] >= 0x4f ){
					pBuf[i + 1] = pBuf[i + 1] + 0x31;
				}else{
					pBuf[i + 1] = pBuf[i + 1] + 0x30;
				}
//@@@ 2001.02.03 End
			}
		}
		if( nCharChars > 0 ){
			i += nCharChars - 1;
		}
	}
	return;
}





/* �啶�� */
void CMemory::ToUpper( void )
{
	unsigned char*	pBuf = (unsigned char*)m_pData;
	int				nBufLen = m_nDataLen;
	int				i;
	int				nCharChars;
	unsigned char	uc;
	for( i = 0; i < nBufLen; ++i ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pBuf, nBufLen, i );
		if( nCharChars == 1 ){
			uc = (unsigned char)toupper( pBuf[i] );
			pBuf[i] = uc;
		}else
		if( nCharChars == 2 ){
			/* �S�p�p���������S�p�p�啶�� */
			if( pBuf[i] == 0x82 && pBuf[i + 1] >= 0x81 && pBuf[i + 1] <= 0x9a ){
				pBuf[i] = pBuf[i];
				pBuf[i + 1] = pBuf[i + 1] - 0x21;
//@@@ 2001.02.03 Start by MIK: �M���V�������ϊ�
			//�啶��:0x839f�`0x83b6
			//������:0x83bf�`0x83d6
			}else if( pBuf[i] == 0x83 && pBuf[i + 1] >= 0xbf && pBuf[i + 1] <= 0xd6 ){
				pBuf[i] = pBuf[i];
				pBuf[i + 1] = pBuf[i + 1] - 0x20;
//@@@ 2001.02.03 End
//@@@ 2001.02.03 Start by MIK: ���V�A�����ϊ�
			//�啶��:0x8440�`0x8460
			//������:0x8470�`0x8491 0x847f���Ȃ��I
			}else if( pBuf[i] == 0x84 && pBuf[i + 1] >= 0x70 && pBuf[i + 1] <= 0x91 && pBuf[i + 1] != 0x7f ){
				pBuf[i] = pBuf[i];
				if( pBuf[i + 1] >= 0x7f ){
					pBuf[i + 1] = pBuf[i + 1] - 0x31;
				}else{
					pBuf[i + 1] = pBuf[i + 1] - 0x30;
				}
//@@@ 2001.02.03 End
			}
		}
		if( nCharChars > 0 ){
			i += nCharChars - 1;
		}
	}
	return;
}



/* �|�C���^�Ŏ����������̎��ɂ��镶���̈ʒu��Ԃ��܂� */
/* ���ɂ��镶�����o�b�t�@�̍Ō�̈ʒu���z����ꍇ��&pData[nDataLen]��Ԃ��܂� */
const char* CMemory::MemCharNext( const char* pData, int nDataLen, const char* pDataCurrent )
{
//#ifdef _DEBUG
//	CRunningTimer cRunningTimer( (const char*)"CMemory::MemCharNext" );
//#endif

	const char*	pNext;
	if( pDataCurrent[0] == '\0' ){
		pNext = pDataCurrent + 1;
	}else
	{
//		pNext = ::CharNext( pDataCurrent );
		if(
			/* SJIS�S�p�R�[�h��1�o�C�g�ڂ� */	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
			_IS_SJIS_1( (unsigned char)pDataCurrent[0] )
			&&
			/* SJIS�S�p�R�[�h��2�o�C�g�ڂ� */	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
			_IS_SJIS_2( (unsigned char)pDataCurrent[1] )
		){
			pNext = pDataCurrent + 2;
		}else{
			pNext = pDataCurrent + 1;
		}
	}

	if( pNext >= &pData[nDataLen] ){
		pNext = &pData[nDataLen];
	}
	return pNext;
}



/* �|�C���^�Ŏ����������̒��O�ɂ��镶���̈ʒu��Ԃ��܂� */
/* ���O�ɂ��镶�����o�b�t�@�̐擪�̈ʒu���z����ꍇ��pData��Ԃ��܂� */
const char* CMemory::MemCharPrev( const char* pData, int nDataLen, const char* pDataCurrent )
{
//#ifdef _DEBUG
//	CRunningTimer cRunningTimer( (const char*)"CMemory::MemCharPrev" );
//#endif


	const char*	pPrev;
	pPrev = ::CharPrev( pData, pDataCurrent );

//===1999.08.12  ���̂������ƁA�_���������B===============-
//
//	if( (pDataCurrent - 1)[0] == '\0' ){
//		pPrev = pDataCurrent - 1;
//	}else{
//		if( pDataCurrent - pData >= 2 &&
//			/* SJIS�S�p�R�[�h��1�o�C�g�ڂ� */	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
//			(
//			( (unsigned char)0x81 <= (unsigned char)pDataCurrent[-2] && (unsigned char)pDataCurrent[-2] <= (unsigned char)0x9F ) ||
//			( (unsigned char)0xE0 <= (unsigned char)pDataCurrent[-2] && (unsigned char)pDataCurrent[-2] <= (unsigned char)0xFC )
//			) &&
//			/* SJIS�S�p�R�[�h��2�o�C�g�ڂ� */	//Sept. 1, 2000 jepro '�V�t�g'��'S'�ɕύX
//			(
//			( (unsigned char)0x40 <= (unsigned char)pDataCurrent[-1] && (unsigned char)pDataCurrent[-1] <= (unsigned char)0x7E ) ||
//			( (unsigned char)0x80 <= (unsigned char)pDataCurrent[-1] && (unsigned char)pDataCurrent[-1] <= (unsigned char)0xFC )
//			)
//		){
//			pPrev = pDataCurrent - 2;
//		}else{
//			pPrev = pDataCurrent - 1;
//		}
//	}
//	if( pPrev < pData ){
//		pPrev = pData;
//	}
	return pPrev;
}



/* �R�[�h�ϊ� SJIS��Unicode */
void CMemory::SJISToUnicode( void )
{
	char*	pBufUnicode;
	int		nBufUnicodeLen;
	nBufUnicodeLen = CMemory::MemSJISToUnicode( &pBufUnicode, m_pData, m_nDataLen );
	SetString( pBufUnicode, nBufUnicodeLen );
	delete [] pBufUnicode;
	return;
}



/* �R�[�h�ϊ� SJIS��UnicodeBE */
void CMemory::SJISToUnicodeBE( void )
{
	char*	pBufUnicode;
	int		nBufUnicodeLen;
	nBufUnicodeLen = CMemory::MemSJISToUnicode( &pBufUnicode, m_pData, m_nDataLen );
	SetString( pBufUnicode, nBufUnicodeLen );
	delete [] pBufUnicode;
	SwapHLByte();
	return;
}



/* �R�[�h�ϊ� Unicode��SJIS */
void CMemory::UnicodeToSJIS( void )
{
	char*			pBufUnicode;
	int				nBufUnicodeLen;
	unsigned char*	pBuf;
	pBuf = (unsigned char*)m_pData;
//	BOM�̍폜�͂����ł͂��Ȃ��悤�ɕύX
//	�Ăяo�����őΏ����Ă������� 2002/08/30 Moca
		nBufUnicodeLen = CMemory::MemUnicodeToSJIS( &pBufUnicode, m_pData, m_nDataLen );
		SetString( pBufUnicode, nBufUnicodeLen );
		delete [] pBufUnicode;
	return;
}


/* �R�[�h�ϊ� UnicodeBE��SJIS */
void CMemory::UnicodeBEToSJIS( void ){

	SwapHLByte();
	UnicodeToSJIS();

	return;
}


/* ASCII&SJIS�������Unicode�ɕϊ� */
int CMemory::MemSJISToUnicode( char** ppBufUnicode, const char*pBuf, int nBufLen )
{
	int			i, j, k;
	wchar_t		wchar;
	char*		pBufUnicode;
	int			nCharChars;

	setlocale( LC_ALL, "Japanese" );
	i = 0;
	k = 0;
	// 2005-09-02 D.S.Koba GetSizeOfChar
	nCharChars = CMemory::GetSizeOfChar( pBuf, nBufLen, i );
	while( nCharChars > 0 && i < nBufLen ){
		i += nCharChars;
		k += 2;
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( pBuf, nBufLen, i );
	}
	pBufUnicode = new char[k + 1];
	i = 0;
	k = 0;
	// 2005-09-02 D.S.Koba GetSizeOfChar
	nCharChars = CMemory::GetSizeOfChar( pBuf, nBufLen, i );
	while( nCharChars > 0 && i < nBufLen ){
		j = mbtowc( &wchar, &pBuf[i], nCharChars );
		if( j == -1 || j == 0 ){
			pBufUnicode[k] = 0;
			pBufUnicode[k + 1] = pBuf[i];
			++i;
		}else{
#if 0
		// 2005.11.16 ���p�ɑς��Ȃ��قǑ��x�ቺ����̂Ŗ���������
			if( j != 2 ){
				MYTRACE_A( "%d�o�C�g��Unicode�����ɕϊ����ꂽ SJIS(?)=%x %x\n", j,pBuf[i],((nCharChars >= 2)?(pBuf[i + 1]):0) );
			}
#endif
			*((wchar_t*)&(pBufUnicode[k])) = wchar;
			i += j;
		}
		k += 2;
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( pBuf, nBufLen, i );
	}
	*ppBufUnicode = pBufUnicode;
	return k;
}





/* Unicode�������ASCII&SJIS�ɕϊ� */
int CMemory::MemUnicodeToSJIS( char** ppBufSJIS, const char*pBuf, int nBufLen )
{
	int			i, j, k;;
	char*		pBufSJIS;
	wchar_t*	pUniBuf;
	char		mbchar[4];

	setlocale( LC_ALL, "Japanese" );
	pUniBuf = (wchar_t*)pBuf;
	i = 0;
	k = 0;
	for( i = 0; i < nBufLen / (int)sizeof( wchar_t ); ++i ){
		j = wctomb( (char*)mbchar, pUniBuf[i] );
		if( -1 == j ){
			k += sizeof( wchar_t );
		}else{
			k += j;
		}
	}
	pBufSJIS = new char[k];

	k = 0;
	for( i = 0; i < nBufLen / (int)sizeof( wchar_t ); ++i ){
		j = wctomb( (char*)&(pBufSJIS[k]), pUniBuf[i] );
		if( -1 == j ){
			memcpy( &pBufSJIS[k], &pUniBuf[i], sizeof( wchar_t ) );
			k += sizeof( wchar_t );
		}else{
			k += j;
		}
	}
	*ppBufSJIS = pBufSJIS;
	return k;
}





/* ������u�� */
void CMemory::Replace( char* pszFrom, char* pszTo )
{
	CMemory		cmemWork;
	int			nFromLen = strlen( pszFrom );
	int			nToLen = strlen( pszTo );
	int			nBgnOld = 0;
	int			nBgn = 0;
	while( nBgn <= m_nDataLen - nFromLen ){
		if( 0 == memcmp( &m_pData[nBgn], pszFrom, nFromLen ) ){
			if( 0  < nBgn - nBgnOld ){
				cmemWork.AppendString( &m_pData[nBgnOld], nBgn - nBgnOld );
			}
			cmemWork.AppendString( pszTo, nToLen );
			nBgn = nBgn + nFromLen;
			nBgnOld = nBgn;
		}else{
			nBgn++;
		}
	}
	if( 0  < m_nDataLen - nBgnOld ){
		cmemWork.AppendString( &m_pData[nBgnOld], m_nDataLen - nBgnOld );
	}
	SetNativeData( &cmemWork );
	return;
}



/* ������u���i���{��l���Łj */
void CMemory::Replace_j( char* pszFrom, char* pszTo )
{
	CMemory		cmemWork;
	int			nFromLen = strlen( pszFrom );
	int			nToLen = strlen( pszTo );
	int			nBgnOld = 0;
	int			nBgn = 0;
	while( nBgn <= m_nDataLen - nFromLen ){
		if( 0 == memcmp( &m_pData[nBgn], pszFrom, nFromLen ) ){
			if( 0  < nBgn - nBgnOld ){
				cmemWork.AppendString( &m_pData[nBgnOld], nBgn - nBgnOld );
			}
			cmemWork.AppendString( pszTo, nToLen );
			nBgn = nBgn + nFromLen;
			nBgnOld = nBgn;
		}else{
			if( _IS_SJIS_1( (unsigned char)m_pData[nBgn] ) ) nBgn++;
			nBgn++;
		}
	}
	if( 0  < m_nDataLen - nBgnOld ){
		cmemWork.AppendString( &m_pData[nBgnOld], m_nDataLen - nBgnOld );
	}
	SetNativeData( &cmemWork );
	return;
}



/* ���������e�� */
int CMemory::IsEqual( CMemory& cmem1, CMemory& cmem2 )
{
	char*	psz1;
	char*	psz2;
	int		nLen1;
	int		nLen2;

	psz1 = cmem1.GetStringPtr( &nLen1 );
	psz2 = cmem2.GetStringPtr( &nLen2 );
	if( nLen1 == nLen2 ){
		if( 0 == memcmp( psz1, psz2, nLen1 ) ){
			return TRUE;
		}
	}
	return FALSE;
}





/* ���p���S�p */
void CMemory::ToZenkaku(
		int bHiragana,		/* 1== �Ђ炪�� 0==�J�^�J�i //2==�p����p 2001/07/30 Misaka �ǉ� */
		int bHanKataOnly	/* 1== ���p�J�^�J�i�ɂ̂ݍ�p����*/
)
{
	unsigned char*			pBuf = (unsigned char*)m_pData;
	int						nBufLen = m_nDataLen;
	int						i;
	int						nCharChars;
//	unsigned char			uc;
	unsigned short			usSrc;
	unsigned short			usDes;
	unsigned char*			pBufDes;
	int						nBufDesLen;
	static unsigned char*	pszHanKataSet = (unsigned char*)"���������������������������������������������������������������";
	static unsigned char*	pszDakuSet = (unsigned char*)"��������������������";
	static unsigned char*	pszYouSet = (unsigned char*)"�����";
	BOOL					bHenkanOK;

	pBufDes = new unsigned char[nBufLen * 2 + 1];
	if( NULL ==	pBufDes ){
		return;
	}
	nBufDesLen = 0;
	for( i = 0; i < nBufLen; ++i ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pBuf, nBufLen, i );
		if( nCharChars == 1){
			bHenkanOK = FALSE;
			if( bHanKataOnly ){	/* 1== ���p�J�^�J�i�ɂ̂ݍ�p���� */
				if( NULL != strchr( (const char *)pszHanKataSet, pBuf[i] ) ){
					bHenkanOK = TRUE;
				}
			}else{
				//! �p���ϊ��p�ɐV���ȏ�����t�� 2001/07/30 Misaka
				if( ( (unsigned char)0x20 <= pBuf[i] && pBuf[i] <= (unsigned char)0x7E ) ||
					( bHiragana != 2 && (unsigned char)0xA1 <= pBuf[i] && pBuf[i] <= (unsigned char)0xDF )
				){
					bHenkanOK = TRUE;
				}
			}
			if( bHenkanOK ){
				usSrc = pBuf[i];
				if( FALSE == bHiragana &&
					pBuf[i]		== (unsigned char)'�' &&
					pBuf[i + 1] == (unsigned char)'�' &&
					bHiragana != 2
				){
					usDes = (unsigned short)0x8394; /* �� */
					nCharChars = 2;
				}else {
					usDes = _mbbtombc( usSrc );
					/* ���� */
					if( bHiragana != 2 && pBuf[i + 1] == (unsigned char)'�' && NULL != strchr( (const char *)pszDakuSet, pBuf[i] ) ){
						usDes++;
						nCharChars = 2;
					}
					/* �X�� */
					//! �p���ϊ��p�ɐV���ȏ�����t�� 2001/07/30 Misaka
					//! bHiragana != 2 //�p���ϊ��t���O���I���ł͂Ȃ��ꍇ
					if( bHiragana != 2 && pBuf[i + 1] == (unsigned char)'�' && NULL != strchr( (const char *)pszYouSet, pBuf[i] ) ){
						usDes += 2;
						nCharChars = 2;
					}
				}

				if( bHiragana == 1 ){
					/* �Ђ炪�Ȃɕϊ��\�ȃJ�^�J�i�Ȃ�΁A�Ђ炪�Ȃɕϊ����� */
					if( (unsigned short)0x8340 <= usDes && usDes <= (unsigned short)0x837e ){	/* �@�`�~ */
						usDes-= (unsigned short)0x00a1;
					}else
					if( (unsigned short)0x8380 <= usDes && usDes <= (unsigned short)0x8393 ){	/* ���`�� */
						usDes-= (unsigned short)0x00a2;
					}
				}
				pBufDes[nBufDesLen]		= ( usDes & 0xff00 ) >>  8;
				pBufDes[nBufDesLen + 1] = ( usDes & 0x00ff );
				nBufDesLen += 2;
			}else{
				memcpy( &pBufDes[nBufDesLen], &pBuf[i], nCharChars );
				nBufDesLen += nCharChars;

			}
		}else
		if( nCharChars == 2 ){
			usDes = usSrc = pBuf[i + 1] | ( pBuf[i] << 8 );
			if( bHanKataOnly == 0 ){
				if( bHiragana == 1 ){//�p���ϊ���t���������߂ɐ��l�Ŏw�肵���@2001/07/30 Misaka
					/* �S�p�Ђ炪�Ȃɕϊ��\�ȑS�p�J�^�J�i�Ȃ�΁A�Ђ炪�Ȃɕϊ����� */
					if( (unsigned short)0x8340 <= usSrc && usSrc <= (unsigned short)0x837e ){	/* �@�`�~ */
						usDes = usSrc - (unsigned short)0x00a1;
					}else
					if( (unsigned short)0x8380 <= usSrc && usSrc <= (unsigned short)0x8393 ){	/* ���`�� */
						usDes = usSrc - (unsigned short)0x00a2;
					}
				}else if( bHiragana == 0 ){//�p���ϊ���t���������߂ɐ��l�Ŏw�肵���@2001/07/30 Misaka
					/* �S�p�J�^�J�i�ɕϊ��\�ȑS�p�Ђ炪�ȂȂ�΁A�J�^�J�i�ɕϊ����� */
					if( (unsigned short)0x829f <= usSrc && usSrc <= (unsigned short)0x82dd ){	/* ���`�� */
						usDes = usSrc + (unsigned short)0x00a1;
					}else
					if( (unsigned short)0x82de <= usSrc && usSrc <= (unsigned short)0x82f1 ){	/* �ށ`�� */
						usDes = usSrc + (unsigned short)0x00a2;
					}
				}
			}
			pBufDes[nBufDesLen]		= ( usDes & 0xff00 ) >> 8;
			pBufDes[nBufDesLen + 1] = ( usDes & 0x00ff );
			nBufDesLen += 2;
		}else{
			memcpy( &pBufDes[nBufDesLen], &pBuf[i], nCharChars );
			nBufDesLen += nCharChars;

		}
		if( nCharChars > 0 ){
			i += nCharChars - 1;
		}
	}
	pBufDes[nBufDesLen] = '\0';
	SetString( (const char *)pBufDes, nBufDesLen );
	delete [] pBufDes;


	return;
}



/*!	�S�p�����p
	nMode
		0x01	�J�^�J�i�ɉe���A��
		0x02	�Ђ炪�Ȃɉe���A��
		0x04	�p�����ɉe���A��
*/
#define TO_KATAKANA	0x01
#define TO_HIRAGANA	0x02
#define TO_EISU		0x04
void CMemory::ToHankaku(
		int nMode		/* 0==�J�^�J�i 1== �Ђ炪�� 2==�p����p */
)
{
	static unsigned char*	pszZenDAKU = (unsigned char*)"�������������������������Âłǂ΂тԂׂڃK�M�O�Q�S�U�W�Y�[�]�_�a�d�f�h�o�r�u�x�{��";
	static unsigned char*	pszZenYOU  = (unsigned char*)"�ς҂Ղ؂ۃp�s�v�y�|";

	/* ���� */
	unsigned char*			pBuf = (unsigned char*)m_pData;
	int						nBufLen = m_nDataLen;
	unsigned int			uiSrc;

	/* �o�� */
	unsigned char*			pBufDes = new unsigned char[nBufLen + 1];
	int						nBufDesLen = 0;
	unsigned int			uiDes;
	if( NULL ==	pBufDes ){
		return;
	}

	/* ��Ɨp */
	int						nCharChars;
	unsigned char			pszZen[3];	//	�S�p�����p�o�b�t�@
	int i;
	BOOL bHenkanOK;
	bool bInHiraKata = false;				// �O�̕������J�^�J�ior�Ђ炪�Ȃ������Ȃ�Atrue�Ƃ��A�����A���_�A�����_�𔼊p�֕ϊ��\�Ƃ���
	for( i = 0; i < nBufLen; ++i ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pBuf, nBufLen, i );
		if( nCharChars == 2 ){
			uiSrc = pBuf[i + 1] | ( pBuf[i] << 8 );
			
			bHenkanOK = FALSE;
			if( nMode == 0x00 ){	//	�ǂ�ȕ�����ok���[�h
				bHenkanOK = TRUE;
			}
			if( nMode & TO_KATAKANA ){	/* �J�^�J�i�ɍ�p���� */
				if( ( 0x8340 <= uiSrc && uiSrc <= 0x8396 )
				// 2007.01.26 maru ToZenkaku�̓���ɍ��킹�ăJ�i�L�����ϊ��ΏۂɊ܂߂�(�B�u�v�A�E)
				||( 0x8141==uiSrc || 0x8142==uiSrc || 0x8145==uiSrc || 0x8175==uiSrc || 0x8176==uiSrc) ){
					bHenkanOK = TRUE;
					bInHiraKata = true;
				} else {
					// 2003-04-30 ����� ���������ϊ��ł��Ȃ������̂��C��
					// �A���A�Ђ炪�ȁE�J�^�J�i�̋�ʂ��Ȃ������Ȃ̂Œ��O�̕����Ō��肷��
					if( bInHiraKata == true ) {
						// ����"�["(0x815b)�E���_(0x814a)�E�����_(0x814b)
						if( uiSrc == 0x815b || uiSrc == 0x814a || uiSrc == 0x814b ) {
							bHenkanOK = TRUE;
						} else {
							bInHiraKata = false;
						}
					}
				}
			}
			if( nMode & TO_HIRAGANA ){	/* �Ђ炪�Ȃɍ�p���� */
				if( 0x829F <= uiSrc && uiSrc <= 0x82F1 ){
					bHenkanOK = TRUE;
					bInHiraKata = true;
				} else {
					// 2003-04-30 ����� ���������ϊ��ł��Ȃ������̂��C��
					// �A���A�Ђ炪�ȁE�J�^�J�i�̋�ʂ��Ȃ������Ȃ̂Œ��O�̕����Ō��肷��
					if( bInHiraKata == true ) {
						// ����"�["(0x815b)�E���_(0x814a)�E�����_(0x814b)
						if( uiSrc == 0x815b || uiSrc == 0x814a || uiSrc == 0x814b ) {
							bHenkanOK = TRUE;
						} else {
							bInHiraKata = false;
						}
					}
				}
			}
			if ( nMode & TO_EISU ){		/* �p���ɍ�p���� */
				/* From Here 2007.01.16 maru 7bit��ASCII�͈͂�ΏۂƂ���悤�ɕύX */
				if( 0x8140 <= uiSrc && uiSrc <= 0x8197){
					switch(uiSrc){	/* �J�i�L�����ϊ�����Ȃ��悤�� */
					case 0x8141:	// �A
					case 0x8142:	// �B
					case 0x8145:	// �E
					case 0x814a:	// �J
					case 0x814b:	// �K
					case 0x815b:	// �[
					case 0x8175:	//�u
					case 0x8176:	// �v
						break;
					default:
						bHenkanOK = TRUE;
					}
				}
				else if( 0x824f <= uiSrc && uiSrc <= 0x829a){	//	�p����
					bHenkanOK = TRUE;
				}
				/* From Here 2007.01.16 maru 7bit��ASCII�͈͂�ΏۂƂ���悤�ɕύX */
			}
			if (bHenkanOK == TRUE){
				uiDes = _mbctombb( uiSrc );
				if( uiDes == uiSrc ){	//	�ϊ��s�\
					memcpy( &pBufDes[nBufDesLen], &pBuf[i], nCharChars );
					nBufDesLen += nCharChars;
				}else{
					pBufDes[nBufDesLen] = (unsigned char)uiDes;
					nBufDesLen++;

					memcpy( pszZen, &pBuf[i], 2 );
					pszZen[2] = '\0';
					/* �����T�� */
					if( NULL != strstr( (const char *)pszZenDAKU, (const char *)pszZen ) ){
						pBufDes[nBufDesLen] = (unsigned char)'�';
						nBufDesLen++;
					}else
					/* �X���T�� */
					if( NULL != strstr( (const char *)pszZenYOU,  (const char *)pszZen ) ){
						pBufDes[nBufDesLen] = (unsigned char)'�';
						nBufDesLen++;
					}
				}
			}
			else {
				memcpy( &pBufDes[nBufDesLen], &pBuf[i], nCharChars );
				nBufDesLen += nCharChars;
			}
		}else{
			memcpy( &pBufDes[nBufDesLen], &pBuf[i], nCharChars );
			nBufDesLen += nCharChars;
		}
		if( nCharChars > 0 ){
			i += nCharChars - 1;
		}
	}
	pBufDes[nBufDesLen] = '\0';
	SetString( (const char *)pBufDes, nBufDesLen );
	delete [] pBufDes;

}


/*
|| �t�@�C���̓��{��R�[�h�Z�b�g����
||
|| �y�߂�l�z
||	SJIS		0
||	JIS			1
||	EUC			2
||	Unicode		3
||	UTF-8		4
||	UTF-7		5
||	UnicodeBE	6
||	�G���[		-1
||	�G���[�ȊO�� ECodeType ���g��
*/
ECodeType CMemory::CheckKanjiCodeOfFile( const char* pszFile )
{
	HFILE					hFile;
	HGLOBAL					hgData;
	const unsigned char*	pBuf;
	int						nBufLen;
	ECodeType				nCodeType;

	/* �������m�� & �t�@�C���ǂݍ��� */
	hgData = NULL;
	hFile = _lopen( pszFile, OF_READ );
	if( HFILE_ERROR == hFile ){
		return CODE_ERROR;
	}
	nBufLen = _llseek( hFile, 0, FILE_END );
	_llseek( hFile, 0, FILE_BEGIN );
	if( nBufLen > CheckKanjiCode_MAXREADLENGTH ){
		nBufLen = CheckKanjiCode_MAXREADLENGTH;
	}
	
	if( 0 == nBufLen ){
		_lclose( hFile );
		return CODE_SJIS;
	}
	
	hgData = ::GlobalAlloc( GHND, nBufLen + 1 );
	if( NULL == hgData ){
		_lclose( hFile );
		return CODE_ERROR;
	}
	pBuf = (const unsigned char*)::GlobalLock( hgData );
	_lread( hFile, (void *)pBuf, nBufLen );
	_lclose( hFile );

	/* ���{��R�[�h�Z�b�g���� */
// From Here 2006.09.22  rastiv
	nCodeType = Charcode::DetectUnicodeBom( (const char *)pBuf, nBufLen );
	if( nCodeType == 0 ){
		// Unicode BOM �͌��o����܂���ł����D
		nCodeType = CheckKanjiCode( (const unsigned char *)pBuf, nBufLen );
	}
// To Here

	if( NULL != hgData ){
		::GlobalUnlock( hgData );
		::GlobalFree( hgData );
		hgData = NULL;
	}
	return nCodeType;
}


/*
|| ���{��R�[�h�Z�b�g����
||
|| �y�߂�l�z
||	SJIS		0
||	JIS			1
||	EUC			2
||	Unicode		3
||	UTF-8		4
||	UTF-7		5
||	UnicodeBE	6
*/

// 2006.12.16  rastiv   �A���S���Y��������D
ECodeType CMemory::CheckKanjiCode( const unsigned char* pBuf, int nBufLen )
{
	CESI cesi;
	WCCODE_INFO wci;
	MBCODE_INFO mbci;
	int nPt;	// 
	
	if( !cesi.ScanEncoding((const char *)pBuf, nBufLen) ){
		// �X�L�����Ɏ��s���܂����D
		return CODE_SJIS;  // ���f�t�H���g�����R�[�h��ԋp�D
	}
	
	nPt = cesi.DetectUnicode( &wci );
	if( 0 != nPt ){
		// UNICODE �����o����܂���.
		return wci.eCodeID;
	}//else{
		nPt = cesi.DetectMultibyte( &mbci );
		//nPt := ���L�o�C�g�� �| �s���o�C�g��
		if( 0 < nPt ){
			return mbci.eCodeID;
		}//else{
			return CODE_SJIS;  // ���f�t�H���g�����R�[�h��ԋp�D
		//}
	//}
}




/* �������ʁ�SJIS�R�[�h�ϊ� */
void CMemory::AUTOToSJIS( void )
{
	ECodeType	nCodeType;
	/*
	|| ���{��R�[�h�Z�b�g����
	||
	|| �y�߂�l�z
	||	SJIS		0
	||	JIS			1
	||	EUC			2
	||	Unicode		3
	||	UTF-8		4
	||	UTF-7		5
	||	UnicodeBE	6
	*/
	nCodeType = CheckKanjiCode( (const unsigned char*)m_pData, m_nDataLen );
	switch( nCodeType ){
	case CODE_JIS:			JIStoSJIS();break;		/* E-Mail(JIS��SJIS)�R�[�h�ϊ� */
	case CODE_EUC:			EUCToSJIS();break;		/* EUC��SJIS�R�[�h�ϊ� */
	case CODE_UNICODE:		UnicodeToSJIS();break;	/* Unicode��SJIS�R�[�h�ϊ� */
	case CODE_UTF8:			UTF8ToSJIS();break;		/* UTF-8��SJIS�R�[�h�ϊ� */
	case CODE_UTF7:			UTF7ToSJIS();break;		/* UTF-7��SJIS�R�[�h�ϊ� */
	case CODE_UNICODEBE:	UnicodeBEToSJIS();break;/* UnicodeBE��SJIS�R�[�h�ϊ� */
	}
	return;
}





/* TAB���� */
void CMemory::TABToSPACE( int nTabSpace	/* TAB�̕����� */ )
{
	const char*	pLine;
	int			nLineLen;
	char*		pDes;
	int			nBgn;
	int			i;
	int			nPosDes;
//	BOOL		bEOL;
	int			nPosX;
	int			nWork;
	CEol		cEol;
	nBgn = 0;
	nPosDes = 0;
	/* CRLF�ŋ�؂���u�s�v��Ԃ��BCRLF�͍s���ɉ����Ȃ� */
	while( NULL != ( pLine = GetNextLine( m_pData, m_nDataLen, &nLineLen, &nBgn, &cEol ) ) ){
		if( 0 < nLineLen ){
			nPosX = 0;
			for( i = 0; i < nLineLen; ++i ){
				if( TAB == pLine[i]	){
					nWork = nTabSpace - ( nPosX % nTabSpace );
					nPosDes += nWork;
					nPosX += nWork;
				}else{
					nPosDes++;
					nPosX++;
				}
			}
		}
		nPosDes += cEol.GetLen();
	}
	if( 0 >= nPosDes ){
		return;
	}
	pDes = new char[nPosDes + 1];
	nBgn = 0;
	nPosDes = 0;
	/* CRLF�ŋ�؂���u�s�v��Ԃ��BCRLF�͍s���ɉ����Ȃ� */
	while( NULL != ( pLine = GetNextLine( m_pData, m_nDataLen, &nLineLen, &nBgn, &cEol ) ) ){
		if( 0 < nLineLen ){
			nPosX = 0;
			for( i = 0; i < nLineLen; ++i ){
				if( TAB == pLine[i]	){
					nWork = nTabSpace - ( nPosX % nTabSpace );
					memset( (void*)&pDes[nPosDes], ' ', nWork );
					nPosDes += nWork;
					nPosX += nWork;
				}else{
					pDes[nPosDes] = pLine[i];
					nPosDes++;
					nPosX++;
				}
			}
		}
		memcpy( &pDes[nPosDes], cEol.GetValue(), cEol.GetLen() );
		nPosDes += cEol.GetLen();
	}
	pDes[nPosDes] = '\0';

	SetString( pDes, nPosDes );
	delete [] pDes;
	pDes = NULL;
	return;
}


//!�󔒁�TAB�ϊ�
/*!
	@param nTabSpace TAB�̕�����
	�P�Ƃ̃X�y�[�X�͕ϊ����Ȃ�

	@author Stonee
	@date 2001/5/27
*/
void CMemory::SPACEToTAB( int nTabSpace )
{
	const char*	pLine;
	int			nLineLen;
	char*		pDes;
	int			nBgn;
	int			i;
	int			nPosDes;
	int			nPosX;
	CEol		cEol;

	BOOL		bSpace = FALSE;	//�X�y�[�X�̏��������ǂ���
	int		j;
	int		nStartPos;

	nBgn = 0;
	nPosDes = 0;
	/* �ϊ���ɕK�v�ȃo�C�g���𒲂ׂ� */
	while( NULL != ( pLine = GetNextLine( m_pData, m_nDataLen, &nLineLen, &nBgn, &cEol ) ) ){
		if( 0 < nLineLen ){
			nPosDes += nLineLen;
		}
		nPosDes += cEol.GetLen();
	}
	if( 0 >= nPosDes ){
		return;
	}
	pDes = new char[nPosDes + 1];
	nBgn = 0;
	nPosDes = 0;
	/* CRLF�ŋ�؂���u�s�v��Ԃ��BCRLF�͍s���ɉ����Ȃ� */
	while( NULL != ( pLine = GetNextLine( m_pData, m_nDataLen, &nLineLen, &nBgn, &cEol ) ) ){
		if( 0 < nLineLen ){
			nPosX = 0;	// ��������i�ɑΉ�����\�����ʒu
			bSpace = FALSE;	//���O���X�y�[�X��
			nStartPos = 0;	// �X�y�[�X�̐擪
			for( i = 0; i < nLineLen; ++i ){
				if( SPACE == pLine[i] || TAB == pLine[i] ){
					if( bSpace == FALSE ){
						nStartPos = nPosX;
					}
					bSpace = TRUE;
					if( SPACE == pLine[i] ){
						nPosX++;
					}else if( TAB == pLine[i] ){
						nPosX += nTabSpace - (nPosX % nTabSpace);
					}
				}else{
					if( bSpace ){
						if( (1 == nPosX - nStartPos) && (SPACE == pLine[i - 1]) ){
							pDes[nPosDes] = SPACE;
							nPosDes++;
						} else{
							for( j = nStartPos / nTabSpace; j < (nPosX / nTabSpace); j++ ){
								pDes[nPosDes] = TAB;
								nPosDes++;
								nStartPos += nTabSpace - ( nStartPos % nTabSpace );
							}
							//	2003.08.05 Moca
							//	�ϊ����TAB��1������Ȃ��ꍇ�ɃX�y�[�X���l�߂�����
							//	�o�b�t�@���͂ݏo���̂��C��
							for( j = nStartPos; j < nPosX; j++ ){
								pDes[nPosDes] = SPACE;
								nPosDes++;
							}
						}
					}
					nPosX++;
					pDes[nPosDes] = pLine[i];
					nPosDes++;
					bSpace = FALSE;
				}
			}
			//for( ; i < nLineLen; ++i ){
			//	pDes[nPosDes] = pLine[i];
			//	nPosDes++;
			//}
			if( bSpace ){
				if( (1 == nPosX - nStartPos) && (SPACE == pLine[i - 1]) ){
					pDes[nPosDes] = SPACE;
					nPosDes++;
				} else{
					//for( j = nStartPos - 1; (j + nTabSpace) <= nPosX + 1; j+=nTabSpace ){
					for( j = nStartPos / nTabSpace; j < (nPosX / nTabSpace); j++ ){
						pDes[nPosDes] = TAB;
						nPosDes++;
						nStartPos += nTabSpace - ( nStartPos % nTabSpace );
					}
					//	2003.08.05 Moca
					//	�ϊ����TAB��1������Ȃ��ꍇ�ɃX�y�[�X���l�߂�����
					//	�o�b�t�@���͂ݏo���̂��C��
					for( j = nStartPos; j < nPosX; j++ ){
						pDes[nPosDes] = SPACE;
						nPosDes++;
					}
				}
			}
		}

		/* �s���̏��� */
		memcpy( &pDes[nPosDes], cEol.GetValue(), cEol.GetLen() );
		nPosDes += cEol.GetLen();
	}
	pDes[nPosDes] = '\0';

	SetString( pDes, nPosDes );
	delete [] pDes;
	pDes = NULL;
	return;
}





/* !��ʃo�C�g�Ɖ��ʃo�C�g����������

	@author Moca
	@date 2002/5/27
	
	@note	nBufLen ��2�̔{���łȂ��Ƃ��́A�Ō��1�o�C�g�͌�������Ȃ�
*/
void CMemory::SwapHLByte( void ){
	unsigned char *p;
	unsigned char ctemp;
	unsigned char *p_end;
	unsigned int *pdwchar;
	unsigned int *pdw_end;
	unsigned char*	pBuf;
	int			nBufLen;

	pBuf = (unsigned char*)GetStringPtr( &nBufLen );

	if( nBufLen < 2){
		return;
	}
	// �������̂���
	pdwchar = (unsigned int*)pBuf;
	if( (size_t)pBuf % 2 == 0){
		if( (size_t)pBuf % 4 == 2 ){
			ctemp = pBuf[0];
			pBuf[0]  = pBuf[1];
			pBuf[1]  = ctemp;
			pdwchar = (unsigned int*)(pBuf + 2);
		}
		pdw_end = (unsigned int*)(pBuf + nBufLen - sizeof(unsigned int));

		for(; pdwchar <= pdw_end ; ++pdwchar ){
			pdwchar[0] = ((pdwchar[0] & (unsigned int)0xff00ff00) >> 8) |
						 ((pdwchar[0] & (unsigned int)0x00ff00ff) << 8);
		}
	}
	p = (unsigned char*)pdwchar;
	p_end = pBuf + nBufLen - 2;
	
	for(; p <= p_end ; p+=2){
		ctemp = p[0];
		p[0]  = p[1];
		p[1]  = ctemp;
	}
	return;
}

/*
|| �o�b�t�@�T�C�Y�̒���
*/
void CMemory::AllocStringBuffer( int nNewDataLen )
{
	int		nWorkLen;
	char*	pWork = NULL;
	nWorkLen = nNewDataLen + 1;	/* 1�o�C�g�����������m�ۂ��Ă���(\0������) */
	if( m_nDataBufSize == 0 ){
		/* ���m�ۂ̏�� */
		pWork = (char*)malloc( nWorkLen );
		m_nDataBufSize = nWorkLen;
	}else{
		/* ���݂̃o�b�t�@�T�C�Y���傫���Ȃ����ꍇ�̂ݍĊm�ۂ��� */
		if( m_nDataBufSize < nWorkLen ){
			pWork = (char*)realloc( m_pData, nWorkLen );
			m_nDataBufSize = nWorkLen;
		}else{
			return;
		}
	}


	if( NULL == pWork ){
		::MYMESSAGEBOX(	NULL, MB_OKCANCEL | MB_ICONQUESTION | MB_TOPMOST, GSTR_APPNAME,
			"CMemory::AllocStringBuffer(nNewDataLen==%d)\n�������m�ۂɎ��s���܂����B\n", nNewDataLen
		);
		if( NULL != m_pData && 0 != nWorkLen ){
			/* �Â��o�b�t�@��������ď����� */
			free( m_pData );
			m_pData = NULL;
			m_nDataBufSize = 0;
			m_nDataLen = 0;
		}
		return;
	}
	m_pData = pWork;
	return;
}



/* �o�b�t�@�̓��e��u�������� */
void CMemory::SetString( const char* pData, int nDataLen )
{
	Empty();
	AllocStringBuffer( nDataLen );
	AddData( pData, nDataLen );
	return;
}



/* �o�b�t�@�̓��e��u�������� */
void CMemory::SetString( const char* pszData )
{
	int		nDataLen;
	nDataLen = strlen( pszData );

	Empty();
	AllocStringBuffer( nDataLen );
	AddData( pszData, nDataLen );
	return;
}


/* �o�b�t�@�̓��e��u�������� */
void CMemory::SetNativeData( const CMemory* pcmemData )
{
	char*	pData;
	int		nDataLen;
	pData = pcmemData->GetStringPtr( &nDataLen );
	Empty();
	AllocStringBuffer( nDataLen );
	AddData( pData, nDataLen );
	return;
}


/* �o�b�t�@�̍Ō�Ƀf�[�^��ǉ�����ipublic�����o�j*/
const char* CMemory::AppendString( const char* pData, int nDataLen )
{
	AllocStringBuffer( m_nDataLen + nDataLen );
	AddData( pData, nDataLen );
	return m_pData;
}
/* �o�b�t�@�̍Ō�Ƀf�[�^��ǉ�����ipublic�����o�j*/
void CMemory::AppendString( const char* pszData )
{
	int		nDataLen;
	nDataLen = strlen( pszData );
	AllocStringBuffer( m_nDataLen + nDataLen );
	AddData( pszData, nDataLen );
}
/* �o�b�t�@�̍Ō�Ƀf�[�^��ǉ�����ipublic�����o�j*/
void CMemory::AppendNativeData( const CMemory* pcmemData )
{
	char*	pData;
	int		nDataLen;
	pData = pcmemData->GetStringPtr( &nDataLen );
	AllocStringBuffer( m_nDataLen + nDataLen );
	AddData( pData, nDataLen );
}

void CMemory::Empty( void )
{
	if( m_pData != NULL ){
		free( m_pData );
		m_pData = NULL;
	}
	m_nDataBufSize = 0;
	m_pData = NULL;
	m_nDataLen = 0;
	return;
}

/*!
	@param[in] pData �ʒu�����߂���������̐擪
	@param[in] nDataLen ������
	@param[in] nIdx �ʒu(0�I���W��)

	@date 2005-09-02 D.S.Koba �쐬

	@note nIdx�͗\�ߕ����̐擪�ʒu�Ƃ킩���Ă��Ȃ���΂Ȃ�Ȃ��D
	2�o�C�g������2�o�C�g�ڂ�nIdx�ɗ^����Ɛ��������ʂ������Ȃ��D
*/
int CMemory::GetSizeOfChar( const char* pData, const int nDataLen, const int nIdx )
{
	if( nIdx >= nDataLen ){
		return 0;
	}else if( nIdx == (nDataLen - 1) ){
		return 1;
	}
	
	if( _IS_SJIS_1( reinterpret_cast<const unsigned char*>(pData)[nIdx] )
			&& _IS_SJIS_2( reinterpret_cast<const unsigned char*>(pData)[nIdx+1] ) ){
		return 2;
	}
	return 1;
}

/*[EOF]*/