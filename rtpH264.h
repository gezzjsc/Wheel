#ifndef __RTPH264_H__
#define __RTPH264_H__
#include <string.h>
#include <stdio.h>
#ifndef NULL
#define NULL 0
#endif

typedef unsigned char byte;

int frameNum = 0;
bool kFrame = false;

typedef struct
{
	//LITTLE_ENDIAN 
	unsigned short   cc : 4;	/* CSRC count					*/
	unsigned short   x : 1;		/* header extension flag		*/
	unsigned short   p : 1;		/* padding flag					*/
	unsigned short   v : 2;		/* packet type					*/

	unsigned short   pt : 7;	/* payload type					*/
	unsigned short   m : 1;		/* marker bit					*/

	unsigned short    seq;		/* sequence number				*/
	unsigned long     ts;		/* timestamp					*/
	unsigned long     ssrc;		/* synchronization source		*/
} rtp_hdr_t;

class H264_RTP_UNPACK
{
#define RTP_VERSION 2 
#define BUF_SIZE (1024 * 500) 
public:
	H264_RTP_UNPACK(unsigned char H264PAYLOADTYPE = 96)
		: m_bSPSFound(false)
		, m_bWaitKeyFrame(true)
		, m_bPrevFrameEnd(false)
		, m_bAssemblingFrame(false)
		, m_wSeq(1234)
		, m_ssrc(0)
		, m_H264PAYLOADTYPE(H264PAYLOADTYPE)
	{
		m_pBuf = new byte[BUF_SIZE];
		if (m_pBuf == NULL)
		{
			return;
		}

		m_pEnd = m_pBuf + BUF_SIZE;
		m_pStart = m_pBuf;
		m_dwSize = 0;
	}

	~H264_RTP_UNPACK(void)
	{
		delete[] m_pBuf;
	}

	byte* Parse_RTP_Packet(byte *pBuf, unsigned short nSize, int *outSize)
    {

		if (nSize <= 12)
		{
			return NULL;
		}

		byte *cp = (byte*)&m_RTP_Header;
		cp[0] = pBuf[0];
		cp[1] = pBuf[1];

		m_RTP_Header.seq = pBuf[2];
		m_RTP_Header.seq <<= 8;
		m_RTP_Header.seq |= pBuf[3];

		m_RTP_Header.ts = pBuf[4];
		m_RTP_Header.ts <<= 8;
		m_RTP_Header.ts |= pBuf[5];
		m_RTP_Header.ts <<= 8;
		m_RTP_Header.ts |= pBuf[6];
		m_RTP_Header.ts <<= 8;
		m_RTP_Header.ts |= pBuf[7];

		m_RTP_Header.ssrc = pBuf[8];
		m_RTP_Header.ssrc <<= 8;
		m_RTP_Header.ssrc |= pBuf[9];
		m_RTP_Header.ssrc <<= 8;
		m_RTP_Header.ssrc |= pBuf[10];
		m_RTP_Header.ssrc <<= 8;
		m_RTP_Header.ssrc |= pBuf[11];

		byte *pPayload = pBuf + 12;
		unsigned long PayloadSize = nSize - 12;

		// Skip over any CSRC identifiers in the header:
		if (m_RTP_Header.cc)
		{
			long cc = m_RTP_Header.cc * 4;
			if (PayloadSize < cc)
			{
				return NULL;
			}

			PayloadSize -= cc;
			pPayload += cc;
		}

		// Check for (& ignore) any RTP header extension
		if (m_RTP_Header.x)
		{
			if (PayloadSize < 4)
			{
				return NULL;
			}

			PayloadSize -= 4;
			pPayload += 2;

			long l = pPayload[0];
			l <<= 8;
			l |= pPayload[1];
			pPayload += 2;
			l *= 4;

			if (PayloadSize < l)
			{
				return NULL;
			}

			PayloadSize -= l;
			pPayload += l;
		}

		// Discard any padding bytes:
		if (m_RTP_Header.p)
		{
			if (PayloadSize == 0)
			{
				return NULL;
			}

			long Padding = pPayload[PayloadSize - 1];
			if (PayloadSize < Padding)
			{
				return NULL;
			}

			PayloadSize -= Padding;
		}

		// Check the RTP version number (it should be 2): 
		if (m_RTP_Header.v != RTP_VERSION)
		{
			return NULL;
		}

		// Check the Payload Type. 
		if (m_RTP_Header.pt != m_H264PAYLOADTYPE)
		{
			return NULL;
		}

		
		int PayloadType = pPayload[0] & 0x1f;			//PayloadType:  1-23
		//				24   STAP-A
		//				28	 FU-A

		int NALType = PayloadType;
		if (NALType == 28)
		{
			if (PayloadSize < 2)		// FU indicator    FU header
			{
				return NULL;
			}
			NALType = pPayload[1] & 0x1f;
		}

		if (m_ssrc != m_RTP_Header.ssrc)
		{
			m_ssrc = m_RTP_Header.ssrc;
			SetLostPacket();
		}

		if (NALType == 0x07) // SPS 
		{
			m_bSPSFound = true;
		}

		if (!m_bSPSFound)
		{
			return NULL;
		}

		if (NALType == 0x07 || NALType == 0x08 || NALType == 0x06) // SPS PPS  SEI
		{
			m_wSeq = m_RTP_Header.seq;
			m_bPrevFrameEnd = true;
			pPayload -= 4;
			pPayload[0] = 0;
			pPayload[1] = 0;
			pPayload[2] = 0;
			pPayload[3] = 1;
			//*((unsigned long*)(pPayload)) = 0x01000000;
			*outSize = PayloadSize + 4;
			return pPayload;
		}

		if (m_bWaitKeyFrame)
		{
			if (m_RTP_Header.m) // frame end 
			{
				m_bPrevFrameEnd = true;
				if (!m_bAssemblingFrame)
				{
					m_wSeq = m_RTP_Header.seq;
					return NULL;
				}
			}

			if (!m_bPrevFrameEnd)
			{
				m_wSeq = m_RTP_Header.seq;
				return NULL;
			}
			else
			{
				if (NALType != 0x05) // KEY FRAME 
				{
					m_wSeq = m_RTP_Header.seq;
					m_bPrevFrameEnd = false;
					return NULL;
				}
			}
		}

		if (m_RTP_Header.seq != (unsigned short)(m_wSeq + 1))//lost packet 
		{
			m_wSeq = m_RTP_Header.seq;
			SetLostPacket();
			return NULL;
		}
		else
		{
			m_wSeq = m_RTP_Header.seq;
			m_bAssemblingFrame = true;

			if (PayloadType != 28) // whole NAL 
			{
				//*((unsigned long*)(m_pStart)) = 0x01000000;
                m_pStart[0] = 0;
                m_pStart[1] = 0;
                m_pStart[2] = 0;
                m_pStart[3] = 1;
				m_pStart += 4;
				m_dwSize += 4;
			}
			else // FU_A 
			{
				if (pPayload[1] & 0x80) // FU_A start 
				{
					//*((unsigned long*)(m_pStart)) = 0x01000000;
                    m_pStart[0] = 0;
                    m_pStart[1] = 0;
                    m_pStart[2] = 0;
                    m_pStart[3] = 1;
					m_pStart += 4;
					m_dwSize += 4;
					pPayload[1] = (pPayload[0] & 0xE0) | NALType;
					pPayload += 1;
					PayloadSize -= 1;
				}
				else
				{
					pPayload += 2;
					PayloadSize -= 2;
				}
			}

			if (m_pStart + PayloadSize < m_pEnd)
			{
				memcpy(m_pStart, pPayload, PayloadSize);
				m_dwSize += PayloadSize;
				m_pStart += PayloadSize;
			}
			else // memory overflow 
			{
				SetLostPacket();
				return NULL;
			}

			if (m_RTP_Header.m) // frame end 
			{
				*outSize = m_dwSize;
				m_pStart = m_pBuf;
				m_dwSize = 0;

                frameNum ++;

				if (NALType == 0x05) // KEY FRAME 
				{
                    kFrame = true;
					m_bWaitKeyFrame = false;
				}
                else
                    kFrame = false;

                return m_pBuf;
			}
			else
			{
                return NULL;
			}
		}
	}

