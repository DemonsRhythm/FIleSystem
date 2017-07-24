#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "Buf.h"

class BufferManager
{
public:
	static const int NBUF = 15; //������ƿ顢������������
	static const int BUFFER_SIZE = 512;	//��������С�� ���ֽ�Ϊ��λ
	static const int NSECTOR = 40956;	//��������������

public:
	BufferManager();
	~BufferManager();
	void Initialize();

	Buf* GetBlk(int blkno);	//����һ�黺�棬���ڶ�д�豸dev�ϵ��ַ���blkno
	void Brelse(Buf* bp);	//�ͷŻ�����ƿ�buf
	Buf* Bread(int blkno);	//��һ�����̿顣blknoΪĿ����̿��߼����
	void Bwrite(Buf* bp);	//дһ�����̿�

	void CLrBuf(Buf *bp);	//��ջ���������
	void Bflush();			//��devָ���豸�������ӳ�д�Ļ���ȫ�����������
    int  Strategy(Buf *bp); //IO

    void Start();           //����IO

private:
	Buf bFreeList;			//���ɻ�����п��ƿ�
	Buf m_Buf[NBUF];		//������ƿ�����
	unsigned char Buffer[NBUF][BUFFER_SIZE];	//���������� 
	/* I/O������� */
	/* I/O������� */
    Buf* d_actf;
    Buf* d_actl;
};

#endif
