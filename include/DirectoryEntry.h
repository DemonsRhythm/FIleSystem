#ifndef DIRECTORY_ENTRY_H
#define DIRECTORY_ENTRY_H

class DirectoryEntry
{
public:
    static const int DIRSIZE = 28;  //Ŀ¼����·�����ֵ�����ַ�������

public:
    int m_inumber;  //Ŀ¼����inode���
    char m_name[DIRSIZE];   //Ŀ¼����·��������
};

#endif

