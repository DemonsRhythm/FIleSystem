#include "../include/INode.h"
#include "../include/MyFsManager.h"

#include <string.h>
#include <stdio.h>
#include <algorithm>

Inode::Inode()
{
    /* ���Inode�����е����� */
        // this->Clean();
        /* ȥ��this->Clean();�����ɣ�
         * Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
         * �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
         * ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
         * �����ʼ��Ϊ��Чֵ��
         */
    /* ��Inode����ĳ�Ա������ʼ��Ϊ��Чֵ */
        this->i_flag = 0;
        this->i_mode = 0;
        this->i_count = 0;
        this->i_nlink = 0;
        this->i_number = -1;
        this->i_uid = -1;
        this->i_gid = -1;
        this->i_size = 0;
        for(int i = 0; i < 10; i++)
        {
            this->i_addr[i] = 0;
        }
}

Inode::~Inode()
{

}

int Inode::Bmap(int lbn)
{
    Buf *pFirstBuf;
    Buf *pSecondBuf;
    int phyBlkno;   //ת����������̿��
    int *iTable;    //���ڷ�����������һ�μ�ӡ����μ��������
    int index;
    BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();
    FileSystem& fileSys = MyFsManager::Instance().GetFileSystem();

    /*
     * Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
     * (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
     *
     * (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
     * �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
     *
     * (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
     * �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
     * (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
     */
    if(lbn >= Inode::HUGE_FILE_BLOCK)
    {
        printf("Exceed of MAX FILE SIZE!!!\n");
        return 0;
    }

    if(lbn < 6)//С���ļ�
    {
        phyBlkno = this->i_addr[lbn];
        /*
         * ������߼���Ż�û����Ӧ�������̿����֮��Ӧ�������һ������顣
         * ��ͨ�������ڶ��ļ���д�룬��д��λ�ó����ļ���С�����Ե�ǰ
         * �ļ���������д�룬����Ҫ�������Ĵ��̿飬��Ϊ֮�����߼����
         * �������̿��֮���ӳ�䡣
         */
        if(phyBlkno==0 && (pFirstBuf=fileSys.BAlloc())!=NULL)
        {
            /*
            * ��Ϊ����ܿ������ϻ�Ҫ�õ��˴��·�������ݿ飬���Բ��������������
            * �����ϣ����ǽ�������Ϊ�ӳ�д��ʽ���������Լ���ϵͳ��I/O������
            */
            bufMgr.Bwrite(pFirstBuf);
            phyBlkno = pFirstBuf->b_blkno;
            /* ���߼����ӳ�䵽������ */
            this->i_addr[lbn] = phyBlkno;
            this->i_flag |= Inode::IUPD;
        }
        return phyBlkno;
    }
    else//���͡������ļ�
    {
        /* �����߼����lbn��Ӧ��i_addr[]�е����� */
        if(lbn<Inode::LARGE_FILE_BLOCK)// �����ļ�: ���Ƚ���7 - (128 * 2 + 6)���̿�֮��
        {
            index = (lbn-Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
        }
        else//�����ļ�: ���Ƚ���263 - (128 * 128 * 2 + 128 * 2 + 6)���̿�֮��
        {
            index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
        }

        phyBlkno = this->i_addr[index];
        if(phyBlkno==0)//��������Ӧ�ļ���������
        {
            this->i_flag |= Inode::IUPD;
            /* ������п��ż�������� */
            if((pFirstBuf=fileSys.BAlloc())==NULL)
            {
                return 0;//�޿�������
            }
            this->i_addr[index] = pFirstBuf->b_blkno;
        }
        else
            pFirstBuf = bufMgr.Bread(phyBlkno);
        /* ��ȡ�������׵�ַ */
        iTable = (int *)pFirstBuf->b_addr;
        if(index>=8)
        {
            /*
            * ���ھ����ļ��������pFirstBuf���Ƕ��μ��������
            * ��������߼���ţ����ɶ��μ���������ҵ�һ�μ��������
            */
            index = ((lbn-Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;

            /* iTableָ�򻺴��еĶ��μ������������Ϊ�㣬������һ�μ�������� */
            phyBlkno = iTable[index];

            if(phyBlkno==0)
            {
                unsigned char Buffer1[BufferManager::BUFFER_SIZE];
                memcpy(Buffer1,pFirstBuf->b_addr,BufferManager::BUFFER_SIZE);
                int tblkno1 = pFirstBuf->b_blkno;
                if((pSecondBuf=fileSys.BAlloc())==NULL)
                {
                    /* ����һ�μ����������̿�ʧ�ܣ��ͷŻ����еĶ��μ��������Ȼ�󷵻� */
                    bufMgr.Brelse(pFirstBuf);
                    return 0;
                }
                unsigned char Buffer2[BufferManager::BUFFER_SIZE];
                memcpy(Buffer2,pSecondBuf->b_addr,BufferManager::BUFFER_SIZE);
                int tblkno2 = pSecondBuf->b_blkno;

                iTable = (int *)&Buffer1;
                iTable[index] = pSecondBuf->b_blkno;
                memcpy(pFirstBuf->b_addr,Buffer1,BufferManager::BUFFER_SIZE);
                pFirstBuf->b_blkno=tblkno1;
                bufMgr.Bwrite(pFirstBuf);

                memcpy(pSecondBuf->b_addr,Buffer2,BufferManager::BUFFER_SIZE);
                pSecondBuf->b_blkno = tblkno2;
                pFirstBuf = pSecondBuf;
                iTable = (int *)pSecondBuf->b_addr;//ָ��һ�μ��������
            }
            else
            {
                /* �ͷŶ��μ��������ռ�õĻ��棬������һ�μ�������� */
                bufMgr.Brelse(pFirstBuf);
                pSecondBuf = bufMgr.Bread(phyBlkno);

                pFirstBuf = pSecondBuf;
                iTable = (int *)pSecondBuf->b_addr;//ָ��һ�μ��������
            }

        }
        /* �����߼����lbn����λ��һ�μ���������еı������index */
        if(lbn<Inode::LARGE_FILE_BLOCK)
            index = (lbn-Inode::SMALL_FILE_BLOCK)%Inode::ADDRESS_PER_INDEX_BLOCK;
        else
            index = (lbn-Inode::LARGE_FILE_BLOCK)%Inode::ADDRESS_PER_INDEX_BLOCK;

        unsigned char Buffer[BufferManager::BUFFER_SIZE];
        memcpy(Buffer,pFirstBuf->b_addr,BufferManager::BUFFER_SIZE);
        int tblkno = pFirstBuf->b_blkno;
        if((phyBlkno=iTable[index])==0 && (pSecondBuf=fileSys.BAlloc())!=NULL)
        {
            /* �����䵽���ļ������̿�ŵǼ���һ�μ���������� */
            phyBlkno = pSecondBuf->b_blkno;
            /* �������̿顢���ĺ��һ�μ����������������� */
            bufMgr.Bwrite(pSecondBuf);
            memcpy(pFirstBuf->b_addr,Buffer,BufferManager::BUFFER_SIZE);
            iTable[index] = phyBlkno;
            pFirstBuf->b_blkno=tblkno;
            bufMgr.Bwrite(pFirstBuf);
        }
        else
        {
            memcpy(pFirstBuf->b_addr,Buffer,BufferManager::BUFFER_SIZE);
            pFirstBuf->b_blkno=tblkno;
            bufMgr.Brelse(pFirstBuf);//�ͷ�һ�μ��������ռ�û���
        }
        return phyBlkno;
    }
}

void Inode::IUpdate(int time)
{
    Buf *pBuf;
    DiskInode diskInode;
    FileSystem& fileSys = MyFsManager::Instance().GetFileSystem();
    BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();

    /* ��IUPD��IACC��־֮һ�����ã�����Ҫ������ӦDiskInode
     * Ŀ¼����������������;����Ŀ¼�ļ���IACC��IUPD��־ */
    if((this->i_flag & (Inode::IUPD | Inode::IACC))!=0)
    {
        if(fileSys.g_spb->s_ronly != 0)
            return;//ֻ���ļ�ϵͳ
        pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + this->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

        diskInode.d_mode = this->i_mode;
        diskInode.d_nlink = this->i_nlink;
        diskInode.d_uid = this->i_uid;
        diskInode.d_gid = this->i_gid;
        diskInode.d_size = this->i_size;
        for(int i=0;i<10;i++)
            diskInode.d_addr[i] = this->i_addr[i];
        diskInode.d_mtime = time;

        /* ��pָ�򻺴����оɵ����inode��ƫ��λ�� */
        unsigned char* p = pBuf->b_addr+(this->i_number%FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
        DiskInode* pNode = &diskInode;
        memcpy((int *)p,(int *)pNode,sizeof(DiskInode));
        bufMgr.Bwrite(pBuf);
    }
}

void Inode::ITrunc()
{
    FileSystem& fileSys = MyFsManager::Instance().GetFileSystem();
    BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();
    /* ����FILO��ʽ�ͷţ��Ծ���ʹ��SuperBlock�м�¼�Ŀ����̿��������
     *
     * Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
     * (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
     *
     * (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
     * �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
     *
     * (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
     * �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
     * (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
     */
    for(int i=9;i>=0;i--)
    {
        if(this->i_addr[i]!=0)
        {
            if(i>=6&&i<=9)
            {
                Buf* pFirstBuf = bufMgr.Bread(this->i_addr[i]);
                int* pFirst = (int *)pFirstBuf->b_addr;
                unsigned char Buffer1[BufferManager::BUFFER_SIZE];
                memcpy(Buffer1,pFirstBuf->b_addr,BufferManager::BUFFER_SIZE);
                int tblkno1 = pFirstBuf->b_blkno;
                /* ÿ�ż���������¼ 512/sizeof(int) = 128�����̿�ţ�������ȫ��128�����̿� */
                for(int j=128-1;j>=0;j--)
                {
                    if(pFirst[j]!=0)
                    {
                        if(i>=8&&i<=9)
                        {
                            Buf* pSecondBuf = bufMgr.Bread((pFirst[j]));
                            int* pSecond = (int *)pSecondBuf->b_addr;
                            for(int k=128-1;k>=0;k--)
                            {
                                if(pSecond[k]!=0)
                                    fileSys.BFree(pSecond[k]);
                            }
                            bufMgr.Brelse(pSecondBuf);
                        }//end if
                        memcpy(pFirstBuf->b_addr,Buffer1,BufferManager::BUFFER_SIZE);
                        pFirstBuf->b_blkno=tblkno1;
                        pFirst = (int *)&Buffer1;
                        fileSys.BFree(pFirst[j]);
                    }//end if
                }//end for
                bufMgr.Brelse(pFirstBuf);
            }//end if
            fileSys.BFree(this->i_addr[i]);
            this->i_addr[i] = 0;//0��ʾ������������
        }
    }

    /* �̿��ͷ���ϣ��ļ���С���� */
    this->i_size = 0;
    /* ����IUPD��־λ����ʾ���ڴ�Inode��Ҫͬ������Ӧ���Inode */
    this->i_flag |= Inode::IUPD;
    /* ����ļ���־ ��ԭ����RWXRWXRWX����*/
    this->i_mode &= ~(Inode::ILARG & Inode::IRWXU & Inode::IRWXG & Inode::IRWXO);
    this->i_nlink = 1;
}

void Inode::Clean()
{
    /*
     * Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
     * �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
     * ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
     * �����ʼ��Ϊ��Чֵ��
     */
    this->i_mode = 0;
    this->i_nlink = 0;
    this->i_uid = -1;
    this->i_gid = -1;
    this->i_size = 0;
    for(int i=0;i<10;i++)
        this->i_addr[i]=0;
}

void Inode::ICopy(Buf *bp, int inumber)
{
    DiskInode diskInode;
    DiskInode* pNode = &diskInode;

    unsigned char* p = bp->b_addr + (inumber%FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
    memcpy((int *)pNode,(int *)p,sizeof(DiskInode));

    /* �����Inode����dInode����Ϣ���Ƶ��ڴ�Inode�� */
    this->i_mode = diskInode.d_mode;
    this->i_nlink = diskInode.d_nlink;
    this->i_uid = diskInode.d_uid;
    this->i_gid = diskInode.d_gid;
    this->i_size = diskInode.d_size;
    for(int i = 0; i < 10; i++)
    {
        this->i_addr[i] = diskInode.d_addr[i];
    }

}

void Inode::ReadI()
{
    int lbn;        //�߼����
    int phyBlkno;   //������
    int offset;     //��ǰ�ַ�������ʼ����λ��
    int nbytes;     //�������û�Ŀ�����ֽ�����
    Buf* pBuf;
    BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();
    FileManager& fileMgr = MyFsManager::Instance().GetFileManager();

    if(fileMgr.m_Count==0)
        return ;
    this->i_flag |= Inode::IACC;

    /* һ��һ���ַ���ض�������ȫ�����ݣ�ֱ���ļ�β */
    while(fileMgr.m_Count!=0)
    {
        lbn = phyBlkno = fileMgr.m_Offset/Inode::BLOCK_SIZE;
        offset = fileMgr.m_Offset % Inode::BLOCK_SIZE;
        /* ���͵��û������ֽ�������ȡ�������ʣ���ֽ����뵱ǰ�ַ�������Ч�ֽ�����Сֵ */
        nbytes = std::min(Inode::BLOCK_SIZE-offset,fileMgr.m_Count);
        int remain = this->i_size - fileMgr.m_Offset;
        if(remain<=0)//�����ļ�β
            return ;
        nbytes = std::min(nbytes,remain);
        if((phyBlkno=this->Bmap(lbn))==0)
            return ;
        pBuf = bufMgr.Bread(phyBlkno);
        unsigned char* start = pBuf->b_addr + offset;//������������ʼλ��
        memcpy(fileMgr.m_Base,start,nbytes);

        /* ���¶�дλ�� */
        fileMgr.m_Base += nbytes;
        fileMgr.m_Offset += nbytes;
        fileMgr.m_Count -= nbytes;

        bufMgr.Brelse(pBuf);
    }
}

void Inode::WriteI()
{
    int lbn;        //�߼����
    int phyBlkno;   //������
    int offset;     //��ǰ�ַ�������ʼ����λ��
    int nbytes;     //�������û�Ŀ�����ֽ�����
    Buf* pBuf;
    BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();
    FileManager& fileMgr = MyFsManager::Instance().GetFileManager();

    this->i_flag |= (Inode::IACC | Inode::IUPD);

    if(fileMgr.m_Count==0)
        return ;

    while(fileMgr.m_Count!=0)
    {
        lbn = fileMgr.m_Offset / Inode::BLOCK_SIZE;
        offset = fileMgr.m_Offset % Inode::BLOCK_SIZE;
        nbytes = std::min(Inode::BLOCK_SIZE-offset,fileMgr.m_Count);

        if((phyBlkno=this->Bmap(lbn))==0)
            return ;

        if(nbytes==Inode::BLOCK_SIZE)//������ݸպù�һ���ַ��飬��Ϊ����仺��
            pBuf=bufMgr.GetBlk(phyBlkno);
        else//����һ���ַ��飬�ȶ���д���������ַ����Ա�������Ҫ��д�����ݣ�
            pBuf=bufMgr.Bread(phyBlkno);

        unsigned char* start = pBuf->b_addr + offset;
        memcpy(start,fileMgr.m_Base,nbytes);

        /* ���¶�дλ�� */
        fileMgr.m_Base += nbytes;
        fileMgr.m_Offset += nbytes;
        fileMgr.m_Count -= nbytes;

        if((fileMgr.m_Offset%Inode::BLOCK_SIZE)==0)//д��һ���ַ���
            bufMgr.Bwrite(pBuf);
        else
            bufMgr.Bwrite(pBuf);

        //�ļ���������
        if(this->i_size<fileMgr.m_Offset)
        {
            this->i_size = fileMgr.m_Offset;
        }
        this->i_flag |= Inode::IUPD;
    }
}

DiskInode::DiskInode()
{
    this->d_mode = 0;
    this->d_nlink = 0;
    this->d_uid = -1;
    this->d_gid = -1;
    this->d_size = 0;
    for(int i = 0; i < 10; i++)
    {
        this->d_addr[i] = 0;
    }
    this->d_atime = 0;
    this->d_mtime = 0;
}
DiskInode::~DiskInode()
{

}