	void SetLostPacket()
	{
		m_bSPSFound = false;
		m_bWaitKeyFrame = true;
		m_bPrevFrameEnd = false;
		m_bAssemblingFrame = false;
		m_pStart = m_pBuf;
		m_dwSize = 0;
	}

private:
	rtp_hdr_t m_RTP_Header;
	byte *m_pBuf;
	bool m_bSPSFound;
	bool m_bWaitKeyFrame;
	bool m_bAssemblingFrame;
	bool m_bPrevFrameEnd;
	byte *m_pStart;
	byte *m_pEnd;
	unsigned long m_dwSize;
	unsigned short m_wSeq;
	byte m_H264PAYLOADTYPE;
	unsigned long m_ssrc;
};



void unpackRtpH264(void *rtpData, unsigned int dataSize, char *fileName, int ignoreFrameNum)
{
    static int frameRate = -1;

	static H264_RTP_UNPACK unpack;
	static bool firstCall = true;
	int outSize;
	byte *pFrame = unpack.Parse_RTP_Packet((byte*)rtpData, dataSize, &outSize);

	if (pFrame != NULL)
    {
        if(kFrame) {
                if(frameNum > 15) {
                    frameRate = frameNum;
                }

            frameNum = 0;
        }


		if(firstCall){
			remove(fileName);
			firstCall = false;
		}

        bool needWriteFile = false;
        if(frameRate == -1) {
            needWriteFile = true;
        }
        else {
			if(ignoreFrameNum < 0)
				ignoreFrameNum = 0;
			else if(ignoreFrameNum >= frameRate)
				ignoreFrameNum = frameRate - 1;

            if(frameNum < frameRate - ignoreFrameNum)
            {
                needWriteFile = true;
            }

        }
        if(needWriteFile) {
            FILE *fp = NULL;
            //fp = fopen(fileName, "ab+");
            if (fopen_s(&fp, fileName, "ab+"))
            /*if (!fp)*/ {
                fprintf(stderr, "failed to open file:%s\n", fileName);
                return;
            }

            fwrite(pFrame, outSize, 1, fp);
            fclose(fp);
        }
		

	}
}
#endif
