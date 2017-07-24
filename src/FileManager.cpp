#include "../include/FileManager.h"
#include "../include/MyFsManager.h"
#include "../include/DirectoryEntry.h"

#include "stdio.h"
#include "string.h"
#include <iostream>

FileManager::FileManager()
{

}
FileManager::~FileManager()
{

}

void FileManager::Initialize(bool flag)
{
    this->m_FileSystem = &MyFsManager::Instance().GetFileSystem();
    this->m_InodeTable = &MyFsManager::Instance().GetInodeTable();

    /* ��ȡrootDirInode:0#inode */
    this->rootDirInode = MyFsManager::Instance().GetInodeTable().IGet(0);
    if(flag==true)
    {
        /* ��ʼ�����ڵ�Ŀ¼�����.��.. */
        Buf *pBuf = MyFsManager::Instance().GetFileSystem().BAlloc();
        this->rootDirInode->i_addr[0] = pBuf->b_blkno;
        this->rootDirInode->i_size += 2*sizeof(DirectoryEntry);
        DirectoryEntry dir[2];
        dir[0].m_name[0]='.';
        for(int i=1;i<DirectoryEntry::DIRSIZE;i++)
            dir[0].m_name[i]='\0';

        dir[0].m_inumber=this->rootDirInode->i_number;
        dir[1].m_name[0]='.';
        dir[1].m_name[1]='.';
        for(int i=2;i<DirectoryEntry::DIRSIZE;i++)
            dir[1].m_name[i]='\0';
        dir[1].m_inumber=this->rootDirInode->i_number;
        memcpy(pBuf->b_addr,dir,sizeof(dir));
        MyFsManager::Instance().GetBufferManager().Bwrite(pBuf);
        this->rootDirInode->i_flag |= Inode::IUPD;
        MyFsManager::Instance().GetInodeTable().IPut(this->rootDirInode);
    }


    /* ��ȡrootDirInode:0#inode */
    this->rootDirInode = MyFsManager::Instance().GetInodeTable().IGet(0);
    this->cdir = MyFsManager::Instance().GetInodeTable().IGet(0);
    this->pdir = MyFsManager::Instance().GetInodeTable().IGet(0);
    strcpy(this->curdir,"/");
}

int FileManager::fcreat(const char *name, int mode)//�����ļ���Ŀ¼
{
    Inode* pInode;
    bool flag;//�����ж��Ƿ��ҵ�
    pInode = this->NameI(name,FileManager::CREATE,flag);
    if(NULL == pInode && flag==true)
    {
        pInode=this->MakeNode(mode);
        if(NULL == pInode)
        {
            printf("Create Failed!!!\n");
            return -1;
        }
    }
    else if(flag==true)
    {
        pInode->ITrunc();
    }
    return 0;
}

Inode* FileManager::fopen(const char *name)//���ļ�
{
    Inode* pInode;
    bool flag;
    pInode = this->NameI(name,FileManager::OPEN,flag);
    return pInode;
}

void FileManager::fclose()
{
    this->m_Count=0;
    this->m_Offset=0;
    this->m_Base=NULL;
    this->f_Offset=0;
    //pInode->i_flag |= (Inode::IACC | Inode::IUPD);
    //this->m_InodeTable->IPut(pInode);

}

int FileManager::fread(Inode* pInode,char *buf, int length)
{
    this->m_Base = (unsigned char*)buf;
    this->m_Count = length;
    this->m_Offset = this->f_Offset;
    pInode->ReadI();
    this->f_Offset = this->m_Offset;
    return length-this->m_Count;
}

int FileManager::fwrite(Inode *pInode, char *buf, int length)
{
    this->m_Base = (unsigned char*)buf;
    this->m_Count = length;
    this->m_Offset = this->f_Offset;
    pInode->WriteI();
    this->f_Offset = this->m_Offset;
    pInode->i_flag |= (Inode::IACC | Inode::IUPD);

    return length-this->m_Count;
}

