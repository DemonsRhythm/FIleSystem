#ifndef BUF_H
#define BUF_H

/*
 * ������ƿ�buf����
 * ��¼����Ӧ�����ʹ���������Ϣ��
 * ͬʱ����I/O����飬��¼�û���
 * ��ص�I/O�����ִ�н����
 */
class Buf
{
public:
	enum BUfFlag{//b_flags��`�ı�־λ
		B_WRITE = 0x1,		// д�������������е���Ϣд��Ӳ��
		B_READ	= 0x2,		// �����������̶�ȡ��Ϣ��������
		B_DONE = 0x4,		// I/O��������
		B_DELWRI = 0x8,		// �ӳ�д������Ӧ����Ҫ��������ʱ���ٽ�������д����Ӧ���豸��
        B_ERROR = 0x10,		// I/O����
        B_BUSY = 0x20       //busy
	};

	unsigned int b_flags;	// ������ƿ��־λ

	int		padding;		/* 4�ֽ���䣬ʹ��b_forw��b_back��Buf������Devtab��
							 * �е��ֶ�˳���ܹ�һ�£�����ǿ��ת������� */
	/* ������ƿ���й���ָ�� */
	Buf*	b_forw;
	Buf*	b_back;
	Buf*	av_forw;
	Buf*	av_back;

	int		b_wcount;		// �贫�͵��ֽ���
	unsigned char* b_addr;	// ָ��û�����ƿ�������Ļ��������׵�ַ
	int		b_blkno;		// �����߼����
	int		b_error;		// I/O����ʱ��Ϣ
	int		b_resid;		// I/O����ʱ��δ���͵�ʣ���ֽ���
};

#endif
