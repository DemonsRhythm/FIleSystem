#ifndef FILE_SYS_H
#define FILE_SYS_H

#include "INode.h"
#include "Buf.h"
#include "BufferManager.h"

class SuperBlock//�����鶨��
{
public:
	SuperBlock();
	~SuperBlock();
public:

	int s_isize;	//���Inode��ռ�õ��̿���
	int s_fsize;	//�̿�����

	int s_nfree;	//�����̿���
	int s_free[100];//�����̿�����

	int s_ninode;	//����inode��
	int s_inode[100];//�������inode����

	int s_flock;	//���������̿��������־
	int s_ilock;	//��������inode���־

	int s_fmod;		//�ڴ���SuperBlock�������޸ı�־����Ҫ����
	int s_ronly;	//���ļ�ϵͳֻ�ܶ���
	int s_time;		//���һ�θ���ʱ��
	int padding[47];//��䣬ʹSuperBlockռ��2������
};

class FileSystem
{
public:
	static const int SUPER_BLOCK_SECTOR_NUMBER = 1;	//SuperBlock��ʼ��1#����
	static const int ROOTINO = 0;					//ϵͳ��Ŀ¼���inode���

	static const int INODE_NUMBER_PER_SECTOR = 8;	//һ�������ܴ��8��inode
	static const int INODE_ZONE_START_SECTOR = 3;	//inode����ʼ��3#����
	static const int INODE_ZONE_SIZE = 4551;		//inode��4551������

	static const int DATA_ZONE_START_SECTOR = 4554;	//����������ʼ������
	static const int DATA_ZONE_END_SECTOR = 40956;	//�������Ľ���������
	static const int DATA_ZONE_SIZE = 40957-DATA_ZONE_START_SECTOR;	//��������С

public:
	FileSystem();
	~FileSystem();
	void Initialize();//��ʼ������
    SuperBlock* GetSuperBlock();
	void LoadSuperBlock();//ϵͳ��ʼ��ʱ����SuperBlock
	void Update();//����SuperBlock
	Inode* IAlloc();//�������inode
	void IFree(int number);//�ͷ�inode
	Buf* BAlloc();//��������̿�
	void BFree(int blkno);//�ͷſ����̿�

private:
	BufferManager* bufferManager;
public:
    SuperBlock* g_spb;
};

#endif