void FileManager::fdelete(const char *name)
{
    Inode* pInode;
    bool flag;
    pInode=this->NameI(name,FileManager::DELETE,flag);
    if(pInode!=NULL)//�����˸�Ŀ¼
    {
        int i;
        char iname[DirectoryEntry::DIRSIZE];
        memset(iname,0,sizeof(iname));
        for(i=strlen(name)-1;name[i]!='/'&&i>=0;i--)
            ;
        if(i<0)
            i++;
        strcpy(iname,name+i);
        DirectoryEntry tmpdir;
        int count;
        while((count = this->fread(pInode,(char *)&tmpdir,32))!=0)
        {
            if(strcmp(tmpdir.m_name,iname)==0)//�ҵ�Ҫɾ����Ŀ¼��
            {
                Inode* tmpnode;
                tmpnode = this->m_InodeTable->IGet(tmpdir.m_inumber);
                tmpnode->ITrunc();
                MyFsManager::Instance().GetFileSystem().IFree(tmpdir.m_inumber);
                memset(&tmpdir,0,sizeof(tmpdir));
                this->m_Offset-=sizeof(DirectoryEntry);
                this->m_Count=sizeof(DirectoryEntry);//DirectoryEntry::DIRSIZE + 4;
                this->m_Base=(unsigned char*)&tmpdir;

                pInode->WriteI();//Ŀ¼��д�븸Ŀ¼
                this->m_InodeTable->IPut(pInode);
                break;
            }
        }
    }
}

char* simplifyPath(char* path)
{
    int top = -1;
    int i;
    int j;

    for(i = 0; path[i] != '\0'; ++i)
    {
        path[++top] = path[i];
        if(top >= 1 && path[top - 1] == '/' && path[top] == '.' && (path[i + 1] == '/' || path[i + 1] == '\0'))
        {
            top -= 2;
        }
        else if(top >= 2 && path[top - 2] == '/' && path[top - 1] == '.' && path[top] == '.' && (path[i + 1] == '/' || path[i + 1] == '\0'))
        {
            for(j = top - 3; j >= 0; --j)
            {
                if(path[j] == '/') break;
            }
            if(j < 0)
            {
                top = -1;
            }
            else
            {
                top = j - 1;
            }
        }
        else if(path[top] == '/' && path[i + 1] == '/') --top;
    }
    if(top > 0)
    {
        if(path[top] == '/') path[top] = '\0';
        else path[top + 1] = '\0';
    }
    else if(top == 0) path[top + 1] = '\0';
    else
    {
        path[0] = '/';
        path[1] = '\0';
    }
    return path;
}

void FileManager::ChDir(const char *pathname)
{
    Inode* pInode;
    bool flag;
    pInode = this->NameI(pathname,FileManager::OPEN,flag);
    if(pInode == NULL)
    {
        //printf("Dir not found!!!\n");
        return;
    }
    if((pInode->i_mode & Inode::IFMT)!=Inode::IFDIR)
    {
        this->m_InodeTable->IPut(pInode);
        printf("%s is not a dir!!!\n",pathname);
        return;
    }
    this->m_InodeTable->IPut(this->cdir);
    this->cdir = pInode;

    char* newpathname = (char *)pathname;
    newpathname = simplifyPath(newpathname);
    this->SetCurDir(pathname);
}

void FileManager::ls()
{
    DirectoryEntry tmp;
    int count;
    this->f_Offset=0;
    while((count = this->fread(this->cdir,(char *)&tmp,32))!=0)
    {
        if(tmp.m_inumber>0||tmp.m_name[0]=='.')
            printf("%s\t",tmp.m_name);
    }
    printf("\n");
    this->fclose();
}

