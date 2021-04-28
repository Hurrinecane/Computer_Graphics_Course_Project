#pragma once

class IndexBuffer
{
public:
	IndexBuffer(const  unsigned int* data, unsigned int count);
	~IndexBuffer();

	void Bind() const;
	void Unbind() const;

private:
	unsigned int m_RendererID;
	unsigned int m_Count;
};