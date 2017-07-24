#ifndef OPEN_FILE_MANAGER_H
#define OPEN_FILE_MANAGER_H

#include "INode.h"
#include "fileSys.h"


class InodeTable
{
public:
    static const int NINODE = 100;//�ڴ�inode����

public:
    InodeTable();
    ~InodeTable();

    void Initialize();
    Inode* IGet(int inumber);   //��ȡ���inode
    void IPut(Inode* pNode);    /*���ٸ��ڴ�Inode�����ü����������Inode�Ѿ�û��Ŀ¼��ָ������
                                 * ���޽������ø�Inode�����ͷŴ��ļ�ռ�õĴ��̿顣*/
    void UpdateInodeTable();    //�����б��޸Ĺ����ڴ�Inode���µ���Ӧ���Inode��
    int  IsLoaded(int inumber); //���inode�Ƿ����ڴ����п���
    Inode* GetFreeInode();      //����ڴ�inode����һ������inode

public:
    Inode m_Inode[NINODE];
    FileSystem* fileSys;
};

#endif // OPENFILEMANAGER_H