/* ����NULL��ʾĿ¼����ʧ�ܣ������Ǹ�ָ�룬ָ���ļ����ڴ��inode */
Inode* FileManager::NameI(const char *pathname, DirectorySearchMode mode, bool& flag)
{
    Inode* pInode;
    Buf* pBuf;
    char curchar;
    char* pChar;
    int freeEntryOffset;    //�Դ����ļ���ʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ����
    BufferManager& bufMgr = MyFsManager::Instance().GetBufferManager();

    flag=true;

    /*
     * �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
     * ����ӽ��̵�ǰ����Ŀ¼��ʼ������
     */

    if('/'==(curchar=*pathname++))
        pInode = this->rootDirInode;
    else
        pInode = this->cdir;

    /* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
    this->m_InodeTable->IGet(pInode->i_number);

    while ( '/' == curchar )
    {
        curchar = *pathname++;
    }

    /* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
    if('\0'==curchar && mode!=FileManager::OPEN)
    {
        printf("Not allowed to change or delete curdir!!!\n");
        this->m_InodeTable->IPut(pInode);
        return NULL;
    }

    /* ���ѭ��ÿ�δ���pathname��һ��·������ */
    while(true)
    {
        /* �������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳� */
        //

        /* ����·��������ϣ��򷵻���Ӧinodeָ�� */
        if('\0'==curchar)
            return pInode;

        /* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
        if((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
        {
            printf("Not dir!!!\n");
            break;
        }

        /* ��Pathname�е�ǰ׼������ƥ���·������������dbuf[]�У����ں�Ŀ¼����бȽ� */
        pChar = &(this->dbuf[0]);
        while('/'!=curchar && '\0'!=curchar)
        {
            if(pChar < &(this->dbuf[DirectoryEntry::DIRSIZE]))
            {
                *pChar = curchar;
                pChar++;
            }
            curchar=*pathname++;
        }
        /* ��dbufʣ��Ĳ������Ϊ'\0' */
        while(pChar < &(this->dbuf[DirectoryEntry::DIRSIZE]))
        {
            *pChar = '\0';
            pChar++;
        }
        /* �������////a//b ����·�� ����·���ȼ���/a/b */
        while ( '/' == curchar )
        {
            curchar = *pathname++;
        }

        /* �ڲ�ѭ�����ֶ���dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
        this->m_Offset = 0;
        /* ����ΪĿ¼����������հ�Ŀ¼�� */
        this->m_Count = pInode->i_size / (DirectoryEntry::DIRSIZE + 4);
        freeEntryOffset = 0;
        pBuf = NULL;
        while(true)
        {
            if(this->m_Count==0)//��Ŀ¼���Ѿ�������
            {
                if(pBuf!=NULL)
                    bufMgr.Brelse(pBuf);
                /* if create */
                if(FileManager::CREATE == mode && curchar=='\0')
                {
                    /* ����Ŀ¼inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
                    this->pdir = pInode;

                    if(freeEntryOffset)
                        this->m_Offset = freeEntryOffset - sizeof(DirectoryEntry);//(DirectoryEntry::DIRSIZE +  4);
                    else
                        pInode->i_flag |= Inode::IUPD;
                    /* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
                    return NULL;
                }
                /* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����˳� */
                this->m_InodeTable->IPut(pInode);
                flag=false;
                printf("Dir not found!!!\n");
                return NULL;
            }
            /* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
            if(this->m_Offset%Inode::BLOCK_SIZE == 0)
            {
                if(pBuf!=NULL)
                    bufMgr.Brelse(pBuf);
                /* ����Ҫ���������̿�� */
                int phyBlkno = pInode->Bmap(this->m_Offset/Inode::BLOCK_SIZE);
                pBuf = bufMgr.Bread(phyBlkno);
            }
            /* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����dent */
            int *src = (int *)(pBuf->b_addr+(this->m_Offset%Inode::BLOCK_SIZE));
            memcpy((int *)&this->dent,src,sizeof(DirectoryEntry));

            this->m_Offset+=sizeof(DirectoryEntry);//(DirectoryEntry::DIRSIZE + 4);
            this->m_Count--;
            /* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
            if(this->dent.m_inumber==0 && this->m_Offset>64)
            {
                if(freeEntryOffset==0)
                    freeEntryOffset=this->m_Offset;
                continue;
            }
            int i;
            for(i=0;i<DirectoryEntry::DIRSIZE;i++)
            {
                if(this->dbuf[i]!=this->dent.m_name[i])
                    break;// ƥ����ĳһ�ַ�����������forѭ��
            }
            if(i<DirectoryEntry::DIRSIZE)// ����Ҫ������Ŀ¼�����ƥ����һĿ¼��
                continue;
            else//Ŀ¼��ƥ��ɹ����ص����While(true)ѭ��
                break;
        }
        /*
         * ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
         * ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
         * ������ֱ������'\0'������
         */
        if(pBuf!=NULL)
            bufMgr.Brelse(pBuf);
        /* �����ɾ���������򷵻ظ�Ŀ¼inode����Ҫɾ���ļ���inode����dent.m_inumber�� */
        if(FileManager::DELETE == mode && '\0'==curchar)
        {
            return pInode;
        }
        /*
         * ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼inode������ƥ��ɹ���
         * Ŀ¼��m_inumber�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���inode��
         */
        this->m_InodeTable->IPut(pInode);
        pInode = this->m_InodeTable->IGet(this->dent.m_inumber);
        /* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

        if ( pInode==NULL )	/* ��ȡʧ�� */
        {
            return NULL;
        }
    }
    this->m_InodeTable->IPut(pInode);
    return NULL;
}

/* ��fcreat���� */
Inode* FileManager::MakeNode(unsigned int mode)
{
    Inode* pInode;
    pInode = this->m_FileSystem->IAlloc();
    if(pInode==NULL)
        return NULL;
    pInode->i_flag |= (Inode::IACC | Inode::IUPD);
    pInode->i_mode = mode | Inode::IALLOC;
    pInode->i_nlink = 1;
    pInode->i_uid = -1;
    pInode->i_gid = -1;
    this->WriteDir(pInode);
    return pInode;
}

void FileManager::WriteDir(Inode *pInode)
{
    if((pInode->i_mode & Inode::IFMT) == Inode::IFDIR)//����Ŀ¼��Ҫ�����µ�Ŀ¼�ļ�
    {
        /* ��ʼ��Ŀ¼�����.��.. */
        Buf *pBuf = MyFsManager::Instance().GetFileSystem().BAlloc();
        pInode->i_addr[0] = pBuf->b_blkno;
        pInode->i_size += 2*sizeof(DirectoryEntry);
        DirectoryEntry dir[2];
        dir[0].m_name[0]='.';
        for(int i=1;i<DirectoryEntry::DIRSIZE;i++)
            dir[0].m_name[i]='\0';

        dir[0].m_inumber=pInode->i_number;//��ǰĿ¼inode�����Լ�
        dir[1].m_name[0]='.';
        dir[1].m_name[1]='.';
        for(int i=2;i<DirectoryEntry::DIRSIZE;i++)
            dir[1].m_name[i]='\0';
        dir[1].m_inumber=this->pdir->i_number;//��Ŀ¼inode
        memcpy(pBuf->b_addr,dir,sizeof(dir));
        MyFsManager::Instance().GetBufferManager().Bwrite(pBuf);
    }

    this->dent.m_inumber = pInode->i_number;
    for(int i=0;i<DirectoryEntry::DIRSIZE;i++)
        this->dent.m_name[i] = this->dbuf[i];
    this->m_Count=sizeof(DirectoryEntry);//DirectoryEntry::DIRSIZE + 4;
    this->m_Base=(unsigned char*)&this->dent;

    this->pdir->WriteI();//Ŀ¼��д�븸Ŀ¼
    this->m_InodeTable->IPut(this->pdir);
    this->m_InodeTable->IPut(pInode);
}

void FileManager::SetCurDir(const char *pathname)
{
    if(strcmp(pathname,".")==0)//��ǰĿ¼
        return;
    else if(strcmp(pathname,"..")==0)//��Ŀ¼
    {
        int length = strlen(this->curdir);
        if(this->curdir[length-1]!='/')
        {
            int i;
            for(i=length-1;curdir[i]!='/'&&i>=0;i--)
                ;
            char *tmpdir = new char[50];
            strcpy(tmpdir,this->curdir);
            strncpy(this->curdir,tmpdir,i+1);
            this->curdir[i+1]='\0';
            delete tmpdir;
        }
    }
    /* ·�����ǴӸ�Ŀ¼'/'��ʼ����������curdir������ϵ�ǰ·������ */
    else if(pathname[0]!='/')
    {
        int length = strlen(this->curdir);
        if(this->curdir[length-1]!='/')
        {
            this->curdir[length] = '/';
            length++;
        }
        strcpy(this->curdir+length,pathname);
    }
    else/* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
        strcpy(this->curdir,pathname);
}
