/********************************************************************

purpose:	
 *********************************************************************/
#ifdef WIN32
#include "stdafx.h"
#else
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#endif

#include "yt/util/crypt.h"
#include <netinet/in.h>

namespace yt
{
	void xtea(int32_t* v, // 64bit of data [in/out]
			int32_t* o, // 64bits of data [out]
			int32_t* k, // 128bit key [in]
			int32_t N) // Routine rounds [in]
	{
		uint32_t y = v[0];
		uint32_t z = v[1];
		uint32_t DELTA = 0x9e3779b9; // 0x00000000 - 0x61C88647 == 0x9e3779b9

		if(N>0)
		{
			// Encoding
			uint32_t limit = DELTA*N;
			uint32_t sum = 0;
			while(sum != limit)
			{
				y += (z<<4 ^ z>>5) + z ^ sum + k[sum&3];
				sum += DELTA;
				z += (y<<4 ^ y>>5) + y ^ sum + k[sum>>11&3];
			}
		}
		else
		{
			// Decoding
			uint32_t sum = DELTA*(-N);
			while(sum)
			{
				z -= (y<<4 ^ y>>5) + y ^ sum + k[sum>>11&3];
				sum -= DELTA;
				y -= (z<<4 ^ z>>5) + z ^ sum + k[sum&3];
			}
		}

		o[0] = y;
		o[1] = z;
	}

	/*****************************************************************************
	//	���ܺ��Buffer�ṹ
	//  ���������������Щ����������������������Щ��������Щ���������
	//  �� PadLength  ��  Padded Random BYTEs ��  Data  �� Zero s ��
	//  ���������������੤���������������������੤�������੤��������
	//  ��    1Byte   ��    PadLength Bytes   �� N Bytes�� 7Bytes ��
	//  ���������������ة����������������������ة��������ة���������
	// Pad�������ڽ�����Buffer���뵽8�ֽڶ���
	 ******************************************************************************/

#define ZERO_LENGTH 7
#define ENCRYPT_ROUNDS (32)
#define DECRYPT_ROUNDS (-32)
#define ENCRYPT_BLOCK_LENGTH_IN_BYTE (8)

