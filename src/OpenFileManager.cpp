#include "../include/OpenFileManager.h"
#include "../include/MyFsManager.h"

#include <time.h>
#include <stdio.h>


InodeTable::InodeTable()
{

}
InodeTable::~InodeTable()
{

}

void InodeTable::Initialize()
{
    this->fileSys = &MyFsManager::Instance().GetFileSystem();
}

Inode* InodeTable::IGet(int inumber)
{
    Inode *pInode;
    while(true)
    {
        /* ���ָ���豸dev�б��Ϊinumber�����Inode�Ƿ����ڴ濽�� */
        int index = this->IsLoaded(inumber);
        if(index>=0)//�ҵ�����
        {
            pInode = &(this->m_Inode[index]);
            pInode->i_count++;
            pInode->i_flag |= Inode::ILOCK;
            return pInode;
        }
        else//��Ҫ�������inode
        {
            pInode = this->GetFreeInode();
            /* �ڴ�inode������ */
            if(pInode==NULL)
            {
                printf("Inode Table Overflow!!!\n");
                return NULL;
            }
            else//����ɹ����������inode
            {
                pInode->i_number = inumber;
                pInode->i_flag = Inode::ILOCK;
                pInode->i_count++;

                BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();
                Buf *pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);

                pInode->ICopy(pBuf,inumber);
                bufMgr.Brelse(pBuf);
                return pInode;
            }
        }
    }
    return NULL;
}

/* close�ļ�ʱ�����Iput
 *      ��Ҫ���Ĳ������ڴ�i�ڵ���� i_count--����Ϊ0���ͷ��ڴ� i�ڵ㡢���иĶ�д�ش���
 * �����ļ�;��������Ŀ¼�ļ������������󶼻�Iput���ڴ�i�ڵ㡣·�����ĵ�����2��·������һ���Ǹ�
 *   Ŀ¼�ļ�������������д������ļ�������ɾ��һ�������ļ���������������д���ɾ����Ŀ¼����ô
 *   	���뽫���Ŀ¼�ļ�����Ӧ���ڴ� i�ڵ�д�ش��̡�
 *   	���Ŀ¼�ļ������Ƿ��������ģ����Ǳ��뽫����i�ڵ�д�ش��̡�
 */
void InodeTable::IPut(Inode *pNode)
{
    pNode->IUpdate(time(NULL));
    if(pNode->i_count==1)//��������ʱ������
    {
        if(pNode->i_nlink<=0)//���ļ��Ѿ�û��Ŀ¼·��ָ����
        {
            pNode->ITrunc();
            pNode->i_mode=0;
            this->fileSys->IFree(pNode->i_number);
        }
        pNode->i_flag=0;
        pNode->i_number=-1;//��ʾ����
        pNode->i_count--;
    }

}

void InodeTable::UpdateInodeTable()
{
    for(int i=0;i<InodeTable::NINODE;i++)
    {
        /* ���Inode����count������0��count == 0��ζ�Ÿ��ڴ�Inodeδ���κδ��ļ����ã�����ͬ����*/
        if(this->m_Inode[i].i_count!=0)
        {
            this->m_Inode[i].IUpdate(time(NULL));
        }
    }
}

int InodeTable::IsLoaded(int inumber)
{
    for(int i=0;i<InodeTable::NINODE;i++)
    {
        if(this->m_Inode[i].i_number==inumber && this->m_Inode[i].i_count!=0)
            return i;
    }
    return -1;
}

Inode* InodeTable::GetFreeInode()
{
    for(int i=0;i<InodeTable::NINODE;i++)
    {
        if(this->m_Inode[i].i_count==0)
            return &(this->m_Inode[i]);
    }
    return NULL;
}
