#include "../include/MyFsManager.h"
#include "../include/Device.h"
#include "string.h"

BufferManager::BufferManager()
{

}
BufferManager::~BufferManager()
{

}


void BufferManager::Initialize()
{
	int i;
	Buf* bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	for(i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
        memset(this->Buffer[i],0,BUFFER_SIZE);
		bp->b_addr = this->Buffer[i];
		/* ��ʼ��NODEV���� */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;

		bp->b_flags = 0;
        //this->d_actf = NULL;
        //this->d_actl = NULL;

		Brelse(bp);
	}
	return ;
}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf *bp;
	Buf *dp = &(this->bFreeList);

	/* ���������������Ӧ���棬�򷵻ظû��� */
    for(bp = dp->b_forw;bp!=dp;bp = bp->b_forw)
	{
		if(bp->b_blkno!=blkno)
			continue;
        /* �����ɶ�����ȡ�� */
        //bp->av_back->av_forw = bp->av_forw;
        //bp->av_forw->av_back = bp->av_back;
        //bp->b_flags |= Buf::B_BUSY;
		return bp;
	}

    /* ȡ���ɶ��е�һ�����п� */
    /*for(bp = dp->b_forw;bp!=dp;bp = bp->b_forw)
    {
        if(bp->b_flags & Buf::B_BUSY)
            continue;
        break;
    }*/
    bp = this->bFreeList.av_forw;
    /* �����ɶ�����ȡ�� */
   // bp->av_back->av_forw = bp->av_forw;
    //bp->av_forw->av_back = bp->av_back;
   // bp->b_flags |= Buf::B_BUSY;

    bp->b_flags = Buf::B_BUSY;
    this->CLrBuf(bp);
//    bp->b_back->b_forw = bp->b_forw;
//	bp->b_forw->b_back = bp->b_back;

//	bp->b_forw = dp->b_forw;
//	bp->b_back = (Buf *)dp;
//	dp->b_forw->b_back = bp;
//    dp->b_forw = bp;
	bp->b_blkno = blkno;
	return bp;
}

void BufferManager::Brelse(Buf *bp)//�ͷŻ����
{
    bp->b_flags &= ~(Buf::B_BUSY);
    (this->bFreeList.av_back)->av_forw = bp;
    bp->av_back = this->bFreeList.av_back;
    bp->av_forw = &(this->bFreeList);
    this->bFreeList.av_back = bp;
	return ;
}

Buf* BufferManager::Bread(int blkno)
{
	Buf *bp;
	bp = this->GetBlk(blkno);

	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	this->Strategy(bp);
	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_flags |= Buf::B_WRITE;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	this->Strategy(bp);
	this->Brelse(bp);
	return ;
}

void BufferManager::CLrBuf(Buf *bp)
{
	int *pInt = (int *)bp->b_addr;
	for(unsigned int i=0;i<BufferManager::BUFFER_SIZE / sizeof(int);i++)
	{
		pInt[i] = 0;
	}
	return ;
}

void BufferManager::Bflush()
{
	Buf *bp;
	for(bp = this->bFreeList.av_forw; bp != &(this->bFreeList); bp = bp->av_forw)
	{
		/* �ҳ����ɶ����������ӳ�д�Ŀ� */
		if(bp->b_flags & Buf::B_DELWRI)
		{

            this->Bwrite(bp);
		}
	}
	return ;
}

int BufferManager::Strategy(Buf *bp)
{
	/* ���I/O�������Ƿ񳬳��˴������������� */
	if(bp->b_blkno >= BufferManager::NSECTOR)
	{
		/* ���ó����־ */
		bp->b_flags |= Buf::B_ERROR;
		/*
		 * ��������²�����ִ��I/O�����������൱��ģ������ж�
		 * ��������е���IODone()���ѵȴ�I/O���������Ľ��̡�
		 */
        bp->b_flags |= Buf::B_DONE;
		return 0;
	}

	/* ��bp����I/O������еĶ�β����ʱI/O�����Ѿ��˻�����������ʽ����bp->av_forw��־�������β */
	bp->av_forw = NULL;
	if(this->d_actf==NULL)
    {
        this->d_actf = bp;
    }
	else
	{
		this->d_actl->av_forw = bp;
	}
	this->d_actl = bp;

	this->Start();
   // bp->b_flags |= Buf::B_DONE;
	return 0;
}

void BufferManager::Start()
{
	Buf *bp;
	if((bp = this->d_actf)==0)
		return ;
    Disk& disk = MyFsManager::Instance().GetDisk();
	disk.DevStart(bp);
	return ;
}