	int CXTEA::Encrypt(const char* pbyInBuffer, int nInBufferLength, char* pbyOutBuffer, int nOutBufferLength, char arrbyKey[16] )
	{
		if(pbyInBuffer == NULL || nInBufferLength <= 0)
		{
			return 0;
		}

		// ������Ҫ�����Buffer��С
		int nPadDataZero = 1/*Pad Length*/ + nInBufferLength + ZERO_LENGTH;
		int nPadLength = nPadDataZero % ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		if(nPadLength != 0)
		{
			nPadLength = ENCRYPT_BLOCK_LENGTH_IN_BYTE - nPadLength;
		}

		int nTotalLength = nPadDataZero + nPadLength;

		if(nTotalLength > nOutBufferLength || pbyOutBuffer == NULL)
		{
			return 0;
		}

		const char* pbyInCursor = pbyInBuffer;
		char* pbyOutCurosr = pbyOutBuffer;

		memset(pbyOutBuffer, 0, nOutBufferLength);

		char arrbyFirst8Bytes[ENCRYPT_BLOCK_LENGTH_IN_BYTE] = {0};
		// Pad length, ֻʹ�������λ����5λ����������
		arrbyFirst8Bytes[0] = (((char)rand()) & 0xf8) | ((char)nPadLength);
		//arrbyFirst8Bytes[0] = ((char)nPadLength);

		// ����������Pad��
		for(int i =1; i<=nPadLength; ++i)
		{
			arrbyFirst8Bytes[i] = (char)rand();
			//arrbyFirst8Bytes[i] = (char)0;
		}

		// �ô��������ݲ�����һ������
		for(int i = 1+nPadLength; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
			arrbyFirst8Bytes[i] = *pbyInCursor++;
		}

		// ��һ�����ܿ�����������ģ����ں����������
		char* pbyLast8BytesCryptedData = pbyOutBuffer;
		const char* pbyLast8BytesPlainData = arrbyFirst8Bytes;

		// ��һ��Buffer������Ҫ������
		xtea((int32_t*)arrbyFirst8Bytes, (int32_t*)pbyOutCurosr, (int32_t*)arrbyKey, ENCRYPT_ROUNDS);
		//	xtea((Int32*)pbyOutCurosr, (Int32*)arrbyFirst8Bytes, (Int32*)arrbyKey, DECRYPT_ROUNDS);
		pbyOutCurosr += ENCRYPT_BLOCK_LENGTH_IN_BYTE;

		// ��������������ڲ�����InBuffer�ļ��ܹ���
		char arrbySrcBuffer[ENCRYPT_BLOCK_LENGTH_IN_BYTE] = {0};
		while((pbyInCursor - pbyInBuffer) < (nInBufferLength - 1))
		{
			memcpy(arrbySrcBuffer, pbyInCursor, ENCRYPT_BLOCK_LENGTH_IN_BYTE);
			// ����һ���������
			for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
			{
				arrbySrcBuffer[i] ^= pbyLast8BytesCryptedData[i];
			}
			xtea((int32_t*)arrbySrcBuffer, (int32_t*)pbyOutCurosr, (int32_t*)arrbyKey, ENCRYPT_ROUNDS);
			// ����һ���������
			for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
			{
				pbyOutCurosr[i] ^= pbyLast8BytesPlainData[i];
			}

			pbyLast8BytesCryptedData = pbyOutCurosr;
			pbyLast8BytesPlainData = pbyInCursor;

			pbyOutCurosr += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
			pbyInCursor += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		}

		// ��һ�κ���һ�α�ע�͵��Ĺ�����ͬ��ֻ�������޸�InBuffer������
		/*	while((pbyInCursor - pbyInBuffer) < (nInBufferLength - 1))
			{
		// ����һ���������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
		pbyInCursor[i] ^= pbyLast8BytesCryptedData[i];
		}
		xtea((Int32*)pbyInCursor, (Int32*)pbyOutCurosr, (Int32*)arrbyKey, ENCRYPT_ROUNDS);
		// ����һ���������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
		pbyOutCurosr[i] ^= pbyLast8BytesPlainData[i];
		}

		pbyLast8BytesCryptedData = pbyOutCurosr;
		pbyLast8BytesPlainData = pbyInCursor;

		pbyOutCurosr += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		pbyInCursor += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		}
		 */
		// ��β�� 1Byte���� + 7Byte У��
		char arrbyLast8Bytes[ENCRYPT_BLOCK_LENGTH_IN_BYTE] = {0};
		arrbyLast8Bytes[0] = *pbyInCursor;

		// ����һ���������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
			arrbyLast8Bytes[i] ^= pbyLast8BytesCryptedData[i];
		}
		xtea((int32_t*)arrbyLast8Bytes, (int32_t*)pbyOutCurosr, (int32_t*)arrbyKey, ENCRYPT_ROUNDS);
		// ����һ���������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
			pbyOutCurosr[i] ^= pbyLast8BytesPlainData[i];
		}

		return nTotalLength;
	}
	// TODO: OutBufferLength�ж�
	int CXTEA::Decrypt(const char* pbyInBuffer, int nInBufferLength, char* pbyOutBuffer, int nOutBufferLength, char arrbyKey[16] )
	{
		if(pbyInBuffer == NULL || nInBufferLength <= 0)
		{
			return false;
		}

		// Buffer����Ӧ�����ܱ� ENCRYPT_BLOCK_LENGTH_IN_BYTE ������
		if(nInBufferLength%ENCRYPT_BLOCK_LENGTH_IN_BYTE || nInBufferLength <= 8)
		{
			return 0;
		}

		const char* pbyInCursor = pbyInBuffer;
		char* pbyOutCursor = pbyOutBuffer;

		// �Ƚ����ǰ�����Pad��ENCRYPT_BLOCK_LENGTH_IN_BYTE��Byte
		char arrbyFirst8Bytes[ENCRYPT_BLOCK_LENGTH_IN_BYTE] = {0};
		xtea((int32_t*)pbyInCursor, (int32_t*)arrbyFirst8Bytes, (int32_t*)arrbyKey, DECRYPT_ROUNDS);
		pbyInCursor += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		// Pad����ֻ�����˵�һ���ֽڵ����3bit����5bit�������
		int nPadLength = arrbyFirst8Bytes[0] & 0x07;

		// ����ԭʼ���ݵĳ���
		int nPlainDataLength = nInBufferLength - 1/*PadLength Length*/ - nPadLength - ZERO_LENGTH;
		if(nPlainDataLength <= 0 || pbyOutBuffer == NULL)
		{
			return 0;
		}

		// OutBuffer����
		if(nPlainDataLength > nOutBufferLength)
		{
			return 0;
		}

		// ǰһ������ĺ����ģ����ں����������
		const char* pbyLast8BytesCryptedData = pbyInBuffer;
		char* pbyLast8BytesPlainData = arrbyFirst8Bytes;

		// ����һ����Pad��Ϣ֮��������Ƶ����Buffer
		for(int i=0; i < 7/*ENCRYPT_BLOCK_LENGTH_IN_BYTE - 1*/ - nPadLength; ++i)
		{
			*pbyOutCursor++ = arrbyFirst8Bytes[1+nPadLength + i];
		}

		// ���ܳ������һ����������п�
		// ͬ���ܹ��̣�����ע�͵��ģ��ǲ��Ķ�InBuffer��

		char arrbySrcBuffer[8] = {0};
		//for(int i=8; i<nInBufferLength-8; i+=8)
		while(pbyInCursor - pbyInBuffer < nInBufferLength - 8)
		{
			memcpy(arrbySrcBuffer, pbyInCursor, 8);
			// ����һ��8char�������
			for(int i=0; i<8; ++i)
			{
				arrbySrcBuffer[i] ^= pbyLast8BytesPlainData[i];
			}
			xtea((int32_t*)arrbySrcBuffer, (int32_t*)pbyOutCursor, (int32_t*)arrbyKey, DECRYPT_ROUNDS);
			// ����һ��8char�������
			for(int i=0; i<8; ++i)
			{
				pbyOutCursor[i] ^= pbyLast8BytesCryptedData[i];
			}

			pbyLast8BytesCryptedData = pbyInCursor;
			pbyLast8BytesPlainData = pbyOutCursor;

			pbyInCursor += 8;
			pbyOutCursor += 8;
		}

		// ֱ�Ӹ�InBuffer�İ汾
		/*	while(pbyInCursor - pbyInBuffer < nInBufferLength - ENCRYPT_BLOCK_LENGTH_IN_BYTE)
			{
		// ����һ��8Byte�������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
		pbyInCursor[i] ^= pbyLast8BytesPlainData[i];
		}
		xtea((Int32*)pbyInCursor, (Int32*)pbyOutCursor, (Int32*)arrbyKey, DECRYPT_ROUNDS);
		// ����һ��8Byte�������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
		pbyOutCursor[i] ^= pbyLast8BytesCryptedData[i];
		}

		pbyLast8BytesCryptedData = pbyInCursor;
		pbyLast8BytesPlainData = pbyOutCursor;

		pbyInCursor += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		pbyOutCursor += ENCRYPT_BLOCK_LENGTH_IN_BYTE;
		}
		 */
		// ���8Byte�� �����7Byte��У��
		char arrbyLast8Bytes[ENCRYPT_BLOCK_LENGTH_IN_BYTE] = {0};
		// ����һ��8Byte�������
		memcpy(arrbySrcBuffer, pbyInCursor, 8);
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
			arrbySrcBuffer[i] ^= pbyLast8BytesPlainData[i];
		}
		xtea((int32_t*)arrbySrcBuffer, (int32_t*)arrbyLast8Bytes, (int32_t*)arrbyKey, DECRYPT_ROUNDS);
		// ����һ��8Byte�������
		for(int i=0; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
			arrbyLast8Bytes[i] ^= pbyLast8BytesCryptedData[i];
		}

		// У������0
		for(int i=1; i<ENCRYPT_BLOCK_LENGTH_IN_BYTE; ++i)
		{
			if(arrbyLast8Bytes[i] != 0)
			{
				return 0;
			}
		}
		*pbyOutCursor = arrbyLast8Bytes[0];

		return nPlainDataLength;
	}




