#include "../include/fileSys.h"
#include "../include/Buf.h"
#include "../include/MyFsManager.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* SuperBlock�� */
SuperBlock::SuperBlock()
{

}
SuperBlock::~SuperBlock()
{

}

/* FileSystem�� */
FileSystem::FileSystem()
{
    this->g_spb = new SuperBlock();
}
FileSystem::~FileSystem()
{
    delete this->g_spb;
}

void FileSystem::Initialize()//��ʼ��
{
	this->bufferManager = &MyFsManager::Instance().GetBufferManager();

    this->LoadSuperBlock();
}

void FileSystem::LoadSuperBlock()//��ȡSuperBlock
{
	Buf *pBuf;

	for(int i=0;i<2;i++)
	{
        int *p = (int *)this->g_spb + i * 128;
        pBuf = this->bufferManager->Bread(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
		memcpy(p,(int *)pBuf->b_addr,512);
        this->bufferManager->Brelse(pBuf);
	}
    this->g_spb->s_flock = 0;
    this->g_spb->s_ilock = 0;
    this->g_spb->s_ronly = 0;
    this->g_spb->s_time  = time(0);
}

void FileSystem::Update()//ͬ��SuperBlock
{
    SuperBlock* sb = this->g_spb;
    Buf *pBuf;

    if(sb->s_fmod == 0 || sb->s_ilock != 0 || sb->s_flock != 0 || sb->s_ronly != 0)
    {
        return;
    }
    sb->s_fmod=0;
    sb->s_time = time(0);

    /*
     * Ϊ��Ҫд�ص�������ȥ��SuperBlock����һ�黺�棬���ڻ�����СΪ512�ֽڣ�
     * SuperBlock��СΪ1024�ֽڣ�ռ��2��������������������Ҫ2��д�������
     */
    for(int i=0;i<2;i++)
    {
        /* ��һ��pָ��SuperBlock�ĵ�0�ֽڣ��ڶ���pָ���512�ֽ� */
        int* p = (int *)sb + i * 128;
        /* ��Ҫд�뵽�豸dev�ϵ�SUPER_BLOCK_SECTOR_NUMBER + j������ȥ */
        pBuf = this->bufferManager->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
        memcpy((int *)pBuf->b_addr,p,512);
        this->bufferManager->Bwrite(pBuf);
    }
    //this->bufferManager->Bflush();
}

Inode* FileSystem::IAlloc()
{
    SuperBlock* sb = g_spb;
    Buf *pBuf;
    Inode *pNode;
    int inumber;//���䵽�Ŀ������inode���

    if(sb->s_ninode<=0)//superblockֱ�ӹ����inode�������ѿ�
    {
        inumber = 0;//������Ϊ0��inode���ã����Դ�1��ʼ
        for(int i=0;i<sb->s_isize;i++)
        {
            pBuf=this->bufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR+i);
            int *p = (int *)pBuf->b_addr;

            /* ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ�� */
            for(int j=0;j<FileSystem::INODE_NUMBER_PER_SECTOR;j++)
            {
                inumber++;
                int mode = *(p+j*sizeof(DiskInode)/sizeof(int));
                /* �����inode�ѱ�ռ�ã����ܼ������inode������ */
                if(mode!=0)
                    continue;
                /*
                 * ������inode��i_mode==0����ʱ������ȷ��
                 * ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
                 * ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
                 */
                if(MyFsManager::Instance().GetInodeTable().IsLoaded(inumber)==-1)
                {
                    sb->s_inode[sb->s_ninode++]=inumber;//�ڴ�û�ж�Ӧ����
                    if(sb->s_ninode>=100)//��������������������������
                        break;
                }
            }
            this->bufferManager->Brelse(pBuf);
            if(sb->s_ninode>=100)//��������������������������
                break;
        }
        /* ���������û���������κο������inode������NULL */
        if(sb->s_ninode<=0)
        {
            printf("No Space on Disk!!!\n");
            return NULL;
        }
    }
    /*
     * ���沿���Ѿ���֤������ϵͳ��û�п������Inode��
     * �������Inode�������бض����¼�������Inode�ı�š�
     */
    while(true)
    {
        inumber = sb->s_inode[--sb->s_ninode];
        pNode = MyFsManager::Instance().GetInodeTable().IGet(inumber);//����inode�����ڴ�
        if(pNode==NULL)//δ�ܷ��䵽�ڴ�
        {
            printf("Inode Table Overflow!!!\n");
            return NULL;
        }
        if(pNode->i_mode==0)//��տ���inode����
        {
            pNode->Clean();
            sb->s_fmod=1;
            return pNode;
        }
        else
        {
            MyFsManager::Instance().GetInodeTable().IPut(pNode);
            continue;
        }
    }
    return NULL;
}

void FileSystem::IFree(int number)
{
    SuperBlock *sb = g_spb;
    if(sb->s_ninode>=100)
        return;
    sb->s_inode[sb->s_ninode++] = number;
    sb->s_fmod = 1;
}

Buf* FileSystem::BAlloc()
{
	int blkno;
    SuperBlock* sb = g_spb;
	Buf *pBuf;

	blkno = sb->s_free[--sb->s_nfree];
	if(blkno==0)//��������Խ��
	{
		sb->s_nfree = 0;
		printf("No Space!!!\n");
		return NULL;
	}

	/* 
	 * ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��,
	 * ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]�С�
	 */
	if(sb->s_nfree<=0)
	{
		sb->s_flock++;
		pBuf = this->bufferManager->Bread(blkno);
		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int *)pBuf->b_addr;
		/* ���ȶ��������̿���s_nfree */
		sb->s_nfree = *p++;
		/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		memcpy(sb->s_free,p,400);
		this->bufferManager->Brelse(pBuf);
		sb->s_flock = 0;
	}
	/* ��ͨ����³ɹ����䵽һ���д��̿� */
	pBuf = this->bufferManager->GetBlk(blkno);
	this->bufferManager->CLrBuf(pBuf);
	sb->s_fmod = 1;//SuperBlock���޸�

	return pBuf;
}

void FileSystem::BFree(int blkno)
{
    SuperBlock* sb = g_spb;
	Buf* pBuf;

	sb->s_fmod = 1;
	/* ���Ϸ��� */
	if(blkno<FileSystem::DATA_ZONE_START_SECTOR || blkno>FileSystem::DATA_ZONE_END_SECTOR)
	{
		printf("Invalid Block Number!!!\n");
		return;
	}

	/* 
	 * �����ǰϵͳ���Ѿ�û�п����̿飬
	 * �����ͷŵ���ϵͳ�е�1������̿�
	 */
	if(sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0;	/* ʹ��0��ǿ����̿���������־ */
	}
	/* SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ���� */
	if(sb->s_nfree >= 100)
	{
		sb->s_flock = 1;
		/* 
		 * ʹ�õ�ǰBFree()������Ҫ�ͷŵĴ��̿飬���ǰһ��100������
		 * ���̿��������
		 */
		pBuf = this->bufferManager->GetBlk(blkno);	/* Ϊ��ǰ��Ҫ�ͷŵĴ��̿���仺�� */
		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int *)pBuf->b_addr;
		
        /* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
		*p++ = sb->s_nfree;
		/* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
		memcpy(p,sb->s_free,400);
		sb->s_nfree=0;
		/* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
		this->bufferManager->Bwrite(pBuf);

		sb->s_flock = 0;
	}
	sb->s_free[sb->s_nfree++] = blkno;	/* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
	sb->s_fmod = 1;

}
