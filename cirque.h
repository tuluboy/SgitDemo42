// cirque.h
// ˫�˶���
#ifndef __cirque_dadfei_ckdjerjc0984_dckjembnb__
#define __cirque_dadfei_ckdjerjc0984_dckjembnb__
#include <functional>
// read �ɹ��� ���� pop
namespace zc
{
template <typename T>
class cirque
{
public:
	cirque() : m_InitSize(10)
	{
		m_pData = new T[m_InitSize];
		m_pDataEnd = m_pData + m_InitSize;

		m_pRead = nullptr; // ���п�
		m_pWrite = m_pData;
		_op = [](T* des, const T* src){*des = *src; };
	}

	cirque(int nInitSize, std::function<void(T* des, const T* src)> fct) : m_InitSize(nInitSize), _op(fct)
	{
		m_pData = new T[m_InitSize];
		m_pDataEnd = m_pData + m_InitSize;

		m_pRead = nullptr; // ���п�
		m_pWrite = m_pData;
	}

	cirque(const cirque<T>& in)
	{
	}

	~cirque()
	{
		delete[] m_pData;
	}

	void init()
	{
		while (!m_pRead)pop();
	}

	// read �ɹ������� pop
	void pop()
	{
		if (!m_pRead) return;

		m_pRead++;
		if (m_pRead == m_pDataEnd)m_pRead = m_pData;
		if (m_pRead == m_pWrite)m_pRead = nullptr; // ������
	}

	// ���ص�ǰ��β��Ԫ�أ�������Ϊ��ʱ�����ص��ǿ�ָ�룬read �ɹ������� pop
	T* read()
	{
		return m_pRead;
	}

	// ����nullptr��ʾд��ʧ��
	T* write(const T* t)
	{
		// д��ǰ�����п�λ
		if (m_pWrite == m_pRead)
		{
			return nullptr; // д��ʧ��
		}

		//*m_pWrite = *t;
		_op(m_pWrite, t); // ִ����ȿ���

		T* lastWrite = m_pWrite++;
		if (m_pWrite == m_pDataEnd)
		{
			m_pWrite = m_pData;
		}
		if (!m_pRead)m_pRead = lastWrite;
		return lastWrite;
	}

	// ����nullptr��ʾд��ʧ��
	T* write(const T t)
	{
		// д��ǰ�����п�λ
		if (m_pWrite == m_pRead)
		{
			return nullptr; // д��ʧ��
		}

		*m_pWrite = t;
		T* lastWrite = m_pWrite++;
		if (m_pWrite == m_pDataEnd)
		{
			m_pWrite = m_pData;
		}
		
		if (!m_pRead)m_pRead = lastWrite;

		return lastWrite;
	}
	// ��ȡ��ǰ���ݸ���
	int GetSize()
	{
		if (!m_pRead) return 0; // ��
		if (m_pWrite > m_pRead) return m_pWrite - m_pRead;
		else if (m_pWrite <= m_pRead) return m_pWrite + m_InitSize - m_pRead;
	}

private:
	int m_InitSize;
	T* m_pData;
	T* m_pDataEnd;
	T* m_pWrite;
	T* m_pRead;
	std::function<void(T* des, const T* src)> _op;
};

}
#endif