#define DELTA 0x9e3779b9 /* sqr(5)-1 * 2^31 */

	/// TEA��/���ܵ�Ԫ
	/*
	   @param	rounds IN	����������0ʱ�����ڼ��ܣ�С��0ʱ�����ڽ���	
	   @param	in IN		Ҫ��/���ܵ�64bit����
	   @param	out OUT		��ż�/���ܽ����64bit
	   @param	key	IN		��/���ܵ�key,128bit
	   @remark	�ú�����TEA�㷨�ĺ��ģ�һ��ֻ�ܼ�/����64bit�����ݣ����Ҳ��64bit������
	 */
	static void TEACore(unsigned int in[2], unsigned int out[2], unsigned int key[4], long rounds)
	{
		unsigned int y = in[0], z = in[1];
		unsigned int limit = 0, sum = 0;

		if(rounds > 0)	// encrypt
		{
			limit = DELTA * rounds;
			while(sum != limit)
			{
				y += ((z<<4)^(z>>5)) + (z^sum) + key[sum&3];
				sum += DELTA;
				z += ((y<<4)^(y>>5)) + (y^sum) + key[(sum>>11)&3];
			}
		}
		else	// decrypt
		{
			sum = DELTA * (-rounds);
			while(sum)
			{
				z -= ((y<<4)^(y>>5)) + (y^sum) + key[(sum>>11)&3];
				sum -= DELTA;
				y -= ((z<<4)^(z>>5)) + (z^sum) + key[sum&3];
			}
		}

		out[0] = y; out[1] = z;
	}

	bool TEAEncrypt(const char* pInBuffer, int nInBufferLen, char* pOutBuffer, int* pnOutBufferLen, char pKey[16]/* = DEF_KEY*/, unsigned int uRounds/* = 16*/)
	{
		if (nInBufferLen < 1 || !pnOutBufferLen)
		{
			return false;
		}

		/// padding
		const int nPadLen = ((nInBufferLen % 8) == 0 ? 0 : (8 - (nInBufferLen % 8)));

		int nBufferLenToEncrypt = nInBufferLen + nPadLen;

		if (*pnOutBufferLen < (nBufferLenToEncrypt + 1))	// check: *pnOutBufferLen is length enough
		{
			return false;
		}

		//char* pBufferToEncrypt = new char[nBufferLenToEncrypt];
		char pBufferToEncrypt[65535];
		memcpy(pBufferToEncrypt, pInBuffer, nInBufferLen);
		for (int i = 0; i < nPadLen; i++)	// pad with 0
		{
			pBufferToEncrypt[nInBufferLen + i] = 0;
		}

		/// core
		for (int i = 0; i < nBufferLenToEncrypt; i += 8)
		{
			TEACore((unsigned int*)(pBufferToEncrypt + i), (unsigned int*)(pOutBuffer + i + 1), (unsigned int*)pKey, uRounds);
		}

		pOutBuffer[0] = nPadLen;
		if (pnOutBufferLen)
		{
			*pnOutBufferLen = nBufferLenToEncrypt + 1;
		}

		//delete[] pBufferToEncrypt;
		//pBufferToEncrypt = NULL;

		return true;
	}

	bool TEADecrypt(const char* pInBuffer, int nInBufferLen, char* pOutBuffer, int* pnOutBufferLen, char pKey[16]/* = DEF_KEY*/, unsigned int uRounds/* = 16*/)
	{
		if (nInBufferLen < 9 || ((nInBufferLen - 1) % 8) != 0 || !pnOutBufferLen || *pnOutBufferLen < (nInBufferLen - 1))
		{
			return false;
		}

		for (int i = 1; i < nInBufferLen; i += 8)
		{
			TEACore((unsigned int*)(pInBuffer + i), (unsigned int*)(pOutBuffer + i - 1), (unsigned int*)pKey, -1 * uRounds);
		}

		const int nPadLen = pInBuffer[0];
		if (nPadLen > 0)
		{
			for (int i = 0; i < nPadLen; i++)
			{
				if (pOutBuffer[nInBufferLen - 1 - nPadLen + i] != 0)	// check: padding BYTEs MUST equal 0
				{
					return false;
				}
			}
		}

		if (*pnOutBufferLen)
		{
			*pnOutBufferLen = nInBufferLen - 1 - nPadLen;
		}

		return true;
	}

