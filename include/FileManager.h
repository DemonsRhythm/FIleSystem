#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "fileSys.h"
#include "INode.h"
#include "DirectoryEntry.h"
#include "OpenFileManager.h"

class FileManager
{
public:
    /* Ŀ¼����ģʽ */
    enum DirectorySearchMode
    {
        OPEN = 0,   //�Դ��ļ���ʽ����Ŀ¼
        CREATE = 1, //���½��ļ���ʽ����Ŀ¼
        DELETE = 2
    };

public:
    FileManager();
    ~FileManager();

    void Initialize(bool flag);
    Inode* NameI(const char *pathname, enum DirectorySearchMode mode, bool& flag);//Ŀ¼��������·��ת����inode
    Inode* MakeNode(unsigned int mode); //��fcreat���ã����ڷ����ں���Դ
    void WriteDir(Inode* pInode);       //��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼��
    void SetCurDir(const char* pathname);     //���õ�ǰ����·��
    void ChDir(const char *pathname);                       //�ı䵱ǰ����Ŀ¼

    Inode* fopen(const char *name);
    void fclose();
    int fread(Inode *pInode, char *buf, int length);
    int fwrite(Inode *pInode, char *buf,int length);
    int fcreat(const char *name,int mode);
    void fdelete(const char *name);
    void ls();

public:
    Inode* rootDirInode;    //��Ŀ¼inodeָ��
    Inode* cdir;            //ָ��ǰĿ¼��inodeָ��
    Inode* pdir;            //ָ��Ŀ¼��inodeָ��
    DirectoryEntry dent;    //��ǰĿ¼��Ŀ¼��
    char dbuf[DirectoryEntry::DIRSIZE]; //��ǰ·������
    char curdir[128];       //��ǰ����Ŀ¼����·��

    FileSystem* m_FileSystem;
    InodeTable* m_InodeTable;

    unsigned char* m_Base;  //��ǰ����д�û�Ŀ��������׵�ַ
    int m_Offset;           //��ǰ����д�ļ����ֽ�ƫ����
    int m_Count;            //��ǰ��ʣ��Ķ���д�ֽ�����
    int f_Offset;           //�ļ���дλ��
};

#endif // FILEMANAGER_H

