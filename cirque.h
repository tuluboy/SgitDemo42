// cirque.h
// 双端队列
#ifndef __cirque_dadfei_ckdjerjc0984_dckjembnb__
#define __cirque_dadfei_ckdjerjc0984_dckjembnb__
#include <functional>
// read 成功后 必须 pop
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

		m_pRead = nullptr; // 队列空
		m_pWrite = m_pData;
		_op = [](T* des, const T* src){*des = *src; };
	}

	cirque(int nInitSize, std::function<void(T* des, const T* src)> fct) : m_InitSize(nInitSize), _op(fct)
	{
		m_pData = new T[m_InitSize];
		m_pDataEnd = m_pData + m_InitSize;

		m_pRead = nullptr; // 队列空
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

	// read 成功后必须调 pop
	void pop()
	{
		if (!m_pRead) return;

		m_pRead++;
		if (m_pRead == m_pDataEnd)m_pRead = m_pData;
		if (m_pRead == m_pWrite)m_pRead = nullptr; // 弹空了
	}

	// 返回当前队尾的元素，当队列为空时，返回的是空指针，read 成功后必须调 pop
	T* read()
	{
		return m_pRead;
	}

	// 返回nullptr表示写入失败
	T* write(const T* t)
	{
		// 写入前必须有空位
		if (m_pWrite == m_pRead)
		{
			return nullptr; // 写入失败
		}

		//*m_pWrite = *t;
		_op(m_pWrite, t); // 执行深度拷贝

		T* lastWrite = m_pWrite++;
		if (m_pWrite == m_pDataEnd)
		{
			m_pWrite = m_pData;
		}
		if (!m_pRead)m_pRead = lastWrite;
		return lastWrite;
	}

	// 返回nullptr表示写入失败
	T* write(const T t)
	{
		// 写入前必须有空位
		if (m_pWrite == m_pRead)
		{
			return nullptr; // 写入失败
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
	// 获取当前数据个数
	int GetSize()
	{
		if (!m_pRead) return 0; // 空
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