#define PIANYI 3
	bool StreamDecrypt(const char *inbuf,int inbuflen,char *outbuf,int &outbuflen,char key[16],int type)
	{
		char *outbuf2 = outbuf + PIANYI;
		int outbuflen2 = outbuflen - PIANYI;
		if(type == 1)
		{
			if(!TEADecrypt(inbuf + PIANYI,inbuflen - PIANYI,outbuf2,&outbuflen2,key))
				return false;
		}
		else
		{
			int ret = CXTEA::Decrypt(inbuf + PIANYI,inbuflen - PIANYI,outbuf2,outbuflen2,key);
			if(ret == 0)
				return false;
			else
				outbuflen2 = ret;
		}
		outbuflen2 += PIANYI;

		short packlen = htons((short)outbuflen2);
		memcpy(outbuf,&packlen,sizeof(short));
		outbuflen = outbuflen2;
		return true;
	}

	bool StreamEncrypt(const char *inbuf,int inbuflen,char *outbuf,int &outbuflen,char key[16],int type)
	{
		char *outbuf2 = outbuf + PIANYI;
		int outbuflen2 = outbuflen - PIANYI;
		if(type == 1)
		{
			if(!TEAEncrypt(inbuf + PIANYI,inbuflen - PIANYI,outbuf2,&outbuflen2,key))
				return false;
		}
		else
		{
			int ret = CXTEA::Encrypt(inbuf + PIANYI,inbuflen - PIANYI,outbuf2,outbuflen2,key);
			if(ret == 0)
				return false;
			else
				outbuflen2 = ret;
		}
		outbuflen2 += PIANYI;

		short packlen = htons((short)outbuflen2);
		memcpy(outbuf,&packlen,sizeof(short));
		if(type == 1)
			outbuf[2] = TEAFLAG;
		else
			outbuf[2] = XTEAFLAG;

		outbuflen = outbuflen2;
		return true;
	}
        /*bool StreamCompress(const char *inbuf,int inbuflen,char *outbuf,int &outbuflen)
	{
		unsigned char* outbuf2 = (unsigned char*)outbuf + PIANYI;
		unsigned long outbuflen2 = outbuflen - PIANYI;
		unsigned char *inbuf2 = (unsigned char*)inbuf + PIANYI;
		unsigned long inbuflen2 = inbuflen - PIANYI;
		if(compress(outbuf2,&outbuflen2,inbuf2,inbuflen2) != 0)
			return false;
		outbuflen2 += PIANYI;

		short packlen = htons((short)outbuflen2);
		memcpy(outbuf,&packlen,sizeof(short));
		outbuf[2] = COMPRESSFLAG;
		outbuflen = outbuflen2;

		return true;
	}
	bool StreamUnCompress(const char *inbuf,int inbuflen,char *outbuf,int &outbuflen)
	{
		unsigned char* outbuf2 = (unsigned char*)outbuf + PIANYI;
		unsigned long outbuflen2 = outbuflen - PIANYI;
		unsigned char *inbuf2 = (unsigned char*)inbuf + PIANYI;
		unsigned long inbuflen2 = inbuflen - PIANYI;

		if(uncompress(outbuf2,&outbuflen2,inbuf2,inbuflen2) != 0)
			return false;
	
		outbuflen2 += PIANYI;

		short packlen = htons((short)outbuflen2);
		memcpy(outbuf,&packlen,sizeof(short));
		outbuflen = outbuflen2;

		return true;
	}*/
